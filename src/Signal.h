
#pragma once

#include <cstdint>
#include <chrono>
#include <Vector/DBC.h>
#include "DBCSignal_Wrapper.h"

struct Signal
{
	using id_t = uint64_t;
	id_t id;
	uint64_t bus_id;
	double value;
	const Vector::DBC::Signal* dbc_signal;
	std::chrono::microseconds timestamp;
	void* user_data;
};
