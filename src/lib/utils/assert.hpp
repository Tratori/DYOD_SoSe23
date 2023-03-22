#pragma once

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

#include <boost/preprocessor/stringize.hpp>

#include "string_utils.hpp"

/**
 * This file provides better assertions than the std cassert/assert.h - DebugAssert(condition, msg) and Fail(msg) can be
 * used
 * to both harden code by programming by contract and document the invariants enforced in messages.
 *
 * --> Use DebugAssert() whenever a certain invariant must hold, as in
 *
 * int divide(int numerator, int denominator) {
 *   DebugAssert(denominator == 0, "Divisions by zero are not allowed");
 *   return numerator / denominator;
 * }
 *
 *
 * --> Use Fail() whenever an illegal code path is taken. Especially useful for switch statements:
 *
 * void foo(int v) {
 *   switch(v) {
 *     case 0: //...
 *     case 3: //...
 *     case 17: //...
 *     default: Fail("Illegal parameter");
 * }
 *
 * --> Use Assert() whenever an invariant should be checked even in release builds, either because testing it is
 *     very cheap or the invariant is considered very important
 */

namespace opossum {

namespace detail {
// We need this indirection so that we can throw exceptions from destructors without the compiler complaining. That is
// generally forbidden and might lead to std::terminate, but since we don't want to handle most errors anyway,
// that's fine.
[[noreturn]] inline void fail(const std::string& msg) {
  throw std::logic_error(msg);
}
}  // namespace detail

#define Fail(msg)                                                                                               \
  opossum::detail::fail(opossum::trim_source_file_path(__FILE__) + ":" BOOST_PP_STRINGIZE(__LINE__) " " + msg); \
  static_assert(true, "End call of macro with a semicolon")
}  // namespace opossum

#define Assert(expr, msg)         \
  if (!static_cast<bool>(expr)) { \
    Fail(msg);                    \
  }                               \
  static_assert(true, "End call of macro with a semicolon")

#if OPOSSUM_DEBUG
#define DebugAssert(expr, msg) Assert(expr, msg)
#else
#define DebugAssert(expr, msg)
#endif
