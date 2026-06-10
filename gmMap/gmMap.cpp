/**
 * @file gmMap.cpp
 *
 * @note gmMap is a fully templated class.  In C++, template method bodies
 *       must be visible at every instantiation site, so ALL implementations
 *       live as inline definitions inside @ref gmMap.hpp (below the class
 *       declaration).  This file is therefore intentionally minimal.
 *
 *       If you later want to pre-instantiate the template for specific types
 *       (to reduce compile times), add explicit instantiation declarations
 *       here, for example:
 *
 *       @code
 *       #include "gmMap.hpp"
 *       namespace GameMap {
 *           template class gmMap<int>;
 *           template class gmMap<std::string>;
 *       }
 *       @endcode
 */

#include "gmMap.hpp"
