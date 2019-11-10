
#include "ZmqServer.h"

ZmqServer::ZmqServer(const std::vector<std::string>& ipc_links)
{
	_context = zmq::context_t{1};
	_publisher = zmq::socket_t{_context, ZMQ_PUB};
	for (const auto& ipc_link : ipc_links)
	{
		_publisher.bind(ipc_link);
	}
}
void ZmqServer::cb_sub(const std::vector<CanSync::SubData>& sub_data)
{
	std::size_t nbyte = sub_data.size() * sizeof(CanSync::SubData);
	zmq::const_buffer buffer{(void*)&sub_data[0], nbyte};
	_publisher.send(buffer);
}
