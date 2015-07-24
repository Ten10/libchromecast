#pragma once
#include "utils.h"
#include "connection.h"
#include "connection_channel.h"
#include "heartbeat_channel.h"
#include "receiver_channel.h"

namespace chromecast
{
	template <typename TConnection = ChromecastConnection, typename TReceiverChannel = ReceiverChannel, typename TConnectionChannel = ConnectionChannel, typename THeartbeatChannel = HeartbeatChannel>
	class ChromecastClient
	{
	protected:
		TConnection _connection;
		TReceiverChannel _receiver_channel;
		THeartbeatChannel _heartbeat_channel;
		TConnectionChannel _connection_channel;

		static const std::string k_sender0;
		static const std::string k_receiver0;
	public:
		typedef std::function<void(bool)> ConnectedCallback;

		ChromecastClient(boost::asio::io_service& io_service)
			: _connection(io_service),
			_heartbeat_channel(io_service, _connection, k_sender0, k_receiver0),
			_receiver_channel(io_service, _connection, k_sender0, k_receiver0),
			_connection_channel(_connection, k_sender0, k_receiver0)
		{
		}

		void AsyncConnect(std::string device_ip, const ConnectedCallback& callback)
		{
			static uint16_t k_chromecast_port = 8009;
			_connection.AsyncConnect(device_ip, k_chromecast_port,
				[=](const boost::system::error_code& error)
			{
				if (!error)
				{
					_connection_channel.Connect();
					_heartbeat_channel.Start();
				}
				if (callback)
					callback(!error);
			});
		}

		void Close()
		{
			_connection_channel.Close();
			_connection.Close();
		}

		template <typename TApplication>
		void Launch(const std::function<void(TApplication&)>& callback)
		{
			auto application = std::make_shared<TApplication>();
			Launch(application, [=](bool launched)
			{
				THROW_ON_ERROR_EX(!launched, "failed to launch application");
				if (callback)
					callback(static_cast<TApplication&>(*application));
			});
		}

		template <typename TApplication>
		void Join(const std::function<void(TApplication&)>& callback)
		{
			auto application = std::make_shared<TApplication>();
			Join(application, [=](bool joined)
			{
				THROW_ON_ERROR_EX(!joined, "failed to join application");
				if (callback)
					callback(static_cast<TApplication&>(*application));
			});
		}

		void Launch(std::shared_ptr<SenderApplication> application, const ReceiverChannel::OperationCompletedCallback& callback)
		{
			_receiver_channel.Launch(application, callback);
		}

		void Join(std::shared_ptr<SenderApplication> application, const ReceiverChannel::OperationCompletedCallback& callback)
		{
			_receiver_channel.Join(application, callback);
		}

		void Mute(bool mute, const ReceiverChannel::OperationCompletedCallback& callback)
		{
			_receiver_channel.Mute(mute, callback);
		}

		void SetVolume(double volume_level, const ReceiverChannel::OperationCompletedCallback& callback)
		{
			_receiver_channel.SetVolume(volume_level, callback)
		}
	};

	template <typename TReceiverChannel = ReceiverChannel, typename TConnectionChannel = ConnectionChannel, typename THeartbeatChannel = HeartbeatChannel, typename TConnection = ChromecastConnection>
	const std::string ChromecastClient<TReceiverChannel, TConnectionChannel, THeartbeatChannel, TConnection>::k_sender0 = "sender-0";
	template <typename TReceiverChannel = ReceiverChannel, typename TConnectionChannel = ConnectionChannel, typename THeartbeatChannel = HeartbeatChannel, typename TConnection = ChromecastConnection>
	const std::string ChromecastClient<TReceiverChannel, TConnectionChannel, THeartbeatChannel, TConnection>::k_receiver0 = "receiver-0";
}