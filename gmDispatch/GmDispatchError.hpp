/**
 * @file GmDispatchError.hpp
 * @brief Base exception class for the gmDispatch library.
 *
 * All exceptions thrown by the gmDispatch library derive from EDispatchError.
 */

#ifndef GMDISPATCH_GMDISPATCHERROR_HPP
#define GMDISPATCH_GMDISPATCHERROR_HPP

#include <stdexcept>
#include <string>

namespace gmDispatch {

/**
 * @class EDispatchError
 * @brief Base exception for all gmDispatch runtime errors.
 *
 * Throw this (or a derived class) whenever a contract violation or
 * irrecoverable I/O error occurs inside the gmDispatch library.
 */
class EDispatchError : public std::runtime_error
{
public:
	/**
	 * @brief Constructs an EDispatchError with the given message.
	 * @param message Human-readable description of the error.
	 */
	explicit EDispatchError(const std::string& message)
		: std::runtime_error(message)
	{}
};

} // namespace gmDispatch

#endif // GMDISPATCH_GMDISPATCHERROR_HPP
