#include "connection_channel.h"
#include "json_message.h"

namespace chromecast
{
	static const std::string k_connection_namespace = "urn:x-cast:com.google.cast.tp.connection";

	ConnectionChannel::ConnectionChannel(ChromecastConnection& connection, const std::string& sender, const std::string& receiver)
		: ChromecastChannel(connection, ChromecastChannel::Address(sender, receiver, k_connection_namespace))
	{

	}

	void ConnectionChannel::Connect()
	{
		JsonMessage message;
		message["type"] = "CONNECT";
		Send(message.ToString());
	}

	void ConnectionChannel::Close()
	{
		JsonMessage message;
		message["type"] = "CLOSE";
		Send(message.ToString());
	}
}