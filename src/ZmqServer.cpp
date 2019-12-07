
#include "ZmqServer.h"
#include <iostream>
#include "Signal.pb.h"

ZmqServer::ZmqServer(const std::vector<std::string>& ipc_links)
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	_context = zmq::context_t{1};
	_publisher = zmq::socket_t{_context, ZMQ_PUB};
	for (const auto& ipc_link : ipc_links)
	{
		_publisher.bind(ipc_link);
	}
}
ZmqServer::~ZmqServer()
{
}
void ZmqServer::cb_sub(std::chrono::microseconds timestamp, const std::vector<CanSync::SubData>& sub_data)
{
	CanLogSyncServ::Pb_Signals pb_sigs;
	pb_sigs.set_timestamp(timestamp.count());
	for (const auto& data : sub_data)
	{
		CanLogSyncServ::Pb_Signal* sig = pb_sigs.add_sigs();
		sig->set_id(data.id);
		sig->set_value(data.value);
	}
	std::string s_buffer;
	pb_sigs.SerializeToString(&s_buffer);
	zmq::const_buffer zmq_buffer{s_buffer.c_str(), s_buffer.size()};
	_publisher.send(zmq_buffer);
}
