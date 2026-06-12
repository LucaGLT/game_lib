#ifndef GMDISPATCH_FILECHANNEL_HPP
#define GMDISPATCH_FILECHANNEL_HPP

/**
 * @file channels/FileChannel.hpp
 * @brief Channel that appends serialized envelopes to a file on disk.
 */

#include "IChannel.hpp"
#include "ISerializer.hpp"

#include <fstream>
#include <string>
#include <memory>

namespace gmDispatch {

/**
 * @brief Channel that appends one serialized envelope per line to a text file.
 *
 * FileChannel composes with an @ref ISerializer (default: @ref JsonSerializer)
 * to convert each @ref Envelope to a string before writing.  The file is
 * opened in append mode at construction and kept open for the channel's
 * lifetime.
 *
 * @throws EDispatchError at construction if the file cannot be opened.
 *
 * ### Thread safety
 * Thread safety is provided by the owning dispatcher's mutex.  No additional
 * locking is performed inside this class.
 *
 * @par Example
 * @code
 *   std::shared_ptr<gmDispatch::FileChannel> fc =
 *       std::make_shared<gmDispatch::FileChannel>("events.log", "Events");
 *   bus.subscribe("*", fc);
 * @endcode
 */
class FileChannel : public IChannel {
public:
	/**
	 * @brief Opens (or creates) the log file in append mode.
	 *
	 * @param filePath    Path to the output file.
	 * @param channelName Optional name for targeted delivery.  Empty = anonymous.
	 * @param serializer  Optional serializer.  When @c nullptr, a default
	 *                    @ref JsonSerializer is created automatically.
	 * @throws EDispatchError if the file cannot be opened.
	 */
	explicit FileChannel(const std::string&           filePath,
						 const std::string&           channelName = "",
						 std::unique_ptr<ISerializer> serializer  = nullptr);

	/**
	 * @brief Flushes and closes the file.
	 */
	~FileChannel();

	/// @brief Returns the channel name provided at construction.
	std::string name() const override;

	/**
	 * @brief Serialises @p envelope and appends it to the file followed by @c '\\n'.
	 *
	 * @param envelope The dispatch event to write.
	 */
	void send(const Envelope& envelope) override;

	/**
	 * @brief Flushes the file-stream write buffer to the OS.
	 */
	void flush() override;

private:
	std::string                  _name;
	std::string                  _file_path;   ///< Stored for error diagnostics.
	std::ofstream                _file;
	std::unique_ptr<ISerializer> _serializer;
};

} // namespace gmDispatch

#endif // GMDISPATCH_FILECHANNEL_HPP
