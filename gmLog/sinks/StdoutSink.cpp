/**
 * @file sinks/StdoutSink.cpp
 * @brief Implementation of StdoutSink.
 */

#include "StdoutSink.hpp"

#include <iostream>

namespace gmLog {

void StdoutSink::write(const std::string& message)
{
	std::cout << message << std::endl;
}

void StdoutSink::flush()
{
	std::cout.flush();
}

} // namespace gmLog
