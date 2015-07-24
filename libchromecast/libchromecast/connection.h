#pragma once
#include "types.h"
#include "cast_message.h"
#include "channel.h"
#include "channel_factory.h"

#include <memory>
#include <boost\asio.hpp>
#include <boost\endian\arithmetic.hpp>

namespace chromecast
{
	struct TLSSocket;
	class TLSConnection
	{
		typedef TLSConnection type;
		// Type of a shared pointer to this connection socket component

		typedef std::function<void(const boost::system::error_code& error)> RequestCompletedCallback;
		typedef std::function<void(const boost::system::error_code& error, size_t bytes_transferred)> IORequestCompletedCallback;
	private:
		boost::noncopyable _non_copyable;

		TLSSocket* _socket_impl;
		RequestCompletedCallback _connection_callback;

		void OnConnectCompleted(const boost::system::error_code& error);
		void OnHandshake(const boost::system::error_code& error);
		virtual void OnConnectionReady() { }

	protected:
		void AsyncRead(byte* buffer, size_t read_byte_count, const IORequestCompletedCallback& read_completed);

	public:
		TLSConnection(boost::asio::io_service& io_service);
		virtual ~TLSConnection();

		bool AsyncConnect(std::string ip, uint16_t port, const RequestCompletedCallback& callback);
		void AsyncConnect(const  boost::asio::ip::tcp::endpoint& end_point, const RequestCompletedCallback& callback);
		void Close();

		void EnsureConnectionIsAlive();
		void AsyncWrite(boost::asio::streambuf& buffer, const IORequestCompletedCallback& write_completed);
	};

	class ChromecastConnection : protected TLSConnection 
	{
		struct CastPacket
		{
			boost::endian::big_uint32_t length;
			std::unique_ptr<byte[]> data;
		};
		CastPacket _current_packet;
		std::map<ChromecastChannel::Address, ChromecastChannel*> _channel_address_to_channel;

		void OnConnectionReady() override;

		void OnMessage(const CastMessage& message);
		void StartReadingPacketLength();
		void StartReadingData(uint32_t remaining_data_byte_count);
		void StartReading(byte* buffer, uint32_t buffer_size, const std::function<void()>& completed_reading);
	protected:
		virtual void OnUnrecognizedAddress(const CastMessage& message);
	public:
		ChromecastConnection(boost::asio::io_service& io_service);
		~ChromecastConnection();

		ChromecastChannelFactory channel_factory;

		using TLSConnection::AsyncConnect;
		using TLSConnection::Close;

		void AsyncWrite(CastMessage& message);

		void RegisterChannel(ChromecastChannel& channel);
		void UnregisterChannel(const ChromecastChannel& channel);
	};
}