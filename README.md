# MetaMatch

`meta_match` is a header-only modern C++ string dispatcher that turns a fixed key set
into a compile-time character trie and emits direct `switch`-based code.

`meta_match` itself is fundamentally a **C++20** design.[^cxx-version]

[^cxx-version]: This repository builds the benchmark suite as C++23 because
`glaze`, one of the comparison targets in this setup, requires C++23.

## Summary

`meta_match` is a compact compile-time trie that avoids linear scaling and has
different failure modes from hash construction.

- It compiles substantially faster than `glaze` in the current benchmark set.
- On realistic 10-key and 50-key datasets in this repository, `meta_match`
  stays in the low single-digit nanoseconds and usually beats `glaze`.
- In the worst runtime cases observed when running this benchmark suite,
  `meta_match` stays below `6 ns`.
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

## Use Cases

`meta_match` fits best when the key set is fixed at compile time and dispatch
latency matters more than universal optimality.

- CLI command dispatch such as `help`, `version`, and `quit`
- JSON / config field routing for a known schema
- Protocol token dispatch such as HTTP methods or header names
- Embedded or low-allocation code paths where a header-only direct dispatcher is
  preferable to a runtime hash table
- Small-to-medium registries where early characters separate keys well

It is a weaker fit when the key set is large and hash-friendly, or when dynamic
registration is required.

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

In the worst structural case for the trie, compile-time work grows linearly with
the input key length because trie construction has to keep recursing until the
distinguishing character depth.

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

The current implementation uses explicit terminal-state handling, so
prefix-sharing key sets work with arbitrary `std::string_view` inputs and do
not require null-terminated backing storage.

### Runtime

The runtime picture across small and medium key sets:

- `meta_match` is consistently fast across all 10-case and 50-case datasets.
- `meta_match` stays around `2.2 ns` to `4.5 ns` on the realistic 10-key and
  50-key datasets in this suite.
- Across all runtime datasets in the current benchmark run, `meta_match` stays
  below `6 ns`.
- `glaze` wins some miss-heavy or tail-distinct cases, but loses most mixed or
  realistic cases in this suite.
- `if_match` is very competitive for tiny sets. The compiler reduces the linear
  fold to a size-based jump followed by 8-byte word comparisons, so individual
  key checks are cheap. The cost grows linearly with key count, however, and
  once the set is large enough that cost overtakes the trie or hash overhead,
  `if_match` is no longer competitive.

From `artifacts/bench_report/runtime_summary.json`:

- `http_headers_50`, hits only:
  `meta_match 4.30 ns`, `if_match 3.44 ns`, `glaze 7.06 ns`
- `json_fields_10`, hits only:
  `meta_match 2.22 ns`, `if_match 2.74 ns`, `glaze 8.16 ns`
- `first_byte_distinct_10`, hits only:
  `meta_match 2.17 ns`, `if_match 2.77 ns`, `glaze 2.53 ns`
- `tail_distinct_10`, hits only:
  `meta_match 5.16 ns`, `if_match 2.93 ns`, `glaze 2.54 ns`

That shape matches the design:

- early character separation helps `meta_match`
- late separation helps hashing more
- realistic 10-to-50 key sets are often favorable to `meta_match`, but
  tail-distinct families are not

#### Scaling to 255 keys

The `hash_prefix_4byte_255` dataset uses 255 synthetic 4-byte keys on a
hash-friendly distribution. Results (hits / mixed / misses):

| implementation | hits     | mixed    | misses   |
|----------------|----------|----------|----------|
| `meta_match`   | 5.46 ns  | 5.55 ns  | 3.35 ns  |
| `if_match`     | 29.4 ns  | 29.9 ns  | 54.9 ns  |
| `glaze`        | 2.69 ns  | 2.68 ns  | 1.20 ns  |

At 255 keys, the linear fold degrades to ~29 ns on hit paths. `meta_match` stays around 5.5 ns,
approximately 5× faster than `if_match`, because the trie collapses the search
space after the first few characters regardless of key count. `glaze` is the
fastest on this distribution at ~2.7 ns, showing the expected strength of a
well-constructed perfect hash when the key set is hash-friendly.

For larger key sets, the linear fold eventually stops being competitive. Each
key comparison is cheap — the compiler emits a size-based jump then 8-byte word
comparisons — but those checks accumulate linearly with key count. In the
255-key synthetic 4-byte dataset, `if_match` takes about 29 ns on hit paths,
while `meta_match` stays around 5.5 ns because the trie prunes the candidate
set in the first few character steps regardless of total key count. `glaze` is
faster still on this particular distribution, around 2.7 ns, showing the
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
  `meta_match 1.048 ± 0.013 s`, `if_match 0.198 ± 0.003 s`, `glaze 1.691 ± 0.008 s`
- `-c -O1`:
  `meta_match 1.273 ± 0.016 s`, `if_match 0.231 ± 0.003 s`, `glaze 1.769 ± 0.007 s`
- `-c -O3`:
  `meta_match 1.373 ± 0.021 s`, `if_match 0.247 ± 0.012 s`, `glaze 1.818 ± 0.019 s`

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
prefix-sharing families require explicit terminal-state handling.

In other words:

- `glaze` is excellent when its selected hash strategy matches the key set, but
  its performance profile depends on which compile-time hash strategy is selected
- `meta_match` can keep a very small and direct dispatch path for non-prefix
  key sets, and can be extended with explicit terminal handling when prefix
  sharing matters

That broader key-set support is now part of the implementation: prefix-sharing
inputs are handled through explicit terminal-state checks on ordinary
`std::string_view` data.

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
