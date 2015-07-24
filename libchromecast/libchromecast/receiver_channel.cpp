#include "receiver_channel.h"
#include "json_message.h"
#include "connection.h"
#include "utils.h"

namespace chromecast
{
	using namespace std;

	static const std::string k_receiver_namespace = "urn:x-cast:com.google.cast.receiver";

	uint64_t RequestChannel::_request_id = 0;

	RequestChannel::ChannelRequest::ChannelRequest(boost::asio::io_service& io_service, const RequestChannel::ResonseCallback& callback)
		: _callback(callback)
	{
		_retry_timer = make_unique<boost::asio::deadline_timer>(io_service);
	}

	RequestChannel::ChannelRequest::ChannelRequest(ChannelRequest&& other)
		: _retry_timer(move(other._retry_timer)),
		_callback(move(other._callback))
	{

	}

	RequestChannel::ChannelRequest& RequestChannel::ChannelRequest::operator=(ChannelRequest&& other)
	{
		_retry_timer = move(other._retry_timer);
		_callback = move(other._callback);
		return *this;
	}

	void RequestChannel::ChannelRequest::OnRequestCompleted(const JsonMessage& message)
	{
		_retry_timer->cancel();
		if (_callback)
			_callback(message);
	}

	bool RequestChannel::OnMessage(const std::string& message)
	{
		JsonMessage json_message;
		json_message.Parse(message);
		uint64_t request_id = 0;
		std::string type;
		try
		{
			request_id = json_message["requestId"].GetUint64();
			type = json_message["type"].GetString();
		}
		catch (std::runtime_error&)
		{
			return false;
		}

		THROW_ON_ERROR_EX(type == "INVALID_REQUEST", "invalid request, " + json_message["reason"].GetString());

		return OnResponse(request_id, json_message);
	}

	RequestChannel::RequestChannel(boost::asio::io_service& io_service, ChromecastConnection& connection, const ChromecastChannel::Address& address)
		: ChromecastChannel(connection, address),
		_io_service(io_service)
	{

	}

	void RequestChannel::Request(JsonMessage&& message, const ResonseCallback& callback, uint32_t request_retry_interval_seconds)
	{
		ChannelRequest request(_io_service, callback);
		uint64_t request_id = ++_request_id;
		message["requestId"] = request_id;
		request._retry_timer->expires_from_now(boost::posix_time::seconds(request_retry_interval_seconds));
		request._retry_timer->async_wait([=](boost::system::error_code error)
		{
			if (error)
				return;

			//copy ctor, high usage cost.
			auto new_message = message;
			Request(move(new_message), callback);
		});
		_request_id_to_request[request_id] = move(request);

		Send(message.ToString());
	}

	bool RequestChannel::OnResponse(uint64_t request_id, const JsonMessage& message)
	{
		auto it = _request_id_to_request.find(request_id);
		if (it == _request_id_to_request.end())
			return false;

		auto request = move(it->second);
		_request_id_to_request.erase(it);
		request.OnRequestCompleted(message);
		return true;
	}

	bool ReceiverChannel::OnResponse(uint64_t request_id, const JsonMessage& message)
	{
		if (request_id != 0)
			return __super::OnResponse(request_id, message);

		OnReceiverStatus(ReceiverStatus::FromMessage(message));
		return true;
	}

	ReceiverChannel::ReceiverChannel(boost::asio::io_service& io_service, ChromecastConnection& connection, const std::string& sender, const std::string& receiver)
		: RequestChannel(io_service, connection, ChromecastChannel::Address(sender, receiver, k_receiver_namespace))
		
	{

	}

	void ReceiverChannel::GetAppAvailabillity(const std::vector<std::string>& app_ids, const AppAvailabilityCallback& callback)
	{
		THROW_ON_ERROR_EX(app_ids.empty(), "empty application id array");

		JsonMessage message;
		message["type"] = "GET_APP_AVAILABILITY";
		message["appId"] = app_ids;
		Request(move(message), [=](const JsonMessage& message)
		{
			std::vector<AppAvailability> result;
			auto& availability = message["availability"];
			for (const std::string& app_id : app_ids)
			{
				bool available = availability[app_id.c_str()].GetString() == "APP_AVAILABLE";
				result.emplace_back(app_id, available);
			}
			if (callback)
				callback(result);
		});
	}

	void ReceiverChannel::GetStatus(const ReceiverChannel::ReceiverStatusCallback& callback)
	{
		JsonMessage message;
		message["type"] = "GET_STATUS";
		Request<ReceiverStatus>(move(message), [=](const ReceiverStatus& status)
		{
			_last_full_status = status;
			if (callback)
				callback(status);
		});
	}

	void ReceiverChannel::OnReceiverStatus(const ReceiverStatus& status)
	{
		_last_full_status = status;
		if (!_application || _last_full_status.applications.empty() || !_launching_application)
			return;

		for (auto& app_info : _last_full_status.applications)
		{
			std::string app_id = _application->GetID();
			if (app_info.application_id == app_id || app_info.display_name.find(app_id) != std::string::npos)
			{
				_launching_application = false;
				_application->Initialize(_connection.channel_factory, app_info, [=](bool initialized)
				{
					if (_on_application_launched)
						_on_application_launched(initialized);
				});
				break;
			}
		}
	}

	void ReceiverChannel::Launch(std::shared_ptr<SenderApplication> application, const OperationCompletedCallback& callback)
	{
		THROW_ON_ERROR_EX(!application, "invalid application");
		if (_application)
			_application->OnStopped();
		_application = application;
		_on_application_launched = callback;

		JsonMessage message;
		message["type"] = "LAUNCH";
		message["appId"] = _application->GetID();
		Request(move(message), nullptr);
		_launching_application = true;
	}

	void ReceiverChannel::Join(std::shared_ptr<SenderApplication> application, const OperationCompletedCallback& callback)
	{
		if (!callback)
			throw std::runtime_error("The join function requires a valid callback to be provided.");

		GetStatus([=](const ReceiverStatus& status)
		{
			if (!application)
				return callback(false);

			if (_application)
				return callback(false);

			auto it = std::find_if(status.applications.begin(), status.applications.end(), [=](const ReceiverStatus::ApplicationInfo& app_info)
			{
				return app_info.application_id == application->GetID();
			});

			if (it == status.applications.end())
				return callback(false);

			_application = application;
			_application->Initialize(_connection.channel_factory, *it, callback);
		});
	}

	void ReceiverChannel::Mute(bool mute, const OperationCompletedCallback& callback)
	{
		JsonMessage message;
		message["type"] = "SET_VOLUME";
		message["volume"]["muted"] = mute;
		Request<ReceiverStatus>(move(message), [=](const ReceiverStatus& status)
		{
			if (callback)
				callback(status.muted);
		});
	}

	void ReceiverChannel::SetVolume(double volume_level, const OperationCompletedCallback& callback)
	{
		JsonMessage message;
		message["type"] = "SET_VOLUME";
		message["volume"]["level"] = volume_level;
		Request<ReceiverStatus>(move(message), [=](const ReceiverStatus& status)
		{
			if (callback)
				callback(fabs(volume_level - status.volume_level) < 0.01);
		});
	}
}