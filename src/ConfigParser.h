
#pragma once

#include <linux/can.h>
#include <linux/can/raw.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include "Signal.h"
#include "CanBus.h"

class ConfigParserCanBus
{
public:
	struct CfgCanBus
	{
		CanBus::id_t busid;
		std::string iface;
		std::string dbc_file_path;
	};
    ConfigParserCanBus(const std::vector<std::string>& cmd_can_buses);
    const std::vector<CfgCanBus>& can_buses() const;

private:
    std::vector<CfgCanBus> _can_buses;
};
class ConfigParserSignal
{
public:
    struct CfgSignal
    {
		CanBus::id_t busid;
        canid_t canid;
        std::string signal_name;
        Signal::id_t signal_id;
    };

    ConfigParserSignal(const std::vector<std::string>& cmd_signals);
    const std::vector<CfgSignal>& signals() const;

private:
    std::vector<CfgSignal> _signals;
};
