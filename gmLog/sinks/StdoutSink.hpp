#ifndef GMLOG_STDOUTSINK_HPP
#define GMLOG_STDOUTSINK_HPP

/**
 * @file sinks/StdoutSink.hpp
 * @brief Log sink that writes to standard output.
 */

#include "../ILogSink.hpp"

namespace GmLog {

/**
 * @brief Log sink that writes formatted log lines to @c std::cout.
 *
 * Each @ref write() call prints one line followed by @c std::endl (which
 * also flushes @c std::cout implicitly).
 *
 * Suitable for development builds, CLI applications, and containerised
 * environments where log aggregation is performed on stdout.
 *
 * @note Thread safety is provided by the owning @ref SyncDispatcher.
 *       No additional locking is performed inside this class.
 */
class StdoutSink : public ILogSink {
public:
    StdoutSink()  = default;
    ~StdoutSink() = default;

    /**
     * @brief Writes @p message to @c std::cout followed by @c std::endl.
     * @param message Pre-formatted log line.
     */
    void write(const std::string& message) override;

    /**
     * @brief Explicitly flushes @c std::cout.
     */
    void flush() override;
};

} // namespace GmLog

#endif // GMLOG_STDOUTSINK_HPP
