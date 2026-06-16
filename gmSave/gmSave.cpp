/**
 * @file gmSave.cpp
 * @brief Non-template implementations for the gmSave namespace.
 *
 * @note gmSave uses free function templates.  Template bodies must be visible
 *       at every instantiation site, so ALL template implementations live as
 *       inline definitions inside @ref gmSave.hpp.  This file contains only
 *       the non-template bodies:
 *
 *       - EVersionMismatchError constructor
 *       - peek_version()
 *
 *       If you later want to pre-instantiate templates for specific types
 *       (to reduce compile times), add explicit instantiation definitions
 *       here, for example:
 *
 *       @code
 *       namespace gmSave {
 *           template void save<MyStruct>(const std::string&, const MyStruct&, int);
 *           template MyStruct load<MyStruct>(const std::string&);
 *       }
 *       @endcode
 */

#include "gmSave.hpp"

#include <fstream>
#include <iterator>
#include <sstream>

namespace gmSave
{

// --- detail helpers ----------------------------------------------------------

namespace detail
{

void write_file(const std::string& filepath, const std::string& content)
{
	std::ofstream ofs(filepath);
	if (!ofs.is_open())
	{
		throw EFileWriteError("Cannot open file for writing: " + filepath);
	}
	ofs << content;
	if (!ofs.good())
	{
		throw EFileWriteError("Write error on file: " + filepath);
	}
}

std::string read_file(const std::string& filepath)
{
	std::ifstream ifs(filepath);
	if (!ifs.is_open())
	{
		throw EFileReadError("Cannot open file for reading: " + filepath);
	}
	return std::string(std::istreambuf_iterator<char>(ifs),
					   std::istreambuf_iterator<char>());
}

nlohmann::json parse_json(const std::string& content, const std::string& filepath)
{
	try
	{
		return nlohmann::json::parse(content);
	}
	catch (const nlohmann::json::parse_error& e)
	{
		throw EJsonParseError(
			"JSON parse error in '" + filepath + "': " + e.what());
	}
}

} // namespace detail

// --- EVersionMismatchError ----------------------------------------------------

EVersionMismatchError::EVersionMismatchError(uint32_t expected, uint32_t found)
	: ESaveError(
		  [&]
		  {
			  std::ostringstream oss;
			  oss << "Version mismatch: expected " << expected
				  << ", found " << found;
			  return oss.str();
		  }())
	, expected_version(expected)
	, found_version(found)
{}

// --- peek_version ------------------------------------------------------------

std::optional<uint32_t> peek_version(const std::string& filepath)
{
	try
	{
		const std::string    content = detail::read_file(filepath);
		const nlohmann::json j       = detail::parse_json(content, filepath);
		if (j.contains("_version") && j.at("_version").is_number_unsigned())
		{
			return j.at("_version").get<uint32_t>();
		}
		return std::nullopt;
	}
	catch (...)
	{
		return std::nullopt;
	}
}

} // namespace gmSave
