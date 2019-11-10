
#pragma once

#include <zmq.hpp>
#include <vector>
#include "CanSync.h"

class ZmqServer
{
public:
	ZmqServer(const std::vector<std::string>& ipc_links);
	void cb_sub(const std::vector<CanSync::SubData>& sub_data);

private:
	zmq::context_t _context;
	zmq::socket_t _publisher;
};

