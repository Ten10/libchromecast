#include "cast_message.h"
#include "google_cast_message.h"

#include <sstream>

//#include <boost/locale.hpp>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>

namespace chromecast
{
	CastMessage::Address::Address(const std::string& source_id, const std::string& destination_id, const std::string& namespace_id)
		: _source(source_id),
		_destination(destination_id),
		_namespace(namespace_id)
	{
	}

	std::string CastMessage::Address::ToString() const
	{
		std::stringstream stream;
		stream << "Source: " << _source << std::endl;
		stream << "Destination: " << _destination << std::endl;
		stream << "Namespace: " << _namespace << std::endl;
		return stream.str();
	}

	bool CastMessage::Address::operator<(const Address& other) const
	{
		int source_id_compare = _source.compare(other._source);
		if (source_id_compare != 0)
			return source_id_compare == -1;

		int namespace_id_compare = _namespace.compare(other._namespace);
		if (namespace_id_compare != 0)
			return namespace_id_compare == -1;

		if (_destination == "*")
			return other._destination != "*";

		return _destination < other._destination;
	}

	std::string CastMessage::ToString() const
	{
		std::stringstream stream;
		stream << address.ToString() << std::endl;
		stream << "Payload: " << std::endl;
		if (payload_type == CastMessage::ePayloadType::String)
			stream << payload_utf8 << std::endl;
		else
		{
			uint32_t count = 0;
			for (byte b : payload_binary)
			{
				stream << b;
				++count;
				if (count % 10 == 0)
					stream << std::endl;
				else if (count % 5 == 0)
					stream << ' ';
			}
			stream << std::endl;
		}
		return stream.str();
	}

	void CastMessage::Serialize(boost::asio::streambuf& buffer, bool save)
	{
		//auto locale = boost::locale::generator().generate("");

		using namespace google::protobuf::io;
		typedef extensions::core_api::cast_channel::CastMessage ChromeCastMessage;

		if (save)
		{
			ChromeCastMessage msg;
			msg.set_protocol_version(ChromeCastMessage::ProtocolVersion::CastMessage_ProtocolVersion_CASTV2_1_0);
			msg.set_source_id(address._source);
			msg.set_destination_id(address._destination);
			msg.set_namespace_(address._namespace);
			if (payload_type == CastMessage::ePayloadType::String)
			{
				msg.set_payload_type(ChromeCastMessage::PayloadType::CastMessage_PayloadType_STRING);
				//std::string utf8_payload = boost::locale::conv::to_utf<char>(payload_utf8, locale);
				msg.set_payload_utf8(payload_utf8);
			}
			else
			{
				if (payload_binary.empty())
					throw std::exception("cannot send a empty binary message");
				msg.set_payload_type(ChromeCastMessage::PayloadType::CastMessage_PayloadType_BINARY);
				msg.set_payload_binary(&payload_binary[0], payload_binary.size());
			}
			std::ostream stream(&buffer);
			OstreamOutputStream output_stream(&stream);
			CodedOutputStream coded_output_stream(&output_stream);
			if (!msg.SerializeToCodedStream(&coded_output_stream))
				throw std::exception("could not store message");
		}
		else
		{
			ChromeCastMessage msg;
			std::istream stream(&buffer);
			IstreamInputStream input_stream(&stream);
			CodedInputStream coded_input_stream(&input_stream);
			if (!msg.ParseFromCodedStream(&coded_input_stream))
				throw std::exception("could not load message");

			protocol_version = static_cast<eProtocolVersion>(msg.protocol_version());
			address._source = msg.source_id();
			address._destination = msg.destination_id();
			address._namespace = msg.namespace_();
			if (msg.payload_type() == ChromeCastMessage::PayloadType::CastMessage_PayloadType_STRING)
			{
				payload_type = ePayloadType::String;
				payload_utf8 = msg.payload_utf8();
				//payload_utf8 = boost::locale::conv::from_utf<char>(payload_utf8, locale);
			}
			else
			{
				payload_type = ePayloadType::Binary;
				std::string binary = msg.payload_binary();
				payload_binary.assign(binary.begin(), binary.end());
			}
		}
	}
}