
#pragma once

#include <memory>
#include <cstdint>
#include <dbcppp/Network.h>

class DBCSignal_Wrapper
{
public:
	using id_t = uint64_t;
	id_t id;
	std::shared_ptr<dbcppp::Signal> dbc_signal;
	std::shared_ptr<dbcppp::Signal> dbc_mux_signal;
};
