/**
 * @file stats/StatBlock.cpp
 * @brief Implementation of StatBlock.
 */

#include "gmActor/stats/StatBlock.hpp"

namespace gmActor {

void StatBlock::set(const std::string& key, double value)
{
    data_[key] = value;
}

double StatBlock::get(const std::string& key, double default_val) const
{
    auto it = data_.find(key);
    return (it != data_.end()) ? it->second : default_val;
}

bool StatBlock::has(const std::string& key) const
{
    return data_.count(key) > 0;
}

void StatBlock::remove(const std::string& key)
{
    data_.erase(key);
}

const std::unordered_map<std::string, double>& StatBlock::data() const
{
    return data_;
}

} // namespace gmActor
