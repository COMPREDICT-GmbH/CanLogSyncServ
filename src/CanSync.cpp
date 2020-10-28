
#include <iostream>
#include <assert.h>
#include "CanSync.h"

CanSync::CanSync(std::chrono::microseconds sample_rate, std::vector<CanBus>&& can_buses)
	: _sample_rate{sample_rate}
	, _running{false}
	, _can_buses{std::move(can_buses)}
{
}
CanSync::~CanSync()
{
	stop();
}
bool CanSync::running() const
{
	return _running;
}
void CanSync::start()
{
	if (!_running)
	{
		std::size_t num_signals = 0;
		for (auto& can_bus : _can_buses)
		{
			const auto& canids_and_signal_ids = can_bus.canids_and_signal_ids();
			num_signals += canids_and_signal_ids.size();
		}
		_signal_queues.reserve(num_signals);
		for (auto& can_bus : _can_buses)
		{
			const auto& canids_and_signal_ids = can_bus.canids_and_signal_ids();
			for (const auto& cs_id : canids_and_signal_ids)
			{
				SignalFireData signal_fire_data;
				signal_fire_data.current.id = cs_id.second;
				signal_fire_data.current.value = 1'000'000'000.;
				_signal_queues.push_back(signal_fire_data);
				void* user_data = reinterpret_cast<void*>(&_signal_queues.back());
				can_bus.set_user_data(cs_id.first, cs_id.second, user_data);
			}
		}
		_next_fire = std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::system_clock::now().time_since_epoch()) + _sample_rate;
		_running = true;
		_th_worker = std::thread{
			[this]()
			{
				this->worker();
			}};
		for (const auto& can_bus : _can_buses)
		{
			_ths_can_read.emplace_back(
				[this, &can_bus]()
				{
					this->worker_can_read(can_bus);
				});
		}
	}
}
void CanSync::stop()
{
	if (_running)
	{
		_running = false;
		_th_worker.join();
		for (auto& th : _ths_can_read)
		{
			th.join();
		}
		for (auto& can_bus : _can_buses)
		{
			const auto& canids_and_signal_ids = can_bus.canids_and_signal_ids();
			for (const auto& cs_id : canids_and_signal_ids)
			{
				can_bus.get_and_unset_user_data(cs_id.first, cs_id.second);
			}
		}
		_signal_queues.clear();
		_ths_can_read.clear();
	}
}
void CanSync::subscribe(std::unique_ptr<CanSync::Subscriber>&& sub)
{
	_subscribers.push_back(std::move(sub));
}
void CanSync::worker()
{
	std::unordered_map<uint64_t, std::chrono::microseconds> bus_times;
	for (const auto& bus : _can_buses)
	{
		bus_times[bus.id()] = _next_fire - _sample_rate;
	}
	while (_running)
	{
		{
			// enter critical section and poll data until no data left
			unique_lock_t lock{_mx_signal_data_queue};
			bool new_data = _cond_var_frame_recv.wait_for(
				lock, std::chrono::milliseconds{100},
				[this, &bus_times]()
				{
					auto now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
					for (const auto& bt : bus_times)
					{
						if (bt.second + std::chrono::seconds(5) < now)
						{
							throw std::runtime_error("CanSync: Bus timeout! Did not receive a CAN frame for 5 secs!");
						}
					}
					return _signal_data_queue.size() > 0;
				});
			if (new_data)
			{
				while (_signal_data_queue.size())
				{
					Signal& signal_data = _signal_data_queue.front();
					SignalFireData* signal_fire_data =
						reinterpret_cast<SignalFireData*>(signal_data.user_data);
					if (signal_fire_data)
					{
						auto& signal_queue = signal_fire_data->signal_queue;
						signal_queue.push(signal_data);
					}
					bus_times[signal_data.bus_id] = signal_data.timestamp;
					_signal_data_queue.pop();
				}
			}
		}
		for (auto& bt : bus_times)
		{
			if (200000 < std::abs(int64_t(bt.second.count()) - int64_t(_next_fire.count())))
			{
				std::cout << "CanLogSyncServ: Timejump detected! Now I will correct the timejump!" << std::endl;
				_next_fire = bt.second;
				for (auto& bt2 : bus_times)
				{
					bt2.second = bt.second;
				}
				break;
			}
		}
		auto pull_data_to_current =
			[this]()
			{
				auto in_interval =
					[this](std::chrono::microseconds timestamp)
					{
						return (timestamp - _next_fire).count() < 0;
					};
				for (SignalFireData& signal_fire_data : _signal_queues)
				{
					while (signal_fire_data.signal_queue.size() &&
						in_interval(signal_fire_data.signal_queue.front().timestamp))
					{
						const auto& sig = signal_fire_data.signal_queue.front();
						signal_fire_data.current.id = sig.id;
						signal_fire_data.current.value = sig.value;
						signal_fire_data.signal_queue.pop();
					}
				}
			};
		auto fire =
			[this, &bus_times]()
			{
				for (const auto& bus_time : bus_times)
				{
					if (bus_time.second < _next_fire)
					{
						return false;
					}
				}
				return true;
			};
		while (fire())
		{
			pull_data_to_current();
			std::vector<SubData> sub_data;
			sub_data.reserve(_signal_queues.size());
			for (auto& signal_fire_data : _signal_queues)
			{
				sub_data.push_back(signal_fire_data.current);
			}
			for (auto& sub : _subscribers)
			{
				sub->update(_next_fire, sub_data);
			}
			_next_fire += _sample_rate;
		}
	}
}
void CanSync::worker_can_read(const CanBus& can_bus)
{
	while (_running)
	{
		const auto& signals = can_bus.recv(std::chrono::milliseconds{500});
		if (signals.size())
		{
			unique_lock_t lock{_mx_signal_data_queue};
			for (const auto& signal : signals)
			{
				_signal_data_queue.push(signal);
			}
			_cond_var_frame_recv.notify_one();
		}
	}
}
