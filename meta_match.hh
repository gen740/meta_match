// SPDX-License-Identifier: MIT
//
// Copyright (c) 2026 gen
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <algorithm>
#include <array>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#if defined(_MSC_VER)
#define META_MATCH_ALWAYS_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define META_MATCH_ALWAYS_INLINE inline __attribute__((always_inline))
#else
#define META_MATCH_ALWAYS_INLINE inline
#endif

namespace meta_match {

// ============================================================================
// StringLiteral — compile-time string usable as a non-type template parameter
// ============================================================================

/**
 * @brief Compile-time string type usable as a non-type template parameter.
 *
 * C++20 allows class-type NTTPs provided the type satisfies structural
 * requirements.  `StringLiteral<N>` wraps a null-terminated character array so
 * that string names such as `"verbose"` can appear directly in template
 * argument lists.
 *
 * @tparam N Total storage size including the null terminator.
 */
template <std::size_t N>
struct StringLiteral {
  /** Raw character storage (null-terminated, size = N, length = N-1). */
  std::array<char, N> value{};

  // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
  consteval StringLiteral(const char (&str)[N]) noexcept {
    std::copy_n(str, N, value.begin());
  }

  /** String length excluding the null terminator (= N - 1). */
  [[nodiscard]] constexpr auto size() const noexcept -> std::size_t {
    return N - 1;
  }

  /** Pointer to the first character. */
  [[nodiscard]] constexpr auto data() const noexcept -> const char* {
    return value.data();
  }

  /** `std::string_view` over the stored characters (without '\0'). */
  [[nodiscard]] constexpr auto view() const noexcept -> std::string_view {
    return {value.data(), size()};
  }

  /**
   * @brief Character at index `i`, including the null terminator.
   *
   * Accessing `operator[](size())` returns '\0', which is used by the
   * trie to handle keys that are proper prefixes of other keys.
   */
  [[nodiscard]] constexpr auto operator[](std::size_t i) const noexcept -> char {
    return value[i];
  }

  consteval auto operator==(const StringLiteral&) const noexcept
      -> bool = default;
};

// Deduction guide: StringLiteral("hello") → StringLiteral<6>
template <std::size_t N>
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
StringLiteral(const char (&)[N]) -> StringLiteral<N>;

// ============================================================================
// Handler<Key, Fn> — a key–callable pair usable as an element of match()
// ============================================================================

/**
 * @brief Associates a compile-time string key with a callable.
 *
 * `Handler` is the primary building block for meta_match.  Each instance
 * pairs a `StringLiteral` key (embedded in the template parameter) with an
 * arbitrary callable `Fn`.  The call operator forwards to `fn`.
 *
 * Prefer the `make_handler<Key>(fn)` factory over direct construction.
 *
 * @tparam Key  Compile-time string key.
 * @tparam Fn   Callable type (function, lambda, functor, ...).
 */
template <StringLiteral Key, class Fn>
struct Handler {
  /** The callable invoked on a successful match. */
  Fn fn;

  /** Returns the compile-time `StringLiteral` key. */
  [[nodiscard]] static constexpr auto literal() noexcept { return Key; }

  /** Returns a `std::string_view` of the key. */
  [[nodiscard]] static constexpr auto view() noexcept { return Key.view(); }

