#include "utils.h"
#include <string>

namespace chromecast
{
	using namespace std;

	void throw_on_error(bool error, const char* file, const char* function, uint32_t line, const std::string& error_text)
	{
		if (!error)
			return;

		std::string message = std::string("at ") + file + " in " + function + " on " + to_string(line) + ": " + error_text;
		throw std::runtime_error(message);
	}

	void throw_on_error(const boost::system::error_code& error, const char* file, const char* function, uint32_t line, const std::string& error_text)
	{
		if (!error)
			return;
		std::string message = std::string("at ") + file + " in " + function + " on " + to_string(line) + ": " + error.message() + " (error: " + to_string(error.value()) + "), " + error_text;
		throw std::runtime_error(message);	
	}
}