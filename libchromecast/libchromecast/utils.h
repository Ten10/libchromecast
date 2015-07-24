#include <boost\system\error_code.hpp>

namespace chromecast
{
	void throw_on_error(bool error, const char* file, const char* function, uint32_t line, const std::string& error_text);
	void throw_on_error(const boost::system::error_code& error, const char* file, const char* function, uint32_t line, const std::string& error_text = "");
}

#define THROW_ON_ERROR_EX(error, error_text) chromecast::throw_on_error(error, ##__FILE__, ##__FUNCTION__, __LINE__, error_text)
#define THROW_ON_ERROR(error) THROW_ON_ERROR_EX(error, "")
