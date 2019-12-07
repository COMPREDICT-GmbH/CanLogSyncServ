
#include <signal.h>

#include <zmq.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <atomic>
#include <iostream>
#include <fstream>
#include <sstream>
#include <condition_variable>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <dbcppp/Network.h>
#include <dbcppp/DBC_Grammar.h>

#include "ConfigParser.h"
#include "Can.h"
#include "CanSync.h"
#include "CanBus.h"
#include "ZmqServer.h"

std::atomic<bool> g_running;
void term(int signum)
{
	g_running = false;
}
void install_SIGTERM_handler()
{
	struct sigaction action;
	std::memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = term;
	::sigaction(SIGTERM, &action, NULL);
}
static void insert_msgs_into_canbuses(
		std::vector<CanBus>& can_buses,
		const std::vector<ConfigParserSignal::CfgSignal>& cfg_sigs,
		const std::vector<ConfigParserCanBus::CfgCanBus>& cfg_can_buses
	)
{
	std::vector<ConfigParserSignal::CfgSignal> del_list;
	for (const ConfigParserCanBus::CfgCanBus& cfg_can_bus : cfg_can_buses)
	{
		dbcppp::Network network;
		{
			if (!boost::filesystem::exists(cfg_can_bus.dbc_file_path))
			{
				throw std::runtime_error("couldn't find dbc file \"" + cfg_can_bus.dbc_file_path + "\"");
			}
			std::ifstream dbc_file{cfg_can_bus.dbc_file_path};
			if (!(dbc_file >> network))
			{
				throw std::runtime_error("couldn't parse dbc file \"" + cfg_can_bus.dbc_file_path + "\"");
			}
		}
		std::vector<std::pair<canid_t, std::vector<DBCSignal_Wrapper>>> msgs;
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
					[&](const ConfigParserSignal::CfgSignal& cfg_sig)
					{
						return cfg_sig.busid == cfg_can_bus.busid && cfg_sig.canid == msg.first
							&& sig.second->name == cfg_sig.signal_name;
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
					std::cout << "Added Signal: " << msg.second->name << "::" << sig.second->name << std::endl;
					del_list.push_back(*iter);
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
		Can can{cfg_can_bus.iface};
		can.set_filters(filter_ids);
		can_buses.emplace_back(0, std::move(can), std::move(msgs));
	}
	std::vector<ConfigParserSignal::CfgSignal> delta;
	std::set_difference(
		cfg_sigs.begin(), cfg_sigs.end(),
		del_list.begin(), del_list.end(),
		std::back_inserter(delta),
		[](const auto& lhs, const auto& rhs)
		{
			return lhs.canid < rhs.canid && lhs.signal_name < rhs.signal_name;
		});
	if (delta.size())
	{
		throw std::runtime_error("Couldn't find signal with canid=" + std::to_string(delta[0].canid) + " and signal_name="  + delta[0].signal_name);
	}

}

int main(int argc, char** argv)
{
	install_SIGTERM_handler();

	namespace po = boost::program_options;
	po::options_description desc;
	desc.add_options()
		("help", "produce help message")
		("config", po::value<std::string>()->default_value(""), "config file")
		("ipc_link", po::value<std::vector<std::string>>()->multitoken()->required(),
			"List of IPC links. E.g. ipc:///tmp/weather.ipc or tcp://*:5556, "
			"for a exhaustive list of supported protocols, please go to http://wiki.zeromq.org/docs:features "
			"under the Protocols section.")
		("can_bus", po::value<std::vector<std::string>>()->multitoken()->required(), "list of busids, CAN interfaces and DBC files")
		("sample_rate", po::value<uint64_t>()->default_value(5000), "sample rate in microseconds")
		("signal", po::value<std::vector<std::string>>()->multitoken(), "list of signals");
	if (argc == 1)
	{
		std::cout << "usage: CanLogSyncServ "
			"--config=<config_file> "
			"--can_bus=<<busid>;<iface>;<dbc>>... "
			"--ipc_link=<ipc_link>... "
			"[--sample_rate=<sample_rate>] "
			"[--signal=<<busid;<canid>;<signal_name>;<signal_id>>...]"
			<< std::endl;
		std::cout << desc << std::endl;
		return 0;
	}
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	if (vm.empty() || vm.count("help"))
	{
		std::cout << "usage: CanLogSyncServ "
			"--config=<config_file> "
			"--can_bus=<<busid>;<iface>;<dbc>>... "
			"--ipc_link=<ipc_link>... "
			"[--sample_rate=<sample_rate>] "
			"[--signal=<<busid;<canid>;<signal_name>;<signal_id>>...]"
			<< std::endl;
		std::cout << desc << std::endl;
		return 0;
	}
	po::notify(vm);
	const auto config_file_path = vm["config"].as<std::string>();
	const auto ipc_links        = vm["ipc_link"].as<std::vector<std::string>>();
	const auto cmd_can_buses    = vm["can_bus"].as<std::vector<std::string>>();
	const auto sample_rate      = vm["sample_rate"].as<uint64_t>();
	const auto cmd_signals      = vm["signal"];
    
	std::vector<CanBus> can_buses;
	{
		std::vector<std::string> cmd_signal_lines;
		if (!cmd_signals.empty())
		{
			const auto& sigs = cmd_signals.as<std::vector<std::string>>();
			cmd_signal_lines.insert(cmd_signal_lines.end(), sigs.begin(), sigs.end());
		}
		if (config_file_path != "")
		{
			if (!boost::filesystem::exists(config_file_path))
			{
				throw std::runtime_error("Couldn't find \"" + config_file_path + "\"");
			}
			std::ifstream cf{config_file_path};
			for (std::string line; std::getline(cf, line);) cmd_signal_lines.push_back(line);
		}
		ConfigParserSignal cfg_sig{cmd_signal_lines};
		const auto& cfg_sigs = cfg_sig.signals();
		if (cfg_sigs.empty())
		{
			throw std::runtime_error("no signals given, or they couldn't be parsed");
		}
		ConfigParserCanBus cfg_can_bus{cmd_can_buses};
		const auto& cfg_can_buses = cfg_can_bus.can_buses();
		if (cfg_can_buses.empty())
		{
			throw std::runtime_error("no CAN buses given, or they couldn't be parsed");
		}
		insert_msgs_into_canbuses(can_buses, cfg_sigs, cfg_can_buses);
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

	g_running = true;
	while (g_running)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
	return 0;
}
