
#pragma once

#include <zmq.hpp>
#include <vector>
#include "CanSync.h"

class ZmqServer
{
public:
	ZmqServer(const std::vector<std::string>& ipc_links);
	~ZmqServer();
	void cb_sub(std::chrono::microseconds timestamp, const std::vector<CanSync::SubData>& sub_data);

private:
	zmq::context_t _context;
	zmq::socket_t _publisher;
};

