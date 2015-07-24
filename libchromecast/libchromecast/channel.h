#pragma once
#include "cast_message.h"
#include <boost\noncopyable.hpp>

namespace chromecast
{
	class ChromecastConnection;
	class ChromecastChannel
	{
	public:
		typedef CastMessage::Address Address;
	private:
		CastMessage::Address _address;
		boost::noncopyable _noncopyable;
	protected:
		ChromecastConnection& _connection;

		virtual void OnSending(const CastMessage& message);

		virtual bool OnMessage(const std::string& message);
		virtual bool OnMessage(const std::vector<byte>& message);
		virtual void OnUnhandledMessage(const CastMessage& message);
	public:
		ChromecastChannel(ChromecastConnection& connection, const ChromecastChannel::Address& address);
		~ChromecastChannel();

		const Address& GetAddress() const { return _address; }
		void Send(const std::string& json_message);
		void Send(const std::vector<byte>& binary_message);
		void Send(CastMessage&& message);

		virtual void OnMessage(const CastMessage& message);
	};
}