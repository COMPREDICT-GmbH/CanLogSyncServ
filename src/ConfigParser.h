
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

    ConfigParser(const std::vector<std::string>& cmd_signals, const std::string& config_file_path);
    const std::vector<CfgSignal>& signals() const;

private:
    std::vector<CfgSignal> _signals;
};

