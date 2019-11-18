
#include "ConfigParser.h"

ConfigParserCanBus::ConfigParserCanBus(const std::vector<std::string>& cmd_can_buses)
{
	auto push = [this](const std::string& line)
		{
			std::istringstream iss(line);
			std::string busid, iface, dbc_file_path;
			// just skip incorrect lines
			if (!std::getline(iss, busid, ';'))       return;
			if (!std::getline(iss, iface, ';'))       return;
			if (!std::getline(iss, dbc_file_path))    return;
			CfgCanBus can_bus;
			can_bus.busid = std::atoi(busid.c_str());
			can_bus.iface = iface;
			can_bus.dbc_file_path = dbc_file_path;
			_can_buses.push_back(can_bus);
		};
	for (const auto& line : cmd_can_buses) push(line);
}
const std::vector<ConfigParserCanBus::CfgCanBus>& ConfigParserCanBus::can_buses() const
{
	return _can_buses;
}

ConfigParserSignal::ConfigParserSignal(const std::vector<std::string>& cmd_signals)
{
	auto push = [this](const std::string& line)
		{
			std::istringstream iss(line);
			std::string busid, canid, signal_name, signal_id;
			// just skip incorrect lines
			if (!std::getline(iss, busid, ';'))       return;
			if (!std::getline(iss, canid, ';'))       return;
			if (!std::getline(iss, signal_name, ';')) return;
			if (!std::getline(iss, signal_id))        return;
			CfgSignal sig;
			sig.busid = std::atoi(busid.c_str());
			sig.canid = std::atoi(canid.c_str());
			sig.signal_name = signal_name;
			sig.signal_id = std::atoi(signal_id.c_str());
			_signals.push_back(sig);
		};
	for (const auto& line : cmd_signals) push(line);
}
const std::vector<ConfigParserSignal::CfgSignal>& ConfigParserSignal::signals() const
{
    return _signals;
}

