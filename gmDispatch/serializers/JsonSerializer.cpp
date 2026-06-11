#include "JsonSerializer.hpp"

namespace GmDispatch {

std::string JsonSerializer::serialize(const Envelope& /*envelope*/)
{
    // TODO: Phase 2 — build JSON object:
    //   1. Format envelope.timestamp as ISO-8601 UTC with ms precision.
    //   2. Escape envelope.source, typeId, messageId with escapeJsonString().
    //   3. Build targets JSON array from envelope.targets.
    //   4. Represent envelope.payload as envelope.payload.type().name()
    //      (Phase 3: allow registration of per-type payload serializers).
    //   Return the assembled single-line JSON string without trailing newline.
    return "{}";
}

std::string JsonSerializer::escapeJsonString(const std::string& /*value*/)
{
    // TODO: Phase 2 — implement JSON string escaping:
    //   '\' → "\\"
    //   '"' → "\""
    //   '\n' → "\\n"
    //   '\r' → "\\r"
    //   '\t' → "\\t"
    //   chars < 0x20 → "\\uXXXX"
    return "";
}

} // namespace GmDispatch
