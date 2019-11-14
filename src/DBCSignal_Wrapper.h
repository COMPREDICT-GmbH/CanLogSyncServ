
#pragma once

#include <memory>
#include <Vector/DBC.h>
#include <cstdint>

class DBCSignal_Wrapper
{
public:
	using id_t = uint64_t;
	id_t id;
	Vector::DBC::Signal dbc_signal;
	std::shared_ptr<Vector::DBC::Signal> dbc_mux_signal;
};
