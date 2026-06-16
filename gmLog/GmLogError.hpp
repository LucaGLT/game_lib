#ifndef GMLOG_GMLOGERROR_HPP
#define GMLOG_GMLOGERROR_HPP

/**
 * @file GmLogError.hpp
 * @brief Base exception hierarchy for the gmLog library.
 */

#include <stdexcept>
#include <string>

namespace gmLog {

/**
 * @brief Base exception class for all gmLog errors.
 *
 * All exceptions thrown by the gmLog library derive from this class,
 * allowing callers to catch the entire library's error surface with a
 * single catch clause:
 * @code
 *   try { auto logger = gmLog::LoggerFactory::create_file_logger(...); }
 *   catch (const gmLog::ELogError& e) { ... }
 * @endcode
 *
 * @note Not thread-safe — exceptions are created on the throwing thread.
 */
class ELogError : public std::runtime_error
{
public:
	/**
	 * @brief Constructs an ELogError with a descriptive message.
	 * @param message Human-readable description of the error condition.
	 */
	explicit ELogError(const std::string& message)
		: std::runtime_error(message)
	{}
};

} // namespace gmLog

#endif // GMLOG_GMLOGERROR_HPP
