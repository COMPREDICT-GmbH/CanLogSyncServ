
#pragma once

#include <Vector/DBC.h>
#include <cstdint>

class DBCSignal_Wrapper
{
public:
	using id_t = uint64_t;
	id_t id;
	Vector::DBC::Signal dbc_signal;
};
