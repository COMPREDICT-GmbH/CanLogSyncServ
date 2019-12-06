
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

	Can(const std::string& name);
	Can(const Can&) = delete;
	const Can& operator=(const Can&) = delete;
	Can(Can&& other);
	Can& operator=(Can&& other);
	virtual ~Can();
	
	std::optional<Frame> recv(std::chrono::microseconds duration);
	void set_filters(const std::vector<canid_t>& id);
	
private:
	void reconnect();
	fd_set set_;
	int32_t skt_;
	iovec iov_[1];
	canfd_frame frame_;
	msghdr msg_;
	char ctrlmsg_[CMSG_SPACE(sizeof(timeval) + 3 * sizeof(timespec) + sizeof(uint32_t))];
	sockaddr_can addr_;
	ifreq ifr_;
	timeval tv_;
};

