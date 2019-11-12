
#include "ZmqServer.h"
#include <iostream>

ZmqServer::ZmqServer(const std::vector<std::string>& ipc_links)
{
	_context = zmq::context_t{1};
	_publisher = zmq::socket_t{_context, ZMQ_PUB};
	for (const auto& ipc_link : ipc_links)
	{
		_publisher.bind(ipc_link);
	}
}
void ZmqServer::cb_sub(std::chrono::microseconds timestamp, const std::vector<CanSync::SubData>& sub_data)
{
	std::size_t nbyte = sub_data.size() * sizeof(CanSync::SubData);
	char buffer[nbyte + 16];
	uint64_t ts = (uint64_t)timestamp.count();
	uint64_t n  = (uint64_t)sub_data.size();
	std::memcpy(buffer, &ts, 8);
	std::memcpy(buffer + 8, &n, 8);
	std::memcpy(buffer + 16, &sub_data[0], nbyte);
	zmq::const_buffer zmq_buffer{buffer, nbyte + 16};
	_publisher.send(zmq_buffer);
}
