
#pragma once

#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/time.h>
#include <unistd.h>

#include <string>
#include <chrono>
#include <optional>
#include <vector>
#include <cstdint>

class Can
{
public:
	struct Frame
	{
		struct canfd_frame raw_frame;
		std::chrono::microseconds timestamp;
	};

	Can(const std::string& ifnames, const std::vector<canid_t>& filter_ids);
	Can(const Can& other) = delete;
	Can(Can&& other);
	~Can();
	std::optional<Frame> recv(std::chrono::microseconds timeout) const;
	
private:
	int _socket;
	mutable ifreq _ifr;
	mutable sockaddr_can _addr;
	mutable Frame _frame;
	mutable fd_set _rdfs;
	mutable iovec _iov;
	mutable msghdr _msg;
	mutable char _ctrlmsg[CMSG_SPACE(sizeof(timeval) + 3 * sizeof(timespec) + sizeof(__u32))];
};
