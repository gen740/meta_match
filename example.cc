/**
 * meta_match — usage example
 *
 * Shows how to build a string dispatcher using make_handler<Key>(fn)
 * and a std::tuple of Handler objects.
 *
 * Build & run:
 *   make example
 *   ./example
 */

#include <iostream>
#include <string_view>
#include <tuple>

#include "meta_match.hh"

using namespace meta_match;
using namespace std::literals;

// ---------------------------------------------------------------------------
// Example 1: simple command dispatcher
// ---------------------------------------------------------------------------
void example_commands() {
  std::cout << "=== Example 1: command dispatcher ===\n";

  int help_count = 0, version_count = 0, quit_count = 0;

  auto handlers = std::tuple{
      make_handler<"help">   ([&]{ ++help_count;    std::cout << "  → help\n";    }),
      make_handler<"version">([&]{ ++version_count; std::cout << "  → version\n"; }),
      make_handler<"quit">   ([&]{ ++quit_count;    std::cout << "  → quit\n";    }),
  };

  for (std::string_view sv : {"help"sv, "version"sv, "help"sv, "bad"sv, "quit"sv}) {
    std::cout << '"' << sv << "\" → " << (match(sv, handlers) ? "hit" : "miss") << '\n';
  }

  std::cout << "counts: help=" << help_count
            << " version=" << version_count
            << " quit="    << quit_count << "\n\n";
}

// ---------------------------------------------------------------------------
// Example 2: HTTP method router
// ---------------------------------------------------------------------------
void example_http() {
  std::cout << "=== Example 2: HTTP method router ===\n";

  int get_n = 0, post_n = 0;

  auto methods = std::tuple{
      make_handler<"GET">    ([&]{ ++get_n;  }),
      make_handler<"POST">   ([&]{ ++post_n; }),
      make_handler<"PUT">    ([&]{ std::cout << "  PUT\n";    }),
      make_handler<"DELETE"> ([&]{ std::cout << "  DELETE\n"; }),
      make_handler<"PATCH">  ([&]{ std::cout << "  PATCH\n";  }),
      make_handler<"OPTIONS">([&]{ std::cout << "  OPTIONS\n";}),
  };

  for (std::string_view m : {"GET"sv, "POST"sv, "GET"sv, "DELETE"sv, "HEAD"sv}) {
    const bool ok = match(m, methods);
    std::cout << m << " → " << (ok ? "routed" : "405") << '\n';
  }
  std::cout << "GET=" << get_n << " POST=" << post_n << "\n\n";
}

// ---------------------------------------------------------------------------
// Example 3: shared prefix disambiguation ("set" / "setx" / "setenv")
// ---------------------------------------------------------------------------
void example_prefix() {
  std::cout << "=== Example 3: shared prefix (set / setx / setenv) ===\n";

  auto cmds = std::tuple{
      make_handler<"set">   ([&]{ std::cout << "  matched: set\n";    }),
      make_handler<"setx">  ([&]{ std::cout << "  matched: setx\n";   }),
      make_handler<"setenv">([&]{ std::cout << "  matched: setenv\n"; }),
  };

  for (std::string_view sv : {"set"sv, "setx"sv, "setenv"sv, "sete"sv, "se"sv}) {
    std::cout << '"' << sv << "\" → ";
    if (!match(sv, cmds)) std::cout << "  no match\n";
  }
  std::cout << '\n';
}

int main() {
  example_commands();
  example_http();
  example_prefix();
}
