#pragma once
#include <string>
#include <vector>
#include <boost/asio/streambuf.hpp>

#include "types.h"

namespace chromecast
{
	struct CastMessage
	{
		struct Address
		{
			std::string _source;
			std::string _destination;
			std::string _namespace;

			Address() = default;
			Address(const std::string& source_id, const std::string& destination_id, const std::string& namespace_id);

			bool operator<(const Address& other) const;
			std::string ToString() const;
		};

		enum class eProtocolVersion : int
		{
			Version_2_1_0 = 0
		};

		enum class ePayloadType : int
		{
			String = 0,
			Binary = 1
		};

		eProtocolVersion protocol_version = eProtocolVersion::Version_2_1_0;

		Address address;

		ePayloadType payload_type;
		std::string payload_utf8;
		std::vector<byte> payload_binary;

		std::string ToString() const;

		void Serialize(boost::asio::streambuf& buffer, bool save);
	};
}