  /** Invokes the stored callable. */
  auto operator()() noexcept(noexcept(fn())) -> void { fn(); }
};

/**
 * @brief Factory function for constructing a `Handler<Key, Fn>`.
 *
 * @tparam Key  Compile-time string literal, e.g. `"help"`.
 * @param  fn   Callable to invoke on a match.
 * @return `Handler<Key, std::decay_t<Fn>>` wrapping `fn`.
 *
 * @code
 * auto h = make_handler<"help">([] { std::puts("help!"); });
 * @endcode
 */
template <StringLiteral Key, class Fn>
[[nodiscard]] constexpr auto make_handler(Fn&& fn)
    -> Handler<Key, std::decay_t<Fn>> {
  return {std::forward<Fn>(fn)};
}

// ============================================================================
// Internal trie machinery
// ============================================================================

// ============================================================================
// match() — the public entry point
// ============================================================================

// Forward declaration of the ref-tuple overload (used recursively).
template <std::size_t N, class... Entries>
META_MATCH_ALWAYS_INLINE auto match(std::string_view c,
                                    std::tuple<Entries&...> a)
    -> bool;

/**
 * @brief Dispatches `c` against the handlers in `a`.
 *
 * Accepts an lvalue reference to a `std::tuple` of `Handler` objects,
 * wraps them in references, and starts trie traversal at position 0.
 * A temporary tuple is also accepted through the rvalue overload below.
 *
 * @param c  Runtime string to match.
 * @param a  Tuple of `Handler<Key, Fn>` instances (created via `make_handler`).
 * @return   `true` if a handler was found and called; `false` otherwise.
 *
 * @code
 * auto handlers = std::tuple{
 *     make_handler<"help">   ([] { ... }),
 *     make_handler<"version">([] { ... }),
 * };
 * match("help"sv, handlers);     // calls "help" handler → true
 * match("unknown"sv, handlers);  // → false
 * @endcode
 */
template <class... Entries>
META_MATCH_ALWAYS_INLINE auto match(std::string_view c,
                                    std::tuple<Entries...>& a)
    -> bool {
  auto refs = std::apply(
      [](auto&... xs) -> auto { return std::forward_as_tuple(xs...); }, a);
  return match<0>(c, refs);
}

template <class... Entries>
META_MATCH_ALWAYS_INLINE auto match(std::string_view c,
                                    std::tuple<Entries...>&& a)
    -> bool {
  auto refs = std::apply(
      [](auto&... xs) -> auto { return std::forward_as_tuple(xs...); }, a);
  return match<0>(c, refs);
}

template <std::size_t N, char C, class... Entries>
consteval auto get_matched_array() {
  constexpr std::size_t matched_count =
      (((N < Entries::view().size()) && (Entries::literal()[N] == C)) + ... + 0);
  std::array<std::size_t, matched_count> result{};
  std::size_t index = 0;
  [&]<std::size_t... I>(std::index_sequence<I...>) -> auto {
    (((N < Entries::view().size() && Entries::literal()[N] == C)
          ? (result[index++] = I, void())
          : void()),
     ...);
  }(std::index_sequence_for<Entries...>{});

  return result;
}

template <std::size_t Count, std::array<std::size_t, Count> Indices,
          class... Entries>
constexpr auto get_next_candidates(std::tuple<Entries&...> a) {
  return [&]<std::size_t... I>(std::index_sequence<I...>) -> auto {
    return std::forward_as_tuple(std::get<Indices[I]>(a)...);
  }(std::make_index_sequence<Count>{});
}

/**
 * @brief Core trie-dispatch implementation (ref-tuple overload, depth N).
 *
 * At each recursion depth N the function:
 *  1. Reads `c[N]`.
 *  2. Switches on that character.
 *  3. Computes (at compile time) which handlers still have a matching byte at
 *     position `N`.
 *  4. If exactly one handler remains, verifies the full string and calls it.
 *  5. If multiple remain, recurses with `N + 1` and the surviving subset.
 *
 * Not called directly; use the value-tuple `match()` overload above.
 */
template <std::size_t N, class... Entries>
META_MATCH_ALWAYS_INLINE auto match(std::string_view c,
                                    std::tuple<Entries&...> a)
    -> bool {
  constexpr auto matched_number_table = [] -> auto {
    std::array<std::size_t, 0x7f + 1> result{};
    for (unsigned char c = 0x00; c <= 0x7f; ++c) {
      result[c] = (((Entries::literal()[N] == static_cast<char>(c)) ? 1U : 0U) +
                   ... + 0U);
    }
    return result;
  }();

  if (N == c.size()) {
    constexpr auto matched_number =
        ((Entries::view().size() == N ? 1U : 0U) + ... + 0U);

    if constexpr (matched_number == 0) {
      return false;
    } else {
      // Duplicate keys are rejected at compile time when the corresponding trie
      // terminal state is instantiated.
      static_assert(matched_number == 1, "duplicated meta::match key");

      bool called = false;

      [&]<std::size_t... I>(std::index_sequence<I...>) -> auto {
        (((std::tuple_element_t<I, std::tuple<Entries...>>::view().size() == N)
              ? (std::get<I>(a)(), called = true, void())
              : void()),
         ...);
      }(std::index_sequence_for<Entries...>{});

      return called;
    }
  }

// One switch-case per printable ASCII byte.
// At compile time the surviving handlers are narrowed; then either
// the unique match is verified+dispatched, or we recurse.
#define MM_SWITCH_CHAR(C)                                               \
  case C: {                                                             \
    constexpr auto matched_number =                                     \
        matched_number_table[static_cast<unsigned char>(C)];            \
    if constexpr (matched_number == 1) {                                \
      constexpr auto candidate_index =                                  \
          [&]<std::size_t... I>(std::index_sequence<I...>) -> auto {    \
        int i = 0;                                                      \
        ((Entries::literal()[N] == (C) && (i = I, true)) || ...);       \
        return i;                                                       \
      }(std::index_sequence_for<Entries...>{});                         \
      using candidate_type = std::remove_reference_t<                   \
          std::tuple_element_t<candidate_index, decltype(a)>>;          \
      constexpr auto expected = candidate_type::view();                 \
      if (c != expected) {                                              \
        return false;                                                   \
      }                                                                 \
      std::get<candidate_index>(a)();                                   \
      return true;                                                      \
    } else if constexpr (matched_number > 1) {                          \
      return match<N + 1>(                                              \
          c, get_next_candidates<matched_number,                        \
                                 get_matched_array<N, C, Entries...>(), \
                                 Entries...>(a));                       \
    }                                                                   \
    break;                                                              \
  }

  switch (static_cast<unsigned char>(c.data()[N])) {
    MM_SWITCH_CHAR(0x00);
    MM_SWITCH_CHAR(0x20) MM_SWITCH_CHAR(0x21) MM_SWITCH_CHAR(0x22);
    MM_SWITCH_CHAR(0x23) MM_SWITCH_CHAR(0x24) MM_SWITCH_CHAR(0x25);
    MM_SWITCH_CHAR(0x26) MM_SWITCH_CHAR(0x27) MM_SWITCH_CHAR(0x28);
    MM_SWITCH_CHAR(0x29) MM_SWITCH_CHAR(0x2a) MM_SWITCH_CHAR(0x2b);
    MM_SWITCH_CHAR(0x2c) MM_SWITCH_CHAR(0x2d) MM_SWITCH_CHAR(0x2e);
    MM_SWITCH_CHAR(0x2f) MM_SWITCH_CHAR(0x30) MM_SWITCH_CHAR(0x31);
    MM_SWITCH_CHAR(0x32) MM_SWITCH_CHAR(0x33) MM_SWITCH_CHAR(0x34);
    MM_SWITCH_CHAR(0x35) MM_SWITCH_CHAR(0x36) MM_SWITCH_CHAR(0x37);
    MM_SWITCH_CHAR(0x38) MM_SWITCH_CHAR(0x39) MM_SWITCH_CHAR(0x3a);
    MM_SWITCH_CHAR(0x3b) MM_SWITCH_CHAR(0x3c) MM_SWITCH_CHAR(0x3d);
    MM_SWITCH_CHAR(0x3e) MM_SWITCH_CHAR(0x3f) MM_SWITCH_CHAR(0x40);
    MM_SWITCH_CHAR(0x41) MM_SWITCH_CHAR(0x42) MM_SWITCH_CHAR(0x43);
    MM_SWITCH_CHAR(0x44) MM_SWITCH_CHAR(0x45) MM_SWITCH_CHAR(0x46);
    MM_SWITCH_CHAR(0x47) MM_SWITCH_CHAR(0x48) MM_SWITCH_CHAR(0x49);
    MM_SWITCH_CHAR(0x4a) MM_SWITCH_CHAR(0x4b) MM_SWITCH_CHAR(0x4c);
    MM_SWITCH_CHAR(0x4d) MM_SWITCH_CHAR(0x4e) MM_SWITCH_CHAR(0x4f);
    MM_SWITCH_CHAR(0x50) MM_SWITCH_CHAR(0x51) MM_SWITCH_CHAR(0x52);
    MM_SWITCH_CHAR(0x53) MM_SWITCH_CHAR(0x54) MM_SWITCH_CHAR(0x55);
    MM_SWITCH_CHAR(0x56) MM_SWITCH_CHAR(0x57) MM_SWITCH_CHAR(0x58);
    MM_SWITCH_CHAR(0x59) MM_SWITCH_CHAR(0x5a) MM_SWITCH_CHAR(0x5b);
    MM_SWITCH_CHAR(0x5c) MM_SWITCH_CHAR(0x5d) MM_SWITCH_CHAR(0x5e);
    MM_SWITCH_CHAR(0x5f) MM_SWITCH_CHAR(0x60) MM_SWITCH_CHAR(0x61);
    MM_SWITCH_CHAR(0x62) MM_SWITCH_CHAR(0x63) MM_SWITCH_CHAR(0x64);
    MM_SWITCH_CHAR(0x65) MM_SWITCH_CHAR(0x66) MM_SWITCH_CHAR(0x67);
    MM_SWITCH_CHAR(0x68) MM_SWITCH_CHAR(0x69) MM_SWITCH_CHAR(0x6a);
    MM_SWITCH_CHAR(0x6b) MM_SWITCH_CHAR(0x6c) MM_SWITCH_CHAR(0x6d);
    MM_SWITCH_CHAR(0x6e) MM_SWITCH_CHAR(0x6f) MM_SWITCH_CHAR(0x70);
    MM_SWITCH_CHAR(0x71) MM_SWITCH_CHAR(0x72) MM_SWITCH_CHAR(0x73);
    MM_SWITCH_CHAR(0x74) MM_SWITCH_CHAR(0x75) MM_SWITCH_CHAR(0x76);
    MM_SWITCH_CHAR(0x77) MM_SWITCH_CHAR(0x78) MM_SWITCH_CHAR(0x79);
    MM_SWITCH_CHAR(0x7a) MM_SWITCH_CHAR(0x7b) MM_SWITCH_CHAR(0x7c);
    MM_SWITCH_CHAR(0x7d) MM_SWITCH_CHAR(0x7e) MM_SWITCH_CHAR(0x7f);
    default:
      break;
  }
#undef MM_SWITCH_CHAR
  return false;
}

}  // namespace meta_match
