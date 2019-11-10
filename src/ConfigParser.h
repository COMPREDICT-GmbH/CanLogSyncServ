
#pragma once

#include <linux/can.h>
#include <linux/can/raw.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include "Signal.h"

class ConfigParser
{
public:
	struct CfgSignal
	{
		canid_t canid;
		std::string signal_name;
		Signal::id_t signal_id;
	};

	ConfigParser(const std::string& config_file_path)
	{
		std::ifstream cf{config_file_path};
		for (std::string line; std::getline(cf, line);)
		{
			std::istringstream iss(line);
			std::string canid, signal_name, signal_id;
			// just skip incorrect lines
			if (!std::getline(iss, canid, ';'))			continue;
			if (!std::getline(iss, signal_name, ';'))	continue;
			if (!std::getline(iss, signal_id))			continue;
			CfgSignal sig;
			sig.canid = std::atoi(canid.c_str());
			sig.signal_name = signal_name;
			sig.signal_id = std::atoi(signal_id.c_str());
			_signals.push_back(sig);
		}
	}
	const std::vector<CfgSignal>& signals() const
	{
		return _signals;
	}

private:
	std::vector<CfgSignal> _signals;
};
