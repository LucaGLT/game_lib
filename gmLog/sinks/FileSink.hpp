#ifndef GMLOG_FILESINK_HPP
#define GMLOG_FILESINK_HPP

/**
 * @file sinks/FileSink.hpp
 * @brief Log sink that appends to a plain text file.
 */

#include "ILogSink.hpp"

#include <fstream>
#include <string>

namespace gmLog {

/**
 * @brief Log sink that appends formatted log lines to a text file on disk.
 *
 * The file is opened in append mode (@c std::ios::app) at construction time
 * and kept open for the lifetime of the sink.  Each @ref write() call appends
 * one line followed by @c '\\n'.
 *
 * This is an append-only, ever-growing file (V1 design).  Future variants
 * (@c RotatingFileSink, @c DailyFileSink) will implement @ref ILogSink in
 * the same way, allowing the @ref GmLogger to remain unchanged.
 *
 * @throws ELogError (at construction) if the file cannot be opened.
 *
 * @note Thread safety is provided by the owning @ref SyncDispatcher.
 *       No additional locking is performed inside this class.
 */
class FileSink : public ILogSink {
public:
	/**
	 * @brief Opens (or creates) the log file in append mode.
	 * @param filePath  Path to the log file.
	 * @throws ELogError if the file cannot be opened.
	 */
	explicit FileSink(const std::string& filePath);

	/**
	 * @brief Flushes and closes the file.
	 */
	~FileSink();

	/**
	 * @brief Appends @p message to the log file followed by @c '\\n'.
	 * @param message Pre-formatted log line.
	 */
	void write(const std::string& message) override;

	/**
	 * @brief Flushes the file-stream write buffer to the OS.
	 */
	void flush() override;

private:
	std::string   _file_path; ///< Path stored for diagnostic error messages.
	std::ofstream _file;     ///< Open file stream (append mode).
};

} // namespace gmLog

#endif // GMLOG_FILESINK_HPP
