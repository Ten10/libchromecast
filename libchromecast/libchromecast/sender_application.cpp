#include "sender_application.h"
#include "connection.h"
#include "utils.h"

namespace chromecast
{
	void SenderApplication::OnLaunchFailed(const std::string& error)
	{
		THROW_ON_ERROR_EX(true, error);
	}

	void SenderApplication::Initialize(ChromecastChannelFactory& channel_factory, const ReceiverStatus::ApplicationInfo& app_info, const std::function<void(bool)>& on_initialization_completed)
	{
		std::hash<std::string> hashing_function;
		std::string sender_id = "sender_" + std::to_string(hashing_function(_app_id));
		CreateChannels(channel_factory, sender_id, app_info.transport_id);
		_session_id = app_info.session_id;
		_connection->Connect();
		//_heartbeat->Start();
	}

	SenderApplication::SenderApplication(const std::string& app_id)
		: _app_id(app_id)
	{

	}

	std::string SenderApplication::GetID() const 
	{ 
		return _app_id; 
	}

	void SenderApplication::CreateChannels(ChromecastChannelFactory& channel_factory, const std::string& sender_id, const std::string& receiver_id)
	{
		_connection = channel_factory.CreateChannel<ConnectionChannel>(sender_id, receiver_id);
		//_heartbeat = channel_factory.CreateIOServiceChannel<HeartbeatChannel>(sender_id, receiver_id);
	}

	void SenderApplication::OnStopped()
	{
		//_heartbeat.reset();
		if (_connection)
		{
			_connection->Close();
			_connection.reset();
		}
	}
}