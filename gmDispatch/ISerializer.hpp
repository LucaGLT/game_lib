#ifndef GMDISPATCH_ISERIALIZER_HPP
#define GMDISPATCH_ISERIALIZER_HPP

/**
 * @file ISerializer.hpp
 * @brief Abstract serializer interface for the GmDispatch library.
 */

#include "Envelope.hpp"

#include <string>

namespace GmDispatch {

/**
 * @brief Abstract base class for envelope serializers.
 *
 * A serializer converts an @ref Envelope into a string representation that
 * can then be written to an I/O medium (file, socket, stdout, …).  Separating
 * serialization from the channel lets the same channel support different
 * output formats without any changes to routing or dispatch logic.
 *
 * ### V1 concrete implementation
 * @ref JsonSerializer — produces one JSON object per line (JSON Lines format).
 *
 * ### Relationship with ILogFormatter
 * This interface plays the same role as @c GmLog::ILogFormatter, but operates
 * on @ref Envelope instead of @c GmLog::LogRecord.
 *
 * ### Implementing a custom serializer
 * @code
 *   class CsvSerializer : public GmDispatch::ISerializer {
 *   public:
 *       std::string serialize(const GmDispatch::Envelope& envelope) override {
 *           return envelope.source + "," + envelope.typeId + ",...";
 *       }
 *   };
 * @endcode
 *
 * @note Implementations must NOT append a trailing newline; the channel is
 *       responsible for line termination.
 */
class ISerializer {
public:
    virtual ~ISerializer() = default;

    /**
     * @brief Converts an envelope to its string representation.
     *
     * @param envelope The dispatch event to serialize.
     * @return Formatted string ready to be passed to a channel's write
     *         operation.  Must NOT contain a trailing newline.
     */
    virtual std::string serialize(const Envelope& envelope) = 0;
};

} // namespace GmDispatch

#endif // GMDISPATCH_ISERIALIZER_HPP
