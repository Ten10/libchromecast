#include "json_message.h"
#include "utils.h"

#define RAPIDJSON_NO_INT64DEFINE

#include <rapidjson\writer.h>
#include <rapidjson\reader.h>
#include <rapidjson\stringbuffer.h>

namespace chromecast
{
	ConstJsonMessagePart::ConstJsonMessagePart(const rapidjson::Document::GenericValue& value)
		: _value(value)
	{

	}

	bool ConstJsonMessagePart::GetBool() const
	{
		return _value.GetBool();
	}

	double ConstJsonMessagePart::GetDouble() const
	{
		return _value.GetDouble();
	}

	uint32_t ConstJsonMessagePart::GetUint32() const
	{
		return _value.GetUint();
	}

	uint64_t ConstJsonMessagePart::GetUint64() const
	{
		return _value.GetUint64();
	}

	std::string ConstJsonMessagePart::GetString() const
	{
		return _value.GetString();
	}

	bool ConstJsonMessagePart::HasMember(const char* member) const
	{
		return _value.HasMember(member);
	}

	std::string ConstJsonMessagePart::ToString() const
	{
		rapidjson::StringBuffer sb;
		rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
		_value.Accept(writer);
		return sb.GetString();
	}

	size_t ConstJsonMessagePart::Size() const
	{
		return _value.Size();
	}

	ConstJsonMessagePart ConstJsonMessagePart::operator[](const char* text) const
	{
		THROW_ON_ERROR_EX(!_value.HasMember(text), std::string("message does not have member ") + text);
		return ConstJsonMessagePart(_value[text]);
	}

	ConstJsonMessagePart ConstJsonMessagePart::operator[](size_t index) const
	{
		THROW_ON_ERROR_EX(_value.Size() <= index, "array out of bounds");
		return ConstJsonMessagePart(_value[index]);
	}

	JsonMessagePart::JsonMessagePart(rapidjson::Document::GenericValue& value, rapidjson::Document::AllocatorType& allocator)
		: ConstJsonMessagePart(value),
		_value(value),
		_allocator(allocator)
	{

	}

	void JsonMessagePart::operator=(bool value)
	{
		_value.SetBool(value);
	}

	void JsonMessagePart::operator=(double value)
	{
		_value.SetDouble(value);
	}

	void JsonMessagePart::operator=(uint64_t value)
	{
		_value.SetUint64(value);
	}

	void JsonMessagePart::operator=(uint32_t value)
	{
		_value.SetUint(value);
	}

	void JsonMessagePart::operator=(const char* text)
	{
		_value.SetString(text, _allocator);
	}

	void JsonMessagePart::operator=(const std::string& value)
	{
		_value.SetString(value.c_str(), _allocator);
	}

	void JsonMessagePart::operator=(const std::vector<std::string>& items)
	{
		_value.SetArray();
		for (const auto& item : items)
		{
			_value.PushBack("", _allocator);
			JsonMessagePart(_value[_value.Size() - 1], _allocator) = item.c_str();
		}
	}

	void JsonMessagePart::Resize(size_t size)
	{
		_value.SetArray();
		_value.Reserve(size, _allocator);
	}

	JsonMessagePart JsonMessagePart::operator[](const char* text)
	{
		if (!_value.IsObject())
			_value.SetObject();
		if (!_value.HasMember(text))
			_value.AddMember(text, "", _allocator);
		return JsonMessagePart(_value[text], _allocator);
	}

	JsonMessagePart JsonMessagePart::operator[](size_t index)
	{
		//index must be inside array range or at the end of the array, behavior is due to not having resize in rapid json library.
		THROW_ON_ERROR_EX(index > _value.Size(), "out of bounds index " + std::to_string(index));
		if (index == _value.Size())
			_value.PushBack("", _allocator);
		return JsonMessagePart(_value[index], _allocator);
	}

	JsonMessage::JsonMessage()
		: chromecast::JsonMessagePart(_document, _allocator),
		_document(&_allocator)
	{
	}

	JsonMessage::JsonMessage(const JsonMessage& message)
		: chromecast::JsonMessagePart(_document, _allocator),
		_document(&_allocator)
	{
		Parse(message.ToString());
	}

	JsonMessage::~JsonMessage()
	{

	}

	void JsonMessage::Parse(const std::string& json_string)
	{
		_document.Parse<rapidjson::kParseDefaultFlags>(json_string.c_str());
	}

	std::string JsonMessage::ToString() const
	{
		rapidjson::StringBuffer sb;
		rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
		_document.Accept(writer);
		return sb.GetString();
	}

	JsonMessagePart JsonMessage::operator[](const char* text)
	{
		if (!_document.IsObject())
			_document.SetObject();
		if (!_document.HasMember(text))
			_document.AddMember(text, "", _allocator);
		return JsonMessagePart(_document[text], _allocator);
	}

	const ConstJsonMessagePart JsonMessage::operator[](const char* text) const
	{
		return ConstJsonMessagePart(_document[text]);
	}
}