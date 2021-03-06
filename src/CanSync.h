
#pragma once

#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <queue>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <functional>

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
    class Subscriber
    {
    public:
        virtual ~Subscriber() = default;
        virtual void update(std::chrono::microseconds timestamp, const std::vector<SubData>& data) = 0;
    };

    using unique_lock_t = std::unique_lock<std::mutex>;

    CanSync(std::chrono::microseconds sample_rate, std::vector<CanBus>&& can_buses, std::chrono::milliseconds can_timeout);
    ~CanSync();
    bool running() const;
    void start();
    void stop();
    void subscribe(std::unique_ptr<Subscriber>&& sub);

private:
    void worker();
    void worker_can_read(const CanBus& can_bus);
    
    // sampling rate
    std::chrono::microseconds _sample_rate;
    std::chrono::microseconds _next_fire;
    std::atomic<bool> _running;
    std::vector<std::unique_ptr<Subscriber>> _subscribers;

    std::thread _th_worker;
    std::vector<std::thread> _ths_can_read;
    std::vector<CanBus> _can_buses;

    std::queue<Signal> _signal_data_queue;
    std::mutex _mx_signal_data_queue;
    std::condition_variable _cond_var_frame_recv;
    
    struct SignalFireData
    {
        CanBus::id_t busid;
        SubData current{(uint64_t)-1, 0.};
        std::queue<Signal> signal_queue;
    };
    std::vector<SignalFireData> _signal_queues;
    std::chrono::milliseconds _can_timeout;
};
