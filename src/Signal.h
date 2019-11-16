
#pragma once

#include <cstdint>
#include <chrono>
#include <dbcppp/Network.h>
#include "DBCSignal_Wrapper.h"

struct Signal
{
	using id_t = uint64_t;
	id_t id;
	uint64_t bus_id;
	double value;
	std::shared_ptr<dbcppp::Signal> dbc_signal;
	std::chrono::microseconds timestamp;
	void* user_data;
};
