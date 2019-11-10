
#pragma once

#include <unordered_map>
#include <Vector/DBC.h>
#include <vector>
#include <algorithm>
#include "Can.h"
#include "DBCSignal_Wrapper.h"
#include "Signal.h"

class CanBus
{
public:
	using id_t = uint64_t;
	CanBus(id_t bus_id, Can&& can, const std::vector<std::pair<canid_t, std::vector<DBCSignal_Wrapper>>>&& msgs);
	std::vector<Signal> recv(std::chrono::microseconds timeout) const;
	bool set_user_data(canid_t canid, Signal::id_t signal_id, void* user_data);
	void* get_and_unset_user_data(canid_t canid, Signal::id_t signal_id);
	std::vector<std::pair<canid_t, Signal::id_t>> canids_and_signal_ids() const;
	std::chrono::microseconds time() const;

private:
	id_t _bus_id;
	Can _can;
	std::unordered_map<canid_t, std::vector<std::pair<void*, DBCSignal_Wrapper>>> _msgs;
	mutable std::chrono::microseconds _time;
};
