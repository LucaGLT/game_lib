#ifndef GMDISPATCH_STDOUTCHANNEL_HPP
#define GMDISPATCH_STDOUTCHANNEL_HPP

/**
 * @file channels/StdoutChannel.hpp
 * @brief Debug channel that writes serialized envelopes to std::cout.
 */

#include "../IChannel.hpp"
#include "../ISerializer.hpp"
#include "../Envelope.hpp"

#include <memory>

namespace GmDispatch {

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
 *   bus.subscribe("*", std::make_shared<GmDispatch::StdoutChannel>());
 *
 *   // With an explicit serializer:
 *   bus.subscribe("*", std::make_shared<GmDispatch::StdoutChannel>(
 *       std::make_unique<GmDispatch::JsonSerializer>()));
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
     * @brief Constructs a StdoutChannel with the given serializer.
     *
     * @param serializer Optional serializer.  When @c nullptr (default),
     *                   a @ref JsonSerializer is created automatically in
     *                   Phase 2.
     */
    explicit StdoutChannel(std::unique_ptr<ISerializer> serializer = nullptr);

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
    std::unique_ptr<ISerializer> serializer_;
};

} // namespace GmDispatch

#endif // GMDISPATCH_STDOUTCHANNEL_HPP
