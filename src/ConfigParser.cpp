
#include "ConfigParser.h"

ConfigParser::ConfigParser(const std::vector<std::string>& cmd_signals, const std::string& config_file_path)
{
	std::ifstream cf{config_file_path};
	auto push = [this](const std::string& line)
	    {
            std::istringstream iss(line);
            std::string canid, signal_name, signal_id;
            // just skip incorrect lines
            if (!std::getline(iss, canid, ';'))       return;
            if (!std::getline(iss, signal_name, ';')) return;
            if (!std::getline(iss, signal_id))        return;
            CfgSignal sig;
            sig.canid = std::atoi(canid.c_str());
            sig.signal_name = signal_name;
            sig.signal_id = std::atoi(signal_id.c_str());
            _signals.push_back(sig);
        };
        for (const auto& line : cmd_signals)            push(line);
        for (std::string line; std::getline(cf, line);) push(line);
}
const std::vector<ConfigParser::CfgSignal>& ConfigParser::signals() const
{
    return _signals;
}

