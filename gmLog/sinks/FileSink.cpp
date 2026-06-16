/**
 * @file sinks/FileSink.cpp
 * @brief Implementation of FileSink.
 */

#include "FileSink.hpp"
#include "GmLogError.hpp"

namespace gmLog {

FileSink::FileSink(const std::string& filePath)
	: _file_path(filePath)
{
	_file.open(filePath, std::ios::out | std::ios::app);
	if (!_file.is_open())
	{
		throw ELogError("FileSink: cannot open log file: " + filePath);
	}
}

FileSink::~FileSink()
{
	if (_file.is_open())
	{
		_file.flush();
		_file.close();
	}
}

void FileSink::write(const std::string& message)
{
	_file << message << '\n';
}

void FileSink::flush()
{
	_file.flush();
}

} // namespace gmLog
