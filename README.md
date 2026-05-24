# MetaMatch

`meta_match` is a header-only modern C++ string dispatcher that turns a fixed key set
into a compile-time character trie and emits direct `switch`-based code.

`meta_match` itself is fundamentally a C++20 design. This repository builds the
benchmark suite as C++23 because `glaze`, one of the comparison targets, is
C++23 in this setup.

## Summary

`meta_match` is a compact compile-time trie that avoids linear scaling and has
different failure modes from hash construction.

- It compiles substantially faster than `glaze` in the current benchmark set.
- For the 10-case and 50-case runtime datasets in this repository, `meta_match`
  stays below `5 ns` in every measured mode.
- The implementation remains compact: it is header-only, lives in a single file,
  and the dispatch implementation itself stays under roughly 250 lines.
- The strongest area is small-to-medium static key sets where the first few
  characters eliminate most candidates and runtime stability matters more than
  absolute best-case miss latency.
- `meta_match` is not a universal replacement for perfect hashing. For larger
  key sets on hash-friendly distributions, a good perfect hash such as `glaze`
  will win on raw throughput.

These numbers are not intended as a universal ranking. They describe the
current benchmark corpus, compiler, optimization settings, and machine. The
goal is to show the shape of the trade-off rather than claim that one dispatcher
dominates all key distributions.

## Benchmarks

### Method

The repository measures both compile-time and runtime.

Compile-time is measured with `hyperfine`. Each implementation is compiled in
isolation through a dedicated translation unit:

- `_ct_meta.cc`
- `_ct_if.cc`
- `_ct_glaze.cc`

Those compile-time drivers use the string set `"00"` ... `"99"`, so the compiler
always sees exactly 100 registered keys.

Runtime is measured with Google Benchmark against several dataset families:

- `http_methods_10`: realistic short protocol tokens
- `json_fields_10`: reflection / object field names
- `first_byte_distinct_10`: trie-favorable early split
- `tail_distinct_10`: trie-unfavorable late split
- `http_headers_50`: a larger realistic 50-key set
- `hash_prefix_4byte_255`: 255 synthetic 4-byte keys, hash-friendly distribution

An additional `prefix_family_10` dataset is used to study terminal-state
behavior separately from the main non-prefix fast path.

The benchmark suite intentionally covers:

- realistic keys
- trie-favorable keys
- trie-unfavorable keys
- miss-heavy cases
- a 50-key realistic set
- a 255-key large set to observe how each approach scales

The benchmarked fast path is optimized for non-prefix key sets: no registered
key is a proper prefix of another registered key.

For non-prefix key sets, `meta_match` can keep dispatching while multiple trie
candidates remain, and then verify the final candidate with full
`std::string_view` equality. Prefix-sharing key sets such as `set`, `setenv`,
and `setup` require a different terminal strategy.

There are two reasonable terminal strategies:

1. Explicit terminal-state handling:
   safe for arbitrary `std::string_view`, including prefix-sharing key sets.
2. Null-terminator fast path:
   faster and simpler, but only valid when inputs are backed by null-terminated
   storage and the implementation is allowed to read the terminating byte.

The current benchmark suite focuses on the main non-prefix fast path and keeps
the terminal-strategy question separate from that core comparison.

### Runtime

The runtime picture across small and medium key sets:

- `meta_match` is consistently fast across all 10-case and 50-case datasets.
- `meta_match` remains below `5 ns` in every measured runtime cell for those sets.
- `glaze` wins some miss-heavy or tail-distinct cases, but loses most mixed or
  realistic cases in this suite.
- `if_match` is very competitive for tiny sets. The compiler reduces the linear
  fold to a size-based jump followed by 8-byte word comparisons, so individual
  key checks are cheap. The cost grows linearly with key count, however, and
  once the set is large enough that cost overtakes the trie or hash overhead,
  `if_match` is no longer competitive.

From `artifacts/bench_report/runtime_summary.json`:

- `http_headers_50`, hits only:
  `meta_match 2.89 ns`, `if_match 2.38 ns`, `glaze 4.88 ns`
- `json_fields_10`, hits only:
  `meta_match 1.53 ns`, `if_match 1.82 ns`, `glaze 5.69 ns`
- `first_byte_distinct_10`, hits only:
  `meta_match 1.56 ns`, `if_match 1.95 ns`, `glaze 1.75 ns`
- `tail_distinct_10`, hits only:
  `meta_match 2.12 ns`, `if_match 2.05 ns`, `glaze 1.75 ns`

That shape matches the design:

- early character separation helps `meta_match`
- late separation helps hashing more
- realistic 10-to-50 key sets are often favorable to `meta_match`

#### Scaling to 255 keys

The `hash_prefix_4byte_255` dataset uses 255 synthetic 4-byte keys on a
hash-friendly distribution. Results (hits / mixed / misses):

| implementation | hits     | mixed    | misses   |
|----------------|----------|----------|----------|
| `meta_match`   | 3.77 ns  | 3.76 ns  | 1.60 ns  |
| `if_match`     | 20.4 ns  | 20.7 ns  | 1.24 ns  |
| `glaze`        | 2.12 ns  | 2.13 ns  | 0.836 ns |

