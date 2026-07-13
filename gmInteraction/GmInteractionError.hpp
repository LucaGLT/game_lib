#ifndef GMINTERACTION_GMINTERACTIONERROR_HPP
#define GMINTERACTION_GMINTERACTIONERROR_HPP

/**
 * @file GmInteractionError.hpp
 * @brief Exception hierarchy for the gmInteraction library.
 *
 * Every error thrown by gmInteraction derives from @ref EInteractionError so
 * that callers can catch the whole library with a single handler.
 */

#include <stdexcept>
#include <string>

namespace gmInteraction
{

/**
 * @brief Base class for all gmInteraction errors.
 *
 * Prepends the prefix @c "EInteractionError: " to every message so log output
 * is immediately attributable to this library.
 */
class EInteractionError : public std::runtime_error
{
public:
	/**
	 * @brief Builds the error with a descriptive message.
	 *
	 * @param message  Human-readable explanation of the failure.
	 */
	explicit EInteractionError(const std::string& message)
		: std::runtime_error("EInteractionError: " + message)
	{
	}
};

/**
 * @brief Thrown when creating an object whose id already exists in the store.
 */
class EDuplicateObjectError : public EInteractionError
{
public:
	/**
	 * @brief Builds the error for a duplicate object id.
	 *
	 * @param message  Description including the offending id.
	 */
	explicit EDuplicateObjectError(const std::string& message)
		: EInteractionError(message)
	{
	}
};

/**
 * @brief Thrown when an object id is not present in the store.
 */
class EUnknownObjectError : public EInteractionError
{
public:
	/**
	 * @brief Builds the error for an unknown object id.
	 *
	 * @param message  Description including the missing id.
	 */
	explicit EUnknownObjectError(const std::string& message)
		: EInteractionError(message)
	{
	}
};

/**
 * @brief Thrown when a metadata key is requested but not present.
 */
class EUnknownMetaKeyError : public EInteractionError
{
public:
	/**
	 * @brief Builds the error for a missing metadata key.
	 *
	 * @param message  Description including the missing key.
	 */
	explicit EUnknownMetaKeyError(const std::string& message)
		: EInteractionError(message)
	{
	}
};

} // namespace gmInteraction

#endif // GMINTERACTION_GMINTERACTIONERROR_HPP
