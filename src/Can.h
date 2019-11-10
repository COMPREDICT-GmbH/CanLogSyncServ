
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

	Can(const std::string& ifnames);
	Can(const Can& other) = delete;
	Can(Can&& other);
	~Can();
	bool bind();
	std::optional<Frame> recv(std::chrono::microseconds timeout) const;
	
private:
	int _socket;
	struct sockaddr_can _addr;
	struct ifreq _ifr;
	mutable fd_set _rdfs;
	mutable struct msghdr _msg;
	char _ctrlmsg[CMSG_SPACE(sizeof(struct timeval) + 3 * sizeof(struct timespec) + sizeof(__u32))];
};
