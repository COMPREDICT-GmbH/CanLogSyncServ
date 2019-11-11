
#include "CanBus.h"

CanBus::CanBus(id_t bus_id, Can&& can, const std::vector<std::pair<canid_t, std::vector<DBCSignal_Wrapper>>>&& msgs)
		: _bus_id{bus_id}
		, _can{std::move(can)}
		, _time{0}
{
	for (const auto& msg : msgs)
	{
		std::vector<std::pair<void*, DBCSignal_Wrapper>> wrappers;
		for (const auto& sig_wrapper : msg.second)
		{
			wrappers.push_back(std::make_pair(nullptr, std::move(sig_wrapper)));
		}
		_msgs.insert(std::make_pair(msg.first, std::move(wrappers)));
	}
}
std::vector<Signal> CanBus::recv(std::chrono::microseconds timeout) const
{
	std::vector<Signal> result;
	auto frame = _can.recv(timeout);
	if (frame)
	{
		_time = frame->timestamp;
		auto iter_signals = _msgs.find(frame->raw_frame.can_id);
		if (iter_signals != _msgs.end())
		{
			for (const auto& signal : iter_signals->second)
			{
				const auto& raw = frame->raw_frame;
				Signal sig;
				sig.id = signal.second.id;
				std::vector<uint8_t> data{
					raw.data[0], raw.data[1], raw.data[2], raw.data[3],
					raw.data[4], raw.data[5], raw.data[6], raw.data[7]};
				sig.value = const_cast<Vector::DBC::Signal&>(signal.second.dbc_signal).decode(data);
				sig.value = const_cast<Vector::DBC::Signal&>(signal.second.dbc_signal).rawToPhysicalValue(sig.value);
				sig.dbc_signal = &signal.second.dbc_signal;
				sig.timestamp = frame->timestamp;
				sig.bus_id = _bus_id;
				sig.user_data = signal.first;
				result.push_back(sig);
			}
		}
	}
	return result;
}
bool CanBus::set_user_data(canid_t canid, Signal::id_t signal_id, void* user_data)
{
	auto msg_iter = _msgs.find(canid);
	if (msg_iter != _msgs.end())
	{
		auto sig_iter = std::find_if(msg_iter->second.begin(), msg_iter->second.end(),
			[signal_id](const auto& sig) { return sig.second.id == signal_id; });
		if (sig_iter != msg_iter->second.end())
		{
			sig_iter->first = user_data;
			return true;
		}
	}
	return false;
}
void* CanBus::get_and_unset_user_data(canid_t canid, Signal::id_t signal_id)
{
	void* result = nullptr;
	auto msg_iter = _msgs.find(canid);
	if (msg_iter != _msgs.end())
	{
		auto sig_iter = std::find_if(msg_iter->second.begin(), msg_iter->second.end(),
			[signal_id](const auto& sig) { return sig.second.id == signal_id; });
		if (sig_iter != msg_iter->second.end())
		{
			result = sig_iter->first;
			sig_iter->first = nullptr;
		}
	}
	return result;
}
std::vector<std::pair<canid_t, Signal::id_t>> CanBus::canids_and_signal_ids() const
{
	std::vector<std::pair<canid_t, Signal::id_t>> result;
	for (const auto& msg : _msgs)
	{
		for (const auto& sig : msg.second)
		{
			result.push_back(std::make_pair(msg.first, sig.second.id));
		}
	}
	return result;
}
std::chrono::microseconds CanBus::time() const
{
	return _time;
}
