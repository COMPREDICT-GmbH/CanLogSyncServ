
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
	_addr.can_family = AF_CAN;
	std::memset(_ifr.ifr_name, 0, sizeof(_ifr.ifr_name));
	std::memcpy(_ifr.ifr_name, ifname.c_str(), ifname.size());
	if (::ioctl(_socket, SIOCGIFINDEX, &_ifr) < 0)
	{
		throw std::runtime_error("::ioctl failed");
	}
	_addr.can_ifindex = _ifr.ifr_ifindex;
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
}
Can::Can(Can&& other)
{
	std::memcpy(this, &other, sizeof(Can));
	std::memset(&other, 0, sizeof(other));
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
	return ::bind(_socket, (sockaddr*)&_addr, sizeof(_addr)) >= 0;
}
std::optional<Can::Frame> Can::recv(std::chrono::microseconds timeout) const
{
	FD_ZERO(&_rdfs);
	FD_SET(_socket, &_rdfs);
	timeval tv;
	tv.tv_sec = timeout.count() / (1000 * 1000);
	tv.tv_usec = timeout.count() % (1000 * 1000);
	if (0 < ::select(_socket + 1, &_rdfs, NULL, NULL, &tv) && FD_ISSET(_socket, &_rdfs))
	{
		_iov.iov_base = &_frame.raw_frame;
		_msg.msg_name = &_addr;
		_msg.msg_iov = &_iov;
		_msg.msg_namelen = sizeof(_addr);
		_msg.msg_controllen = sizeof(_ctrlmsg);
		_msg.msg_flags = 0;
		int nbytes = ::recvmsg(_socket, &_msg, 0);
		cmsghdr* cmsg;
		for (cmsg = CMSG_FIRSTHDR(&_msg);
			cmsg && (cmsg->cmsg_level == SOL_SOCKET);
			cmsg = CMSG_NXTHDR(&_msg, cmsg))
		{
			if (cmsg->cmsg_type == SO_TIMESTAMP)
			{
				timeval* ptv = (timeval*)CMSG_DATA(cmsg);
				_frame.timestamp = std::chrono::microseconds{ptv->tv_sec * 1000 * 1000 + ptv->tv_usec};
			}
			else if (cmsg->cmsg_type == SO_TIMESTAMPING)
			{
				timespec* stamp = (timespec *)CMSG_DATA(cmsg);
				_frame.timestamp = std::chrono::microseconds{stamp[2].tv_sec * 1000 * 1000 + stamp[2].tv_nsec / 1000};
			}
			return _frame;
		}
	}
	return std::nullopt;
}

