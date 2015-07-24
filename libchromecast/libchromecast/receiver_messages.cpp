#include "receiver_messages.h"
#include "json_message.h"
#include "utils.h"

namespace chromecast
{
	using namespace rapidjson;

	static const std::string k_receiver_status = "RECEIVER_STATUS";
	static const std::string k_invalid_request = "INVALID_REQUEST";

	BasicReceiverStatus::BasicReceiverStatus(BasicReceiverStatus&& other)
	{
		muted = other.muted;
		volume_level = other.volume_level;
		is_standby = other.is_standby;
		is_active_input = move(other.is_active_input);
	}

	BasicReceiverStatus& BasicReceiverStatus::operator=(const BasicReceiverStatus& other)
	{
		muted = other.muted;
		volume_level = other.volume_level;
		is_standby = other.is_standby;
		if (other.is_active_input)
			is_active_input = std::make_unique<bool>(*other.is_active_input);
		return *this;
	}

	BasicReceiverStatus BasicReceiverStatus::FromMessage(const ConstJsonMessagePart& message)
	{
		BasicReceiverStatus status;
		auto& volume = message["volume"];
		status.muted = volume["muted"].GetBool();
		status.volume_level = volume["level"].GetDouble();

		status.is_standby = message["isStandBy"].GetBool();
		if (message.HasMember("isActiveInput"))
			status.is_active_input = std::make_unique<bool>(message["isActiveInput"].GetBool());
		return status;
	}

	ReceiverStatus::ApplicationInfo ReceiverStatus::ApplicationInfo::FromMessage(const ConstJsonMessagePart& message)
	{
		ApplicationInfo app_info;
		app_info.application_id = message["appId"].GetString();
		app_info.display_name = message["displayName"].GetString();
		app_info.session_id = message["sessionId"].GetString();
		app_info.status_text = message["statusText"].GetString();
		if (message.HasMember("transportId"))
			app_info.transport_id = message["transportId"].GetString();
		auto& namespaces_value = message["namespaces"];
		size_t index = 0;
		app_info.namespaces.resize(namespaces_value.Size());
		for (std::string& namespace_id : app_info.namespaces)
			namespace_id = namespaces_value[index++]["name"].GetString();
		return app_info;
	}

	ReceiverStatus::ReceiverStatus(ReceiverStatus&& other)
	{
		static_cast<BasicReceiverStatus&>(*this) = std::move(other);
		applications = move(other.applications);
	}

	ReceiverStatus ReceiverStatus::FromMessage(const ConstJsonMessagePart& message)
	{
		std::string type = message["type"].GetString();
		THROW_ON_ERROR_EX(type != k_receiver_status, "invalid message type, expected a receiver status message instead of the following message: " + message.ToString());

		auto& status_message = message["status"];
		ReceiverStatus status;
		static_cast<BasicReceiverStatus&>(status) = BasicReceiverStatus::FromMessage(status_message);
		if (status_message.HasMember("applications"))
		{
			auto& applications_value = status_message["applications"];
			size_t index = 0;
			status.applications.resize(applications_value.Size());
			for (auto& app : status.applications)
				app = ApplicationInfo::FromMessage(applications_value[index]);
		}
		return status;
	}

	ReceiverResponse::ReceiverResponse(ReceiverResponse&& other)
		: status(std::move(other.status)),
		reason(move(other.reason))
	{

	}

	ReceiverResponse FromMessage(const ConstJsonMessagePart& message)
	{
		ReceiverResponse response;
		std::string type = message["type"].GetString();
		if (type == k_receiver_status)
			response.status = ReceiverStatus::FromMessage(message);
		else if (type == k_invalid_request)
			response.reason = message["reason"].GetString();
		else
			THROW_ON_ERROR_EX(true, "unrecognized receiver response: " + message.ToString());
		return response;
	}
}