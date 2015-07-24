#pragma once
#include "channel.h"
#include <boost\asio\deadline_timer.hpp>

namespace chromecast
{
	class HeartbeatChannel : public ChromecastChannel
	{
		static const byte send_interval_in_seconds = 5;
		static const byte receive_timeout_in_seconds = 30;
		boost::asio::deadline_timer _heartbeat_send_timer;
		boost::asio::deadline_timer _heartbeat_receive_timer;

		void StartSendTimer();
		void StartReceiveTimer();
	public:
		HeartbeatChannel(boost::asio::io_service& io_service, ChromecastConnection& connection, const std::string& sender, const std::string& receiver);

		void Start();
		bool OnMessage(const std::string& message) override;
	};
}