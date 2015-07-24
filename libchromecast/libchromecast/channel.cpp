#include "channel.h"
#include "connection.h"
#include "utils.h"
#include <iostream>

namespace chromecast
{
	using namespace std;

	void ChromecastChannel::OnSending(const CastMessage& message)
	{
	}

	void ChromecastChannel::OnMessage(const CastMessage& message)
	{
		bool handled = false;
		if (message.payload_type == CastMessage::ePayloadType::Binary)
			handled = OnMessage(message.payload_binary);
		else if (message.payload_type == CastMessage::ePayloadType::String)
			handled = OnMessage(message.payload_utf8);
		else
			THROW_ON_ERROR_EX(true, "unrecognized message payload type, message: " + message.ToString());

		if (!handled)
			OnUnhandledMessage(message);
	}

	bool ChromecastChannel::OnMessage(const std::string& message)
	{
		return false;
	}

	bool ChromecastChannel::OnMessage(const std::vector<byte>& message)
	{
		return false;
	}

	void ChromecastChannel::OnUnhandledMessage(const CastMessage& message)
	{
		//if another client is connected and sending commands then we can get the broadcasted response for it commands.
		if (message.address._destination != "*")
			THROW_ON_ERROR_EX(true, "unhandled message: " + message.ToString());
	}

	ChromecastChannel::ChromecastChannel(ChromecastConnection& connection, const ChromecastChannel::Address& address)
		: _address(address),
		_connection(connection)
	{
		_connection.RegisterChannel(*this);
	}

	ChromecastChannel::~ChromecastChannel()
	{
		_connection.UnregisterChannel(*this);
	}

	void ChromecastChannel::Send(const std::string& json_message)
	{
		CastMessage message;
		message.payload_type = CastMessage::ePayloadType::String;
		message.payload_utf8 = json_message;
		Send(move(message));
	}

	void ChromecastChannel::Send(const vector<byte>& binary_message)
	{
		CastMessage message;
		message.payload_type = CastMessage::ePayloadType::Binary;
		message.payload_binary = binary_message;
		Send(move(message));
	}

	void ChromecastChannel::Send(CastMessage&& message)
	{
		message.address = _address;
		//TODO pass a copy of the message so OnSending won't be able to modify the message using const_cast?
		OnSending(message);

		_connection.AsyncWrite(move(message));
	}
}