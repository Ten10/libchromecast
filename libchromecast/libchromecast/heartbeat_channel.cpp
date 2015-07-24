#include "heartbeat_channel.h"
#include "json_message.h"
#include "connection.h"
#include "utils.h"

namespace chromecast
{
	using namespace std;

	static const std::string k_heartbeat_namespace = "urn:x-cast:com.google.cast.tp.heartbeat";

	void HeartbeatChannel::StartSendTimer()
	{
		_heartbeat_send_timer.expires_from_now(boost::posix_time::seconds(send_interval_in_seconds));
		_heartbeat_send_timer.async_wait([=](boost::system::error_code error)
		{
			THROW_ON_ERROR(error);

			JsonMessage message;
			message["type"] = "PING";
			Send(message.ToString());
			Start();
		});
	}

	void HeartbeatChannel::StartReceiveTimer()
	{
		_heartbeat_receive_timer.cancel();
		_heartbeat_receive_timer.expires_from_now(boost::posix_time::seconds(receive_timeout_in_seconds));
		_heartbeat_receive_timer.async_wait([=](boost::system::error_code error)
		{
			if (!error)
				_connection.Close();
		});
	}

	HeartbeatChannel::HeartbeatChannel(boost::asio::io_service& io_service, ChromecastConnection& connection, const std::string& sender, const std::string& receiver)
		: ChromecastChannel(connection, ChromecastChannel::Address(sender, receiver, k_heartbeat_namespace)),
		_heartbeat_send_timer(io_service),
		_heartbeat_receive_timer(io_service)
	{

	}

	void HeartbeatChannel::Start()
	{
		StartSendTimer();
		StartReceiveTimer();
	}

	bool HeartbeatChannel::OnMessage(const std::string& message)
	{
		JsonMessage json_message;
		json_message.Parse(message);

		std::string type = json_message["type"].GetString();
		if (type == "PONG")
			return true;
		if (type != "PING")
			return false;

		JsonMessage pong_message;
		pong_message["type"] = "PONG";
		Send(pong_message.ToString());

		StartReceiveTimer();
		return true;
	}
}