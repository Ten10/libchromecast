#include "connection.h"
#include "channel.h"
#include "json_message.h"
#include "utils.h"

#include <boost/bind.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/detail/config.hpp>

namespace chromecast
{
	using namespace std;
	using namespace boost;

	struct TLSSocket
	{
		// Type of the ASIO socket being used
		typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket_type;

		socket_type socket;

		TLSSocket(boost::asio::io_service& io_service)
			: socket(io_service, boost::asio::ssl::context(boost::asio::ssl::context::tlsv12_client))
		{

		}
	};

	void TLSConnection::OnConnectCompleted(const boost::system::error_code& error)
	{
		if (error)
		{
			if (_connection_callback)
				_connection_callback(error);
			return;
		}

		_socket_impl->socket.async_handshake(boost::asio::ssl::stream_base::client, boost::bind(&TLSConnection::OnHandshake, this, boost::asio::placeholders::error));
	}

	void TLSConnection::OnHandshake(const boost::system::error_code& error)
	{
		if (!error)
			OnConnectionReady();
		if (_connection_callback)
			_connection_callback(error);
	}

	void TLSConnection::AsyncRead(byte* buffer, size_t read_byte_count, const IORequestCompletedCallback& read_completed)
	{
		boost::asio::async_read(_socket_impl->socket, boost::asio::buffer(buffer, read_byte_count), read_completed);
	}

	TLSConnection::TLSConnection(boost::asio::io_service& io_service)
	{
		_socket_impl = new TLSSocket(io_service);
	}

	TLSConnection::~TLSConnection()
	{
		try
		{
			Close();
		}
		catch (std::exception& e)
		{
			std::string msg = e.what();
		}
		delete _socket_impl;
	}

	bool TLSConnection::AsyncConnect(std::string ip_address, uint16_t port, const RequestCompletedCallback& callback)
	{
		system::error_code error;
		auto address = boost::asio::ip::address::from_string(ip_address, error);
		if (error)
			return false;
		boost::asio::ip::tcp::endpoint end_point(address, port);
		AsyncConnect(end_point, callback);
		return true;
	}

	void TLSConnection::AsyncConnect(const boost::asio::ip::tcp::endpoint& end_point, const RequestCompletedCallback& callback)
	{
		_connection_callback = callback;
		_socket_impl->socket.set_verify_mode(boost::asio::ssl::verify_none);
		_socket_impl->socket.lowest_layer().async_connect(end_point, boost::bind(&TLSConnection::OnConnectCompleted, this, boost::asio::placeholders::error));
	}

	void TLSConnection::Close()
	{
		_socket_impl->socket.shutdown();
	}

	void TLSConnection::EnsureConnectionIsAlive()
	{
		byte b = 0;
		boost::system::error_code error;
		_socket_impl->socket.write_some(boost::asio::buffer(&b, sizeof(b)), error);
		THROW_ON_ERROR_EX(error, "invalid application");
	}

	void TLSConnection::AsyncWrite(boost::asio::streambuf& buffer, const IORequestCompletedCallback& write_completed)
	{
		boost::asio::async_write(_socket_impl->socket, buffer, write_completed);
	}

	void ChromecastConnection::OnConnectionReady()
	{
		StartReadingPacketLength();
	}

	void ChromecastConnection::OnMessage(const CastMessage& message)
	{
		if (message.address._destination != "*")
		{
			auto it = _channel_address_to_channel.find(message.address);
			if (it != _channel_address_to_channel.end() && it->second)
				it->second->OnMessage(message);
			else
				OnUnrecognizedAddress(message);
		}
		else
		{
			for (auto& channel_address_to_channel_pair : _channel_address_to_channel)
			{
				if (channel_address_to_channel_pair.first._namespace != message.address._namespace
					|| channel_address_to_channel_pair.first._source != message.address._source)
					continue;
				channel_address_to_channel_pair.second->OnMessage(message);
			}
		}
	}

	void ChromecastConnection::StartReadingPacketLength()
	{
		StartReading((byte*)&_current_packet.length, sizeof(_current_packet.length), [=]()
		{
			uint32_t packet_length = _current_packet.length;
			_current_packet.data = make_unique<byte[]>(packet_length);
			StartReadingData(_current_packet.length);
		});
	}

	void ChromecastConnection::StartReadingData(uint32_t remaining_data_byte_count)
	{
		StartReading(_current_packet.data.get(), _current_packet.length, [=]()
		{
			boost::asio::streambuf buffer;
			buffer.sputn((char*)_current_packet.data.get(), _current_packet.length);

			CastMessage message;
			message.Serialize(buffer, false);
			OnMessage(message);
			StartReadingPacketLength();
		});
	}

	void ChromecastConnection::StartReading(byte* buffer, uint32_t buffer_size, const std::function<void()>& completed_reading)
	{
		__super::AsyncRead(buffer, buffer_size, [=](const system::error_code& error, size_t bytes_transferred)
		{
			if (error || buffer_size != bytes_transferred)
			{
				EnsureConnectionIsAlive();
				StartReading(buffer + bytes_transferred, buffer_size - bytes_transferred, completed_reading);
			}
			else
				completed_reading();
		});
	}

	void ChromecastConnection::OnUnrecognizedAddress(const CastMessage& message)
	{
		THROW_ON_ERROR_EX(true, "received message without a clear destination:\n" + message.ToString());
	}

	ChromecastConnection::ChromecastConnection(boost::asio::io_service& io_service)
		: TLSConnection(io_service),
		channel_factory(io_service, *this)
	{
	}

	ChromecastConnection::~ChromecastConnection()
	{
#ifdef _DEBUG
		//assert(_channel_address_to_channel.empty());
#endif
	}

	void ChromecastConnection::AsyncWrite(CastMessage& message)
	{
		boost::asio::streambuf message_buffer;
		message.Serialize(message_buffer, true);

		auto output_buffer = make_shared<boost::asio::streambuf>();
		endian::big_uint32_t packet_size = message_buffer.size();
		output_buffer->sputn((char*)&packet_size, sizeof(packet_size));
		message.Serialize(*output_buffer, true);

		uint32_t total_size = output_buffer->size();
		__super::AsyncWrite(*output_buffer, [output_buffer, total_size](const boost::system::error_code& error, size_t bytes_transferred)
		{
			THROW_ON_ERROR_EX(error, "failed to write packet data: " + error.message());

			//throwing instead of sending missing data, not encountered a short write.
			THROW_ON_ERROR_EX(total_size != bytes_transferred, "could only write " + to_string(bytes_transferred) + " out of " + to_string(total_size) + " bytes");
		});
	}

	void ChromecastConnection::RegisterChannel(ChromecastChannel& channel)
	{
		ChromecastChannel::Address address = channel.GetAddress();
		swap(address._source, address._destination);
		auto it = _channel_address_to_channel.find(address);
		THROW_ON_ERROR_EX(it != _channel_address_to_channel.end(), "address: " + address.ToString() + " already registered");
		_channel_address_to_channel[address] = &channel;
	}

	void ChromecastConnection::UnregisterChannel(const ChromecastChannel& channel)
	{
		ChromecastChannel::Address address = channel.GetAddress();
		swap(address._source, address._destination);
		auto it = _channel_address_to_channel.find(address);
		THROW_ON_ERROR_EX(it == _channel_address_to_channel.end(), "address " + address.ToString() + " not registered");
		_channel_address_to_channel.erase(it);
	}
}