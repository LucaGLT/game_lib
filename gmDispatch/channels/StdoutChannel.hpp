#ifndef GMDISPATCH_STDOUTCHANNEL_HPP
#define GMDISPATCH_STDOUTCHANNEL_HPP

/**
 * @file channels/StdoutChannel.hpp
 * @brief Debug channel that writes serialized envelopes to std::cout.
 */

#include "IChannel.hpp"
#include "ISerializer.hpp"
#include "Envelope.hpp"

#include <memory>

namespace gmDispatch {

/**
 * @brief Debug channel that serialises envelopes and writes them to @c std::cout.
 *
 * StdoutChannel composes with an @ref ISerializer to convert the @ref Envelope
 * to a string before printing.  If no serialiser is provided at construction,
 * a default @ref JsonSerializer is created automatically (Phase 2).
 *
 * ### Usage
 * @code
 *   // With default JsonSerializer (Phase 2 behaviour):
 *   bus.subscribe("*", std::make_shared<gmDispatch::StdoutChannel>());
 *
 *   // With an explicit serializer:
 *   bus.subscribe("*", std::make_shared<gmDispatch::StdoutChannel>(
 *       std::make_unique<gmDispatch::JsonSerializer>()));
 * @endcode
 *
 * ### Output format (JsonSerializer default)
 * One JSON object per line, written to @c std::cout followed by @c std::endl.
 * @code
 *   {"time":"2026-06-11T10:30:00.123","source":"CoreEngine","typeId":"engine.tick",...}
 * @endcode
 */
class StdoutChannel : public IChannel {
public:
	/**
	 * @brief Constructs a StdoutChannel with an optional name and serializer.
	 *
	 * @param channelName Identifies this channel for targeted delivery.
	 *                    Empty = anonymous.
	 * @param serializer  Optional serializer.  When @c nullptr, a default
	 *                    @ref JsonSerializer is created automatically.
	 */
	explicit StdoutChannel(const std::string&           channelName = "",
						   std::unique_ptr<ISerializer> serializer  = nullptr);

	/// @brief Returns the channel name provided at construction.
	std::string name() const override;

	/**
	 * @brief Serialises @p envelope and writes the result to @c std::cout.
	 *
	 * @param envelope The dispatch event to display.
	 */
	void send(const Envelope& envelope) override;

	/**
	 * @brief Flushes @c std::cout.
	 */
	void flush() override;

private:
	std::string                  _name;
	std::unique_ptr<ISerializer> _serializer;
};

} // namespace gmDispatch

#endif // GMDISPATCH_STDOUTCHANNEL_HPP
