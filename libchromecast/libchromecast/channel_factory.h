#pragma once
#include "channel.h"
#include <boost\asio\io_service.hpp>

namespace chromecast
{
	class ChromecastConnection;
	class ChromecastChannelFactory
	{
		boost::noncopyable _non_copyable;
		ChromecastConnection& _connection;
		boost::asio::io_service& _io_service;
	public:
		ChromecastChannelFactory(boost::asio::io_service& io_service, ChromecastConnection& connection)
			: _connection(connection),
			_io_service(io_service)
		{
		}

		template <typename TChannel, typename ...TArgs>
		std::unique_ptr<TChannel> CreateChannel(TArgs ...Args)
		{
			return std::make_unique<TChannel>(_connection, Args...);
		}

		template <typename TChannel, typename ...TArgs>
		std::unique_ptr<TChannel> CreateIOServiceChannel(TArgs ...Args)
		{
			return std::make_unique<TChannel>(_io_service, _connection, Args...);
		}
	};
}