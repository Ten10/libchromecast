#pragma once
#include "heartbeat_channel.h"
#include "connection_channel.h"
#include "receiver_messages.h"
#include <string>

namespace chromecast
{
	class ChromecastChannelFactory;
	class SenderApplication
	{
	protected:
		std::string _app_id;
		std::string _session_id;
		//std::unique_ptr<HeartbeatChannel> _heartbeat;
		std::unique_ptr<ConnectionChannel> _connection;

	public:
		SenderApplication(const std::string& app_id);

		std::string GetID() const;
		virtual void OnLaunchFailed(const std::string& error);
		virtual void Initialize(ChromecastChannelFactory& channel_factory, const ReceiverStatus::ApplicationInfo& app_info, const std::function<void(bool)>& on_initialization_completed);
		virtual void CreateChannels(ChromecastChannelFactory& channel_factory, const std::string& sender_id, const std::string& receiver_id);
		virtual void OnStopped();
	};
}