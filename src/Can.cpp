
#include "Can.h"
#include <stdexcept>
#include <cstring>

Can::Can(const std::string& ifname)
{
	_socket = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (_socket < 0)
	{
		throw std::runtime_error("::socket failed");
	}
	std::memcpy(_ifr.ifr_ifrn.ifrn_name, ifname.c_str(), ifname.size() + 1);
	if (::ioctl(_socket, SIOCGIFINDEX, &_ifr) < 0)
	{
		throw std::runtime_error("::ioctl failed");
	}
	_addr.can_family = AF_CAN;
	_addr.can_ifindex = _ifr.ifr_ifru.ifru_ivalue;
	
	/* try to switch the socket into CAN FD mode */
	const int canfd_on = 1;
	::setsockopt(_socket, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &canfd_on, sizeof(canfd_on));

	const int timestamp_on = 1;
	if (::setsockopt(_socket, SOL_SOCKET, SO_TIMESTAMP, &timestamp_on, sizeof(timestamp_on)) < 0)
	{
		throw std::runtime_error("::setsockopt failed");
	}
	
	if (!bind())
	{
		throw std::runtime_error("::bind failed");
	}
	_msg.msg_name = &_addr;
	_msg.msg_control = &_ctrlmsg;
}
Can::Can(Can&& other)
{
	std::memcpy(this, &other, sizeof(Can));
	other._socket = 0;
}
Can::~Can()
{
	if (_socket)
	{
		::close(_socket);
	}
}
bool Can::bind()
{
	return ::bind(_socket, (struct sockaddr*)&_addr, sizeof(_addr)) >= 0;
}
std::optional<Can::Frame> Can::recv(std::chrono::microseconds timeout) const
{
	FD_ZERO(&_rdfs);
	FD_SET(_socket, &_rdfs);
	struct timeval tv;
	tv.tv_sec = timeout.count() / (1000 * 1000);
	tv.tv_usec = timeout.count() % (1000 * 1000);
	if (::select(_socket + 1, &_rdfs, NULL, NULL, &tv) > 0 &&
		FD_ISSET(_socket, &_rdfs))
	{
		Frame frame;
		struct iovec iov;
		_msg.msg_iov = &iov;
		_msg.msg_iovlen = 1;
		_msg.msg_namelen = sizeof(_addr);
		_msg.msg_controllen = sizeof(_ctrlmsg);
		_msg.msg_flags = 0;
		iov.iov_base = &frame.raw_frame;
		iov.iov_len = sizeof(frame.raw_frame);

		int nbytes = ::recvmsg(_socket, &_msg, 0);
		struct cmsghdr *cmsg;
		for (cmsg = CMSG_FIRSTHDR(&_msg); cmsg && (cmsg->cmsg_level == SOL_SOCKET); cmsg = CMSG_NXTHDR(&_msg, cmsg))
		{
			if (cmsg->cmsg_type == SO_TIMESTAMP)
			{
				struct timeval* ptv = (struct timeval*)CMSG_DATA(cmsg);
				frame.timestamp = std::chrono::microseconds{ptv->tv_sec * 1000 * 1000 + ptv->tv_usec};
			}
			else if (cmsg->cmsg_type == SO_TIMESTAMPING)
			{
				struct timespec *stamp = (struct timespec *)CMSG_DATA(cmsg);
				/*
					* stamp[0] is the software timestamp
					* stamp[1] is deprecated
					* stamp[2] is the raw hardware timestamp
					* See chapter 2.1.2 Receive timestamps in
					* linux/Documentation/networking/timestamping.txt
					*/
				frame.timestamp = std::chrono::microseconds{stamp[2].tv_sec * 1000 * 1000 + stamp[2].tv_nsec / 1000};
			}
			return frame;
		}
	}
	return std::nullopt;
}

