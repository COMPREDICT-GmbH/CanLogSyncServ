
#include <zmq.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <boost/program_options.hpp>
#include <dbcppp/Network.h>
#include <dbcppp/DBC_Grammar.h>

#include "ConfigParser.h"
#include "Can.h"
#include "CanSync.h"
#include "CanBus.h"
#include "ZmqServer.h"

int main(int argc, char** argv)
{
	namespace po = boost::program_options;
	po::options_description desc;
	desc.add_options()
		("help", "produce help message")
		("config", po::value<std::string>()->default_value(""), "config file")
		("dbc", po::value<std::string>()->required(), "DBC file")
		("ipc_link", po::value<std::vector<std::string>>()->multitoken()->required(),
			"List of IPC links. E.g. ipc:///tmp/weather.ipc or tcp://*:5556, "
			"for a exhaustive list of supported protocols, please go to http://wiki.zeromq.org/docs:features "
			"under the Protocols section.")
		("iface", po::value<std::vector<std::string>>()->multitoken()->required(), "list of CAN interfaces")
		("sample_rate", po::value<uint64_t>()->default_value(5000), "sample rate in microseconds")
		("signal", po::value<std::vector<std::string>>()->multitoken(), "list of signals to log");

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	if (vm.empty() || vm.count("help"))
	{
		std::cout << "usage: CanLogSyncServ "
			"--config=<config_file> "
			"--dbc=<dbc_file> "
			"--iface=<iface>... "
			"--ipc_link=<ipc_link>... "
			"[--sample_rate=<sample_rate>]"
			<< std::endl;
		std::cout << desc << std::endl;
		return 0;
	}
	po::notify(vm);
	const auto& config_file_path = vm["config"].as<std::string>();
	const auto& dbc_file_path    = vm["dbc"].as<std::string>();
	const auto& ipc_links        = vm["ipc_link"].as<std::vector<std::string>>();
	const auto& ifaces           = vm["iface"].as<std::vector<std::string>>();
	const auto& sample_rate      = vm["sample_rate"].as<uint64_t>();
	const auto& cmd_signals      = vm["signal"];
    
	ConfigParser cfg{cmd_signals.empty() ? std::vector<std::string>{} : cmd_signals.as<std::vector<std::string>>(), config_file_path};
	const auto& cfg_sigs = cfg.signals();

	dbcppp::Network network;
	{
		std::ifstream dbc_file{dbc_file_path};
		dbc_file >> network;
	}

	std::vector<std::pair<canid_t, std::vector<DBCSignal_Wrapper>>> msgs;
	std::vector<CanBus> can_buses;
	{
		for (const auto& msg : network.messages)
		{
			std::vector<DBCSignal_Wrapper> wrappers;
			std::shared_ptr<dbcppp::Signal> mux_sig;
			auto sig_mux_iter = std::find_if(msg.second->signals.begin(), msg.second->signals.end(),
				[](const auto& sig)
				{
					return sig.second->multiplexer_indicator == dbcppp::Signal::Multiplexer::MuxSwitch;
				});
		    if (sig_mux_iter != msg.second->signals.end())
			{
				mux_sig = sig_mux_iter->second;
			}
			for (const auto& sig : msg.second->signals)
			{
				auto iter = std::find_if(cfg_sigs.begin(), cfg_sigs.end(),
					[&](const ConfigParser::CfgSignal& cfg_sig)
					{
						return cfg_sig.canid == msg.first && sig.second->name == cfg_sig.signal_name;
					});
				if (iter != cfg_sigs.end())
				{
					DBCSignal_Wrapper sig_wrapper;
					sig_wrapper.id = iter->signal_id;
					sig_wrapper.dbc_signal = sig.second;
					if (sig.second->multiplexer_indicator == dbcppp::Signal::Multiplexer::MuxValue)
					{
						sig_wrapper.dbc_mux_signal = mux_sig;
					}
					wrappers.push_back(sig_wrapper);
				}
			}
			if (wrappers.size())
			{
				msgs.push_back(std::make_pair(msg.second->id, std::move(wrappers)));
			}
		}
		std::vector<canid_t> filter_ids;
		for (const auto& msg : msgs)
		{
			filter_ids.push_back(msg.first);
		}
		for (const auto& iface : ifaces)
		{
			can_buses.emplace_back(0, Can{iface, filter_ids}, std::move(msgs));
		}
	}
	
	struct Sub
		: CanSync::Subscriber
	{
		Sub(ZmqServer& zmq_server) : _zmq_server{zmq_server} {}
		virtual void update(std::chrono::microseconds timestamp, const std::vector<CanSync::SubData>& data) override
		{
			_zmq_server.cb_sub(timestamp, data);
		}
		ZmqServer& _zmq_server;
	};
	ZmqServer zmq_server{ipc_links};
	CanSync can_sync{std::chrono::microseconds{sample_rate}, std::move(can_buses)};
	{
		// for some reason I don't understand if the unique_ptr is passed directly to
		// the subscribe funciton, the next ::recvmsg call in Can will return 0 what will
		// lead to an error throw
		// So the unique_ptr is firstly assigned to a tmp variable
		auto sub = std::make_unique<Sub>(zmq_server);
		can_sync.subscribe(std::move(sub));
	}
	can_sync.start();

	while (can_sync.running())
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	return 0;
}
