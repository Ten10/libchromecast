#pragma once
#include "types.h"
#include <rapidjson\document.h>

#include <string>
#include <vector>

namespace chromecast
{
	class ConstJsonMessagePart
	{
	protected:
		const rapidjson::Document::GenericValue& _value;
	public:
		ConstJsonMessagePart(const rapidjson::Document::GenericValue& value);
		virtual ~ConstJsonMessagePart() { }

		bool GetBool() const;
		double GetDouble() const;
		uint32_t GetUint32() const;
		uint64_t GetUint64() const;
		std::string GetString() const;

		bool HasMember(const char* member) const;
		std::string ToString() const;
		size_t Size() const;

		ConstJsonMessagePart operator[](const char* text) const;
		ConstJsonMessagePart operator[](size_t index) const;
	};

	class JsonMessagePart : public ConstJsonMessagePart
	{
	protected:
		rapidjson::Document::GenericValue& _value;
		rapidjson::Document::AllocatorType& _allocator;
	public:
		JsonMessagePart(rapidjson::Document::GenericValue& value, rapidjson::Document::AllocatorType& allocator);

		void operator=(bool value);
		void operator=(double value);
		void operator=(uint32_t value);
		void operator=(uint64_t value);
		void operator=(const char* text);
		void operator=(const std::string& value);
		void operator=(const std::vector<std::string>& items);

		template <typename T>
		typename std::enable_if<!std::is_pod<T>::value && !std::is_same<std::string, T>::value, void>::type operator=(const std::vector<T>& items)
		{
			_value.SetArray();
			for (const auto& item : items)
			{
				_value.PushBack("", _allocator);
				item.ToMessage(JsonMessagePart(_value[_value.Size() - 1], _allocator));
			}
		}

		template <typename T>
		typename std::enable_if<std::is_pod<T>::value && !std::is_same<std::string, T>::value, void>::type operator=(const std::vector<T>& items)
		{
			_value.SetArray();
			for (const auto& item : items)
			{
				_value.PushBack("", _allocator);
				JsonMessagePart(_value[_value.Size() - 1], _allocator) = item;
			}
		}

		void Resize(size_t size);

		JsonMessagePart operator[](const char* text);
		JsonMessagePart operator[](size_t index);
	};

	class JsonMessage : public JsonMessagePart
	{
	protected:
		rapidjson::Document::AllocatorType _allocator;
		rapidjson::Document _document;
	public:
		JsonMessage();
		JsonMessage(const JsonMessage& message);
		virtual ~JsonMessage();

		void Parse(const std::string& json_string);
		std::string ToString() const;

		JsonMessagePart operator[](const char* text);
		const ConstJsonMessagePart operator[](const char* text) const;
	};
}