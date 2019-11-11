
#pragma once

#include <vector>
#include <string>
#include <thread>
#include <queue>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <functional>
#include <Vector/DBC.h>

#include "CanBus.h"
#include "DBCSignal_Wrapper.h"


class CanSync
{
public:
	struct SubData
	{
		Signal::id_t id;
		double value;
	} __attribute__((packed));

	using unique_lock_t = std::unique_lock<std::mutex>;
	using sub_func_t = std::function<void(std::chrono::microseconds timetamp, const std::vector<SubData>&)>;

	CanSync(std::chrono::microseconds sr, std::vector<CanBus>&& can_buses);
	~CanSync();
	void start();
	void stop();
	void sub(sub_func_t&& sub_func);

private:
	void worker();
	void worker_can_read(const CanBus& can_bus);
	
	// sampling rate
	std::chrono::microseconds _sr;
	std::chrono::microseconds _next_fire;
	std::atomic<bool> _running;
	std::vector<sub_func_t> _sub_funcs;

	std::thread _th_worker;
	std::vector<std::thread> _ths_can_read;
	std::vector<CanBus> _can_buses;

	std::queue<Signal> _signal_data_queue;
	std::mutex _mx_signal_data_queue;
	std::condition_variable _cond_var_frame_recv;
	
	struct SignalFireData
	{
		SubData current{(uint64_t)-1, 0.};
		std::queue<Signal> signal_queue;
	};
	std::vector<SignalFireData> _signal_queues;
};
