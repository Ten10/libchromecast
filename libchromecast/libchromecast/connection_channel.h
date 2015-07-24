#pragma once
#include "channel.h"

namespace chromecast
{
	class ConnectionChannel : public ChromecastChannel
	{
	public:
		ConnectionChannel(ChromecastConnection& connection, const std::string& sender, const std::string& receiver);

		void Connect();
		void Close();
	};
}