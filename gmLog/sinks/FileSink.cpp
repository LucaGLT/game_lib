/**
 * @file sinks/FileSink.cpp
 * @brief Implementation of FileSink.
 */

#include "FileSink.hpp"

#include <stdexcept>

namespace GmLog {

FileSink::FileSink(const std::string& filePath)
    : filePath_(filePath)
{
    file_.open(filePath, std::ios::out | std::ios::app);
    if (!file_.is_open()) {
        throw std::runtime_error("FileSink: cannot open log file: " + filePath);
    }
}

FileSink::~FileSink()
{
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

void FileSink::write(const std::string& message)
{
    file_ << message << '\n';
}

void FileSink::flush()
{
    file_.flush();
}

} // namespace GmLog
