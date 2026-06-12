/**
 * @file channels/FileChannel.cpp
 * @brief Implementation of FileChannel.
 */

#include "FileChannel.hpp"
#include "serializers/JsonSerializer.hpp"
#include "GmDispatchError.hpp"

namespace gmDispatch {

FileChannel::FileChannel(const std::string&           filePath,
						 const std::string&           channelName,
						 std::unique_ptr<ISerializer> serializer)
	: _name(channelName)
	, _file_path(filePath)
	, _serializer(std::move(serializer))
{
	if (!_serializer)
	{
		_serializer = std::make_unique<JsonSerializer>();
	}

	_file.open(_file_path, std::ios::out | std::ios::app);
	if (!_file.is_open())
	{
		throw EDispatchError("FileChannel: cannot open file: " + _file_path);
	}
}

FileChannel::~FileChannel()
{
	if (_file.is_open())
	{
		_file.flush();
		_file.close();
	}
}

std::string FileChannel::name() const
{
	return _name;
}

void FileChannel::send(const Envelope& envelope)
{
	_file << _serializer->serialize(envelope) << '\n';
}

void FileChannel::flush()
{
	_file.flush();
}

} // namespace gmDispatch
