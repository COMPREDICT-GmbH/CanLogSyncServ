
#pragma once

#include <atomic>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include "Can.h"
#include "DBCSignal_Wrapper.h"
#include "Signal.h"

class CanBus
{
public:
	using id_t = uint64_t;
	CanBus(id_t bus_id, Can&& can, const std::vector<std::pair<canid_t, std::vector<DBCSignal_Wrapper>>>&& msgs);
	CanBus(CanBus&) = delete;
	CanBus(CanBus&& other);
	std::vector<Signal> recv(std::chrono::microseconds timeout) const;
	bool set_user_data(canid_t canid, Signal::id_t signal_id, void* user_data);
	void* get_and_unset_user_data(canid_t canid, Signal::id_t signal_id);
	std::vector<std::pair<canid_t, Signal::id_t>> canids_and_signal_ids() const;
	std::chrono::microseconds time() const;

private:
	id_t _bus_id;
	mutable Can _can;
	std::unordered_map<canid_t, std::vector<std::pair<void*, DBCSignal_Wrapper>>> _msgs;
	// time in microseconds
	mutable std::atomic<uint64_t> _time;
	//mutable std::chrono::microseconds _time;
};
