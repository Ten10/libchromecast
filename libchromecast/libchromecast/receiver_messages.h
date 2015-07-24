#pragma once
#include "types.h"
#include <vector>
#include <string>
#include <memory>

namespace chromecast
{
	class ConstJsonMessagePart;
	struct BasicReceiverStatus
	{
		std::unique_ptr<bool> is_active_input;
		bool is_standby;
		bool muted;
		double volume_level;

		BasicReceiverStatus() = default;
		BasicReceiverStatus(BasicReceiverStatus&& other);
		BasicReceiverStatus& operator=(const BasicReceiverStatus& other);
		static BasicReceiverStatus FromMessage(const ConstJsonMessagePart& message);
	};

	struct ReceiverStatus : public BasicReceiverStatus
	{
		struct ApplicationInfo
		{
			std::string application_id;
			std::string display_name;
			std::vector<std::string> namespaces;
			std::string session_id;
			std::string status_text;
			std::string transport_id;

			static ApplicationInfo FromMessage(const ConstJsonMessagePart& message);
		};

		std::vector<ApplicationInfo> applications;

		ReceiverStatus() = default;
		ReceiverStatus(ReceiverStatus&& other);

		static ReceiverStatus FromMessage(const ConstJsonMessagePart& message);
	};

	struct ReceiverResponse
	{
		std::string reason;
		ReceiverStatus status;

		ReceiverResponse() = default;
		ReceiverResponse(ReceiverResponse&& other);

		static ReceiverResponse FromMessage(const ConstJsonMessagePart& message);
	};

	typedef std::pair<std::string, bool> AppAvailability;
}