At 255 keys, the linear fold degrades to ~20 ns on hit paths. `meta_match` stays around 3.8 ns,
approximately 5× faster than `if_match`, because the trie collapses the search
space after the first few characters regardless of key count. `glaze` is the
fastest on this distribution at ~2.1 ns, showing the expected strength of a
well-constructed perfect hash when the key set is hash-friendly.

For larger key sets, the linear fold eventually stops being competitive. Each
key comparison is cheap — the compiler emits a size-based jump then 8-byte word
comparisons — but those checks accumulate linearly with key count. In the
255-key synthetic 4-byte dataset, `if_match` takes about 20 ns on hit paths,
while `meta_match` stays around 3.8 ns because the trie prunes the candidate
set in the first few character steps regardless of total key count. `glaze` is
faster still on this particular distribution, around 2.1 ns, showing the
expected strength of a good perfect-hash strategy. This is the intended
positioning: `meta_match` is not a universal replacement for perfect hashing.
It is a compact compile-time trie that avoids linear scaling and has different
failure modes from hash construction.

![Runtime benchmark](artifacts/bench_report/runtime_benchmark.svg)

### Compile-time

The compile-time result supports the main claim in the summary: `meta_match` is
heavier than a linear fold, but clearly lighter than `glaze` in this repository.

From `artifacts/bench_report/compile_summary.json`:

- `-fsyntax-only`:
  `meta_match 0.712 ± 0.019 s`, `if_match 0.134 ± 0.002 s`, `glaze 1.120 ± 0.012 s`
- `-c -O1`:
  `meta_match 0.863 ± 0.004 s`, `if_match 0.163 ± 0.001 s`, `glaze 1.186 ± 0.007 s`
- `-c -O3`:
  `meta_match 0.916 ± 0.003 s`, `if_match 0.164 ± 0.002 s`, `glaze 1.196 ± 0.007 s`

So the practical reading is:

- `if_match` is still the cheapest thing to compile
- `meta_match` is the middle ground
- `glaze` is the most expensive frontend path here

![Compile-time benchmark](artifacts/bench_report/compile_time.svg)

## How It Works

The dispatcher is generated as a recursive compile-time trie.

One practical advantage of the design is that it stays small. The implementation
is header-only and concentrated in a single file rather than spread across a code
generator, runtime trie structure, and auxiliary tables.

At depth `N`, the implementation computes which handlers still match the byte
at position `N`, then emits one `switch` case per ASCII byte:

```cpp
switch (static_cast<unsigned char>(c.data()[N])) {
  case 'g': ...
  case 's': ...
  case 'v': ...
}
```

For each case:

1. A compile-time table gives the number of surviving candidates.
2. If there is exactly one survivor, the code verifies the full string and calls
   the handler directly.
3. If there are multiple survivors, it recurses to the next character with a
   reduced tuple of candidates.

The important point is that this is not a runtime trie stored in memory. The
trie exists as constexpr/template structure during compilation, and the runtime
artifact is direct branch code.

That is why `meta_match` is good at:

- small or medium fixed key sets
- datasets with useful early character separation
- latency-sensitive dispatch where a few predictable branches beat hashing

## Discussion

`glaze` and `meta_match` solve a similar problem with very different trade-offs.

`glaze` builds a compact hash-based dispatch path. In the implementation path used
here, its `hash_info` machinery uses fixed bucketed tables, and the local source
contains 256-entry structures in this area of the algorithm
(`glaze/core/reflect.hpp`, e.g. `bucket_size(...) -> 256` and related 256-entry
arrays). That is a very different shape from `meta_match`, which does not have a
global bucket-count limit and instead grows with the actual trie frontier.

This difference shows up in the benchmark suite:

- `glaze` is strong when the whole key has to be considered anyway, or when the
  key distribution is hash-friendly and the set is large
- `meta_match` is strong when a few early characters collapse the search space,
  and avoids linear scaling that afflicts the `if` chain at 100+ keys

Another difference is robustness across pathological key families. Prefix ladders
such as:

```text
a
aa
aaa
aaaa
...
```

are awkward for hash-based front-byte discrimination and can become hostile to
some perfect-hash construction strategies. `meta_match` does not need a perfect
hash at all. Its current fast implementation imposes a different constraint:
prefix-sharing families require either explicit terminal-state handling or a
null-terminator-aware fast path.

In other words:

- `glaze` is excellent when its selected hash strategy matches the key set, but
  its performance profile depends on which compile-time hash strategy is selected
- `meta_match` can keep a very small and direct dispatch path for non-prefix
  key sets, and can be extended with explicit terminal handling when prefix
  sharing matters

The second limitation is easier to relax in a controlled way. Reintroducing an
explicit terminal state is mechanically straightforward, and only trades some of
the current fast-path simplicity for broader key-set support.

## Minimal API

```cpp
#include "meta_match.hh"

auto handlers = std::tuple{
    make_handler<"help">([] { show_help(); }),
    make_handler<"quit">([] { do_quit(); }),
    make_handler<"version">([] { show_version(); }),
};

bool ok = match(input, handlers);
```

## Build

```sh
nix develop
make
make bench
make compile-time
make bench-report
```

`make bench-report` writes the JSON summaries and SVG graphs to
[`artifacts/bench_report`](artifacts/bench_report).
