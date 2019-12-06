
#include <stdexcept>
#include <cstring>
#include "Can.h"

// just to save the numbers
// filters[0].can_id = 0x06E00800;
// filters[0].can_mask = 0xEFE08C00;

Can::Can(const std::string& name)
	: skt_{-1}
{
	std::memset(&ifr_, 0, sizeof(ifr_));
	std::memset(&addr_, 0, sizeof(addr_));
	std::memset(&iov_, 0, sizeof(iov_));
	std::memset(&msg_, 0, sizeof(msg_));
	std::memset(&frame_, 0, sizeof(frame_));
	std::memcpy(&ifr_.ifr_ifrn, name.c_str(), name.size());
	addr_.can_family = AF_CAN;
	iov_[0].iov_base = &frame_;
	msg_.msg_name = &addr_;
	msg_.msg_iov = iov_;
	msg_.msg_iovlen = 1;
	msg_.msg_control = &ctrlmsg_;
	reconnect();
}
Can::Can(Can&& other)
{
	set_ = other.set_;
	skt_ = other.skt_;
	std::memcpy(iov_, other.iov_, sizeof(iov_));
	frame_ = other.frame_;
	msg_ = other.msg_;
	addr_ = other.addr_;
	ifr_ = other.ifr_;
	tv_ = other.tv_;
	std::memset(&other, 0, sizeof(Can));
	FD_ZERO(&other.set_);
	other.skt_ = -1;
	addr_.can_family = AF_CAN;
	iov_[0].iov_base = &frame_;
	msg_.msg_name = &addr_;
	msg_.msg_iov = iov_;
	msg_.msg_iovlen = 1;
	msg_.msg_control = &ctrlmsg_;
	//reconnect();
}
Can::~Can()
{
	if (skt_ >= 0)
	{
		::close(skt_);
	}
}
void Can::reconnect()
{
	if (skt_ >= 0)
	{
		::close(skt_);
	}
	skt_ = ::socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if (skt_ == -1)
	{
		throw std::runtime_error{std::string{"socket failed"}};
	}
	if (::ioctl(skt_, SIOCGIFINDEX, &ifr_) == -1)
	{
		throw std::runtime_error{std::string{"ioctl failed"}};
	}
	addr_.can_ifindex = ifr_.ifr_ifindex;
	// try to switch into FD mode
	const int32_t canfd_on = 1;
	::setsockopt(skt_, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &canfd_on, sizeof(canfd_on));
	const int32_t timestamp_on = 1;
	if (::setsockopt(skt_, SOL_SOCKET, SO_TIMESTAMP, &timestamp_on, sizeof(timestamp_on)) < 0)
	{
		throw std::runtime_error{std::string{"setsockopt failed"}};
	}
	if (::bind(skt_, (sockaddr*)&addr_, sizeof(addr_)) == -1)
	{
		throw std::runtime_error{std::string{"bind failed"}};
	}
	FD_ZERO(&set_); /* clear the set */
	FD_SET(skt_, &set_); /* add our file descriptor to the set */
}
std::optional<Can::Frame> Can::recv(std::chrono::microseconds duration)
{
	std::optional<Can::Frame> result{std::nullopt};
	timeval timeout;
	timeout.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
	timeout.tv_usec = (duration - std::chrono::duration_cast<std::chrono::seconds>(duration)).count();
	FD_ZERO(&set_);
	FD_SET(skt_, &set_);
	int32_t rv = ::select(skt_ + 1, &set_, NULL, NULL, &timeout);
	if (0 < rv)
	{
		iov_[0].iov_len = sizeof(frame_);
		msg_.msg_namelen = sizeof(addr_);
		msg_.msg_controllen = sizeof(ctrlmsg_);  
		msg_.msg_flags = 0;
		int32_t nbytes = recvmsg(skt_, &msg_, 0);
		if (nbytes == -1)
		{
			throw std::runtime_error{"recvmsg returned -1"};
		}
		else if (0 < nbytes)
		{
			for (cmsghdr* cmsg = CMSG_FIRSTHDR(&msg_); cmsg && (cmsg->cmsg_level == SOL_SOCKET); cmsg = CMSG_NXTHDR(&msg_, cmsg))
			{
				if (cmsg->cmsg_type == SO_TIMESTAMP)
				{
					memcpy(&tv_, CMSG_DATA(cmsg), sizeof(tv_));
				}
				else if (cmsg->cmsg_type == SO_TIMESTAMPING)
				{
					timespec* stamp = (timespec*)CMSG_DATA(cmsg);
					/*
					 * stamp[0] is the software timestamp
					 * stamp[1] is deprecated
					 * stamp[2] is the raw hardware timestamp
					 * See chapter 2.1.2 Receive timestamps in
					 * linux/Documentation/networking/timestamping.txt
					 */
					tv_.tv_sec = stamp[2].tv_sec;
					tv_.tv_usec = stamp[2].tv_nsec / 1000;
				}
			}
			uint64_t timestamp = tv_.tv_sec * 1000 + tv_.tv_usec / 1000;
			std::chrono::seconds tv_sec{tv_.tv_sec};
			std::chrono::microseconds tv_usec{tv_.tv_usec};
			Frame frame;
			frame.raw_frame = frame_;
			frame.timestamp = tv_usec + tv_sec;
			result = std::optional<Frame>{frame};
		}
	}
	else if (rv == 0)
	{
		//reconnect();
	}
	return result;
}
void Can::set_filters(const std::vector<canid_t>& ids)
{
	::can_filter filters[ids.size()];
	for (std::size_t i = 0; i < ids.size(); i++)
	{
		filters[i].can_id = ids[i];
		filters[i].can_mask = CAN_EFF_FLAG | CAN_RTR_FLAG | CAN_EFF_MASK;
	}
	if (::setsockopt(skt_, SOL_CAN_RAW, CAN_RAW_FILTER, &filters[0], sizeof(::can_filter) * ids.size()) == -1)
	{
		throw std::runtime_error{std::string{"setsockopt failed"}};
	}
}
