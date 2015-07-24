#pragma once
#include "channel.h"
#include "sender_application.h"
#include "receiver_messages.h"

#include <map>
#include <functional>
#include <boost\asio\deadline_timer.hpp>

namespace chromecast
{
	class JsonMessage;
	class RequestChannel : public ChromecastChannel
	{
	public:
		typedef std::function<void(const JsonMessage&)> ResonseCallback;
	private:
		static uint64_t _request_id;
		struct ChannelRequest
		{
			std::unique_ptr<boost::asio::deadline_timer> _retry_timer;
			ResonseCallback _callback;

			ChannelRequest() = default;
			ChannelRequest(boost::asio::io_service& io_service, const ResonseCallback& callback);
			ChannelRequest(ChannelRequest&& other);
			ChannelRequest& operator=(ChannelRequest&& other);
			void OnRequestCompleted(const JsonMessage& message);
		};

		boost::asio::io_service& _io_service;
		std::map<uint64_t, ChannelRequest> _request_id_to_request;

		bool OnMessage(const std::string& message) override;
	protected:
		RequestChannel(boost::asio::io_service& io_service, ChromecastConnection& connection, const ChromecastChannel::Address& address);

		template <typename TResponse>
		void Request(JsonMessage&& message, const std::function<void(const TResponse&)>& callback)
		{
			Request(move(message), [=](const JsonMessage& message)
			{
				if (callback)
					callback(TResponse::FromMessage(message));
			});
		}


		void Request(JsonMessage&& message, const ResonseCallback& callback, uint32_t request_retry_interval_seconds = 5);
		virtual bool OnResponse(uint64_t request_id, const JsonMessage& message);
	};

	class ReceiverChannel : public RequestChannel
	{
	public:
		typedef std::function<void(bool)> OperationCompletedCallback;
		typedef std::function<void(const ReceiverStatus&)> ReceiverStatusCallback;
		typedef std::function<void(const std::vector<AppAvailability>&)> AppAvailabilityCallback;
	private:
		ReceiverStatus _last_full_status;
		bool _launching_application;
		OperationCompletedCallback _on_application_launched;
		std::shared_ptr<SenderApplication> _application;
		
		bool OnResponse(uint64_t request_id, const JsonMessage& message) override;

		friend class ReceiverMessage;
	public:
		ReceiverChannel(boost::asio::io_service& io_service, ChromecastConnection& connection, const std::string& sender, const std::string& receiver);

		void GetAppAvailabillity(const std::vector<std::string>& app_ids, const AppAvailabilityCallback& callback);
		void GetStatus(const ReceiverStatusCallback& callback);
		
		virtual void OnReceiverStatus(const ReceiverStatus& status);
		void Launch(std::shared_ptr<SenderApplication> application, const OperationCompletedCallback& callback);
		void Join(std::shared_ptr<SenderApplication> application, const OperationCompletedCallback& callback);
		void Mute(bool mute, const OperationCompletedCallback& callback);
		void SetVolume(double volume_level, const OperationCompletedCallback& callback);
	};
}