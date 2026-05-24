#include <benchmark/benchmark.h>

#include <array>
#include <cstdint>
#include <glaze/core/opts.hpp>
#include <glaze/core/reflect.hpp>
#include <string_view>
#include <tuple>
#include <type_traits>

#include "meta_match.hh"

namespace {

using namespace std::literals;
constexpr auto kCaseNames = std::array{
    "hits_only_randomized",
    "hit_miss_randomized",
    "miss_only",
};

constexpr std::size_t kSequenceSize = 1024;
constexpr int kModes = static_cast<int>(kCaseNames.size() - 1);

template <std::size_t N>
consteval auto literal_sv(const char (&str)[N]) -> std::string_view {
  return {str, N - 1};
}

template <std::size_t N>
consteval auto max_sv_size(const std::array<std::string_view, N>& values)
    -> std::size_t {
  std::size_t max_size = 0;
  for (const auto value : values) {
    if (value.size() > max_size) {
      max_size = value.size();
    }
  }
  return max_size;
}

// ============================================================================
// Benchmark callable — tracks hit count and mixes internal state
// (prevents the compiler from optimising away calls to the handler)
// ============================================================================

template <StringLiteral Key>
struct BenchFn {
  int hit_count = 0;
  std::uint64_t state =
      0x9e3779b97f4a7c15ULL ^ static_cast<std::uint64_t>(Key[0]);

  void operator()() noexcept {
    ++hit_count;
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    state += static_cast<std::uint64_t>(Key.size()) * 0xbf58476d1ce4e5b9ULL;
  }
};

#define HTTP_METHODS_10(X) \
  X(0, "GET")              \
  X(1, "POST")             \
  X(2, "PUT")              \
  X(3, "PATCH")            \
  X(4, "DELETE")           \
  X(5, "HEAD")             \
  X(6, "OPTIONS")          \
  X(7, "CONNECT")          \
  X(8, "TRACE")            \
  X(9, "PRI")

#define JSON_FIELDS_10(X) \
  X(0, "id")              \
  X(1, "name")            \
  X(2, "email")           \
  X(3, "created_at")      \
  X(4, "updated_at")      \
  X(5, "deleted_at")      \
  X(6, "status")          \
  X(7, "type")            \
  X(8, "version")         \
  X(9, "metadata")

#define PREFIX_FAMILY_10(X) \
  X(0, "get")               \
  X(1, "gets")              \
  X(2, "getenv")            \
  X(3, "getter")            \
  X(4, "getopt")            \
  X(5, "set")               \
  X(6, "sets")              \
  X(7, "setenv")            \
  X(8, "setter")            \
  X(9, "setopt")

#define FIRST_BYTE_DISTINCT_10(X) \
  X(0, "a000")                    \
  X(1, "b000")                    \
  X(2, "c000")                    \
  X(3, "d000")                    \
  X(4, "e000")                    \
  X(5, "f000")                    \
  X(6, "g000")                    \
  X(7, "h000")                    \
  X(8, "i000")                    \
  X(9, "j000")

#define TAIL_DISTINCT_10(X) \
  X(0, "aaaaaaaaaa0")       \
  X(1, "aaaaaaaaaa1")       \
  X(2, "aaaaaaaaaa2")       \
  X(3, "aaaaaaaaaa3")       \
  X(4, "aaaaaaaaaa4")       \
  X(5, "aaaaaaaaaa5")       \
  X(6, "aaaaaaaaaa6")       \
  X(7, "aaaaaaaaaa7")       \
  X(8, "aaaaaaaaaa8")       \
  X(9, "aaaaaaaaaa9")

#define HTTP_HEADERS_50(X)                 \
  X(0, "Accept")                           \
  X(1, "Accept-Charset")                   \
  X(2, "Accept-Encoding")                  \
  X(3, "Accept-Language")                  \
  X(4, "Accept-Ranges")                    \
  X(5, "Access-Control-Allow-Credentials") \
  X(6, "Access-Control-Allow-Headers")     \
  X(7, "Access-Control-Allow-Methods")     \
  X(8, "Access-Control-Allow-Origin")      \
  X(9, "Access-Control-Expose-Headers")    \
  X(10, "Age")                             \
  X(11, "Allow")                           \
  X(12, "Authorization")                   \
  X(13, "Cache-Control")                   \
  X(14, "Connection")                      \
  X(15, "Content-Disposition")             \
  X(16, "Content-Encoding")                \
  X(17, "Content-Language")                \
  X(18, "Content-Length")                  \
  X(19, "Content-Location")                \
  X(20, "Content-Range")                   \
  X(21, "Content-Security-Policy")         \
  X(22, "Content-Type")                    \
  X(23, "Cookie")                          \
  X(24, "Date")                            \
  X(25, "ETag")                            \
  X(26, "Expect")                          \
  X(27, "Expires")                         \
  X(28, "Forwarded")                       \
  X(29, "From")                            \
  X(30, "Host")                            \
  X(31, "If-Match")                        \
  X(32, "If-Modified-Since")               \
  X(33, "If-None-Match")                   \
  X(34, "If-Range")                        \
  X(35, "If-Unmodified-Since")             \
  X(36, "Last-Modified")                   \
  X(37, "Location")                        \
  X(38, "Origin")                          \
  X(39, "Pragma")                          \
  X(40, "Range")                           \
  X(41, "Referer")                         \
  X(42, "Retry-After")                     \
  X(43, "Server")                          \
  X(44, "Set-Cookie")                      \
  X(45, "TE")                              \
  X(46, "Trailer")                         \
  X(47, "Transfer-Encoding")               \
  X(48, "Upgrade")                         \
  X(49, "User-Agent")

#define HASH_PREFIX_4byte_255(X) \
  X(0, "F886")                   \
  X(1, "B347")                   \
  X(2, "5CBF")                   \
  X(3, "90A7")                   \
  X(4, "B2ED")                   \
  X(5, "4711")                   \
  X(6, "FECF")                   \
  X(7, "0BDA")                   \
  X(8, "29C4")                   \
  X(9, "F983")                   \
  X(10, "7EF6")                  \
  X(11, "74E4")                  \
  X(12, "BA1F")                  \
  X(13, "89D9")                  \
  X(14, "31A7")                  \
  X(15, "9A39")                  \
  X(16, "0FF0")                  \
  X(17, "85F1")                  \
  X(18, "9245")                  \
  X(19, "7A68")                  \
  X(20, "3513")                  \
  X(21, "E784")                  \
  X(22, "274B")                  \
  X(23, "D858")                  \
  X(24, "7D1C")                  \
  X(25, "5F26")                  \
  X(26, "70C6")                  \
  X(27, "3C38")                  \
  X(28, "3BF5")                  \
  X(29, "AA04")                  \
  X(30, "5A45")                  \
  X(31, "2957")                  \
  X(32, "6849")                  \
  X(33, "F3E0")                  \
  X(34, "B6AF")                  \
  X(35, "78DE")                  \
  X(36, "FE28")                  \
  X(37, "AFDE")                  \
  X(38, "BADF")                  \
  X(39, "D4BA")                  \
  X(40, "64C5")                  \
  X(41, "3DC8")                  \
  X(42, "0DBB")                  \
  X(43, "FC3D")                  \
  X(44, "8D17")                  \
  X(45, "7B60")                  \
  X(46, "861E")                  \
  X(47, "2113")                  \
  X(48, "1C7C")                  \
  X(49, "2926")                  \
  X(50, "BE17")                  \
  X(51, "D9DE")                  \
  X(52, "74C0")                  \
  X(53, "A2EE")                  \
  X(54, "7306")                  \
  X(55, "7BF0")                  \
  X(56, "8E2C")                  \
  X(57, "9310")                  \
  X(58, "0EB0")                  \
  X(59, "75DA")                  \
  X(60, "8092")                  \
  X(61, "6A7B")                  \
  X(62, "6EAD")                  \
  X(63, "52DD")                  \
  X(64, "2714")                  \
  X(65, "809B")                  \
  X(66, "E18D")                  \
  X(67, "26DD")                  \
  X(68, "930E")                  \
  X(69, "3369")                  \
  X(70, "55ED")                  \
  X(71, "D33A")                  \
  X(72, "E222")                  \
  X(73, "53C7")                  \
  X(74, "9035")                  \
  X(75, "15F1")                  \
  X(76, "D694")                  \
  X(77, "973E")                  \
  X(78, "1803")                  \
  X(79, "E9B6")                  \
  X(80, "A40D")                  \
  X(81, "E896")                  \
  X(82, "1CE3")                  \
  X(83, "7B4A")                  \
  X(84, "2A34")                  \
  X(85, "ED5F")                  \
  X(86, "6098")                  \
  X(87, "8626")                  \
  X(88, "470C")                  \
  X(89, "372D")                  \
  X(90, "704E")                  \
  X(91, "E425")                  \
  X(92, "E949")                  \
  X(93, "E181")                  \
  X(94, "DBB1")                  \
  X(95, "999E")                  \
  X(96, "ECE7")                  \
  X(97, "6781")                  \
  X(98, "5C13")                  \
  X(99, "3B52")                  \
  X(100, "423F")                 \
  X(101, "EB09")                 \
  X(102, "BAE9")                 \
  X(103, "B3C2")                 \
  X(104, "B2B8")                 \
  X(105, "B9D1")                 \
  X(106, "E9B5")                 \
  X(107, "F5B0")                 \
  X(108, "1708")                 \
  X(109, "3247")                 \
  X(110, "F989")                 \
  X(111, "5011")                 \
  X(112, "B1B5")                 \
  X(113, "2C03")                 \
  X(114, "6379")                 \
  X(115, "F643")                 \
  X(116, "F715")                 \
  X(117, "5D8D")                 \
  X(118, "AE82")                 \
  X(119, "EEB3")                 \
  X(120, "19EC")                 \
  X(121, "3228")                 \
  X(122, "EE66")                 \
  X(123, "B626")                 \
  X(124, "11AE")                 \
  X(125, "4D5B")                 \
  X(126, "073E")                 \
  X(127, "2898")                 \
  X(128, "EAAE")                 \
  X(129, "EA67")                 \
  X(130, "2D9C")                 \
  X(131, "8195")                 \
  X(132, "8BFD")                 \
  X(133, "0D41")                 \
  X(134, "D94C")                 \
  X(135, "0CA7")                 \
  X(136, "B1C4")                 \
  X(137, "DDBC")                 \
  X(138, "20D7")                 \
  X(139, "8447")                 \
  X(140, "3073")                 \
  X(141, "C0E6")                 \
  X(142, "292B")                 \
  X(143, "6EAE")                 \
  X(144, "C46A")                 \
  X(145, "6D6F")                 \
  X(146, "0238")                 \
  X(147, "98CC")                 \
  X(148, "F977")                 \
  X(149, "DCED")                 \
  X(150, "6E35")                 \
  X(151, "2B7A")                 \
  X(152, "E936")                 \
  X(153, "5038")                 \
  X(154, "39DD")                 \
  X(155, "FD27")                 \
  X(156, "99ED")                 \
  X(157, "3D43")                 \
  X(158, "9D55")                 \
  X(159, "7826")                 \
  X(160, "371C")                 \
  X(161, "9752")                 \
  X(162, "3682")                 \
  X(163, "5773")                 \
  X(164, "B75C")                 \
  X(165, "5492")                 \
  X(166, "4EBD")                 \
  X(167, "6111")                 \
  X(168, "3DB0")                 \
  X(169, "FE13")                 \
  X(170, "297F")                 \
  X(171, "3A7A")                 \
  X(172, "C9D6")                 \
  X(173, "E458")                 \
  X(174, "CDE1")                 \
  X(175, "333F")                 \
  X(176, "0AF1")                 \
  X(177, "4AE0")                 \
  X(178, "D166")                 \
  X(179, "5CC5")                 \
  X(180, "465B")                 \
  X(181, "4705")                 \
  X(182, "8B54")                 \
  X(183, "2473")                 \
  X(184, "4533")                 \
  X(185, "9C46")                 \
  X(186, "7774")                 \
  X(187, "2C99")                 \
  X(188, "067A")                 \
  X(189, "A373")                 \
  X(190, "E133")                 \
  X(191, "6934")                 \
  X(192, "3DF3")                 \
  X(193, "A7B6")                 \
  X(194, "C556")                 \
  X(195, "3BBE")                 \
  X(196, "4129")                 \
  X(197, "7BF9")                 \
  X(198, "1D95")                 \
  X(199, "FFDF")                 \
  X(200, "A073")                 \
  X(201, "9C4B")                 \
  X(202, "3829")                 \
  X(203, "0C3F")                 \
  X(204, "6547")                 \
  X(205, "D3EC")                 \
  X(206, "FC55")                 \
  X(207, "2E3F")                 \
  X(208, "A2DF")                 \
  X(209, "B7A1")                 \
  X(210, "7DAE")                 \
  X(211, "59E9")                 \
  X(212, "A314")                 \
  X(213, "9B44")                 \
  X(214, "7374")                 \
  X(215, "9213")                 \
  X(216, "E6C7")                 \
  X(217, "3141")                 \
  X(218, "B1DC")                 \
  X(219, "2003")                 \
  X(220, "02C2")                 \
  X(221, "CE01")                 \
  X(222, "484E")                 \
  X(223, "60FF")                 \
  X(224, "3E5E")                 \
  X(225, "69E8")                 \
  X(226, "E0CA")                 \
  X(227, "CBC8")                 \
  X(228, "996A")                 \
  X(229, "FB52")                 \
  X(230, "57DA")                 \
  X(231, "CE85")                 \
  X(232, "5A18")                 \
  X(233, "2101")                 \
  X(234, "2FAF")                 \
  X(235, "5C82")                 \
  X(236, "AF06")                 \
  X(237, "18E3")                 \
  X(238, "6F53")                 \
  X(239, "ABE4")                 \
  X(240, "BBE9")                 \
  X(241, "36FB")                 \
  X(242, "C0AB")                 \
  X(243, "0AD0")                 \
  X(244, "B562")                 \
  X(245, "1712")                 \
  X(246, "A99E")                 \
  X(247, "E5B0")                 \
  X(248, "2AF1")                 \
  X(249, "A1CE")                 \
  X(250, "8D40")                 \
  X(251, "9367")                 \
  X(252, "91E8")                 \
  X(253, "E1CD")                 \
  X(254, "97AA")

#define COMMAND_AS_SV(_, literal) literal_sv(literal),
#define COMMAND_AS_HANDLER(_, literal) make_handler<literal>(BenchFn<literal>{}),
#define COMMAND_AS_CASE(index, literal) \
  case index:                           \
    std::get<index>(commands)();        \
    return;

#define DEFINE_DATASET(TypeName, Count, ListMacro, Miss0, Miss1, Miss2, Miss3) \
  struct TypeName {                                                            \
    inline static constexpr std::size_t kN = Count;                            \
    inline static constexpr std::array<std::string_view, kN> kCommands{        \
        ListMacro(COMMAND_AS_SV)};                                             \
    inline static constexpr std::array<std::string_view, 4> kMissValues{       \
        literal_sv(Miss0), literal_sv(Miss1), literal_sv(Miss2),               \
        literal_sv(Miss3)};                                                    \
    inline static constexpr std::size_t kMaxInputSize =                        \
        max_sv_size(kCommands) > max_sv_size(kMissValues)                      \
            ? max_sv_size(kCommands)                                           \
            : max_sv_size(kMissValues);                                        \
                                                                               \
    using commands_t = decltype(std::tuple{ListMacro(COMMAND_AS_HANDLER)});    \
                                                                               \
    static auto make_commands() -> commands_t {                                \
      return std::tuple{ListMacro(COMMAND_AS_HANDLER)};                        \
    }                                                                          \
    using glaze_keys_t = glz::keys_wrapper<kCommands>;                         \
    inline static constexpr auto& kGlazeHashInfo =                             \
        glz::hash_info<glaze_keys_t>;                                          \
                                                                               \
    template <class Commands>                                                  \
    static inline __attribute__((always_inline)) void invoke_by_index(         \
        std::size_t index, Commands& commands) {                               \
      switch (index) { ListMacro(COMMAND_AS_CASE) default : return; }          \
    }                                                                          \
  }

DEFINE_DATASET(HttpMethods10, 10, HTTP_METHODS_10, "POZT", "OPTION", "GETTING",
               "XGET");
DEFINE_DATASET(JsonFields10, 10, JSON_FIELDS_10, "emali", "created_att", "meta",
               "metadata_extra");
DEFINE_DATASET(PrefixFamily10, 10, PREFIX_FAMILY_10, "ge", "gete", "setting",
               "setenvx");
DEFINE_DATASET(FirstByteDistinct10, 10, FIRST_BYTE_DISTINCT_10, "k000", "a001",
               "a0000", "000a");
DEFINE_DATASET(TailDistinct10, 10, TAIL_DISTINCT_10, "aaaaaaaaaaX", "aaaaaaaaaa",
               "aaaaaaaaaa00", "baaaaaaaaa0");
DEFINE_DATASET(HttpHeaders50, 50, HTTP_HEADERS_50, "X-Definitely-Not-A-Header",
               "Content-Security-Policx", "Content", "Content-Type-Extra");
DEFINE_DATASET(HashPrefix4byte255, 255, HASH_PREFIX_4byte_255, "CE1A", "1A3F",
               "AAAA", "FFFF");

#undef DEFINE_DATASET
#undef COMMAND_AS_CASE
#undef COMMAND_AS_HANDLER
#undef COMMAND_AS_SV

// ============================================================================
// RNG + sequence generator
// ============================================================================

[[nodiscard]] auto next_random(std::uint64_t& state) -> std::uint64_t {
  state = state * 6364136223846793005ULL + 1442695040888963407ULL;
  return state;
}

template <class Dataset>
struct BenchInput {
  std::array<char, Dataset::kMaxInputSize + 1> storage{};
  std::uint8_t size = 0;

  void assign(std::string_view value) {
    size = static_cast<std::uint8_t>(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
      storage[i] = value[i];
    }
    storage[value.size()] = '\x7f';
  }

  [[nodiscard]] auto view() const -> std::string_view {
    return {storage.data(), size};
  }
};

template <class Dataset>
[[nodiscard]] auto make_sequence(int mode)
    -> std::array<BenchInput<Dataset>, kSequenceSize> {
  std::array<BenchInput<Dataset>, kSequenceSize> seq{};
  std::uint64_t rng = 0x9e3779b97f4a7c15ULL +
                      static_cast<std::uint64_t>(mode) * 0xbf58476d1ce4e5b9ULL;

  for (auto& entry : seq) {
    const auto val = next_random(rng);
    if (mode == 0) {
      entry.assign(Dataset::kCommands[val % Dataset::kN]);
    } else if (mode == 2) {
      entry.assign(Dataset::kMissValues[val % Dataset::kMissValues.size()]);
    } else {
      const auto idx = val % (Dataset::kN + 1);
      entry.assign((idx == Dataset::kN)
                       ? Dataset::kMissValues[val % Dataset::kMissValues.size()]
                       : Dataset::kCommands[idx]);
    }
  }
  return seq;
}

// ============================================================================
// Approach 1: meta_match — compile-time trie (this library)
// ============================================================================

template <class Dataset>
struct MetaMatchRunner {
  static void run(std::string_view value,
                  typename Dataset::commands_t& commands) {
    static_cast<void>(match(value, commands));
  }
};

// ============================================================================
// Approach 2: if_match — linear short-circuit fold over equality checks
// ============================================================================

template <std::size_t I, class Commands>
inline __attribute__((always_inline)) auto matches_at(std::string_view value,
                                                      Commands& commands)
    -> bool {
  using cmd_t = std::tuple_element_t<I, std::remove_reference_t<Commands>>;
  if (value == cmd_t::view()) {
    std::get<I>(commands)();
    return true;
  }
  return false;
}

template <class Dataset>
struct IfMatchRunner {
  static void run(std::string_view value,
                  typename Dataset::commands_t& commands) {
    __asm__ volatile("# IfMatchRunner::run - begin");
    [&]<std::size_t... I>(std::index_sequence<I...>) __attribute__((
        always_inline)) -> auto {
      static_cast<void>((matches_at<I>(value, commands) || ...));
    }(std::make_index_sequence<
                            std::tuple_size_v<typename Dataset::commands_t>>{});
    __asm__ volatile("# IfMatchRunner::run - end");
  }
};

// ============================================================================
// Approach 3: glaze_match — glaze perfect hash (keys_wrapper + hash_info)
// ============================================================================

template <class Dataset>
struct GlazeMatchRunner {
  static void run(std::string_view value,
                  typename Dataset::commands_t& commands) {
    const auto index = glz::decode_hash_with_size<
        glz::JSON, typename Dataset::glaze_keys_t, Dataset::kGlazeHashInfo,
        Dataset::kGlazeHashInfo.type>::op(value.data(),
                                          value.data() + value.size(),
                                          value.size());
    if (index >= Dataset::kN) {
      return;
    }
    if (Dataset::kCommands[index] != value) {
      return;
    }
    Dataset::invoke_by_index(index, commands);
  }
};

// ============================================================================
// Generic benchmark driver
// ============================================================================

template <class Dataset, template <class> class Runner>
void bench(benchmark::State& state) {
  const auto mode = static_cast<int>(state.range(0));
  const auto sequence = make_sequence<Dataset>(mode);
  auto commands = Dataset::make_commands();
  std::size_t index = 0;

  for (auto _ : state) {
    Runner<Dataset>::run(sequence[index].view(), commands);
    benchmark::ClobberMemory();
    index = (index + 1) % sequence.size();
  }

  auto total_hits = std::apply(
      [](const auto&... e) -> auto { return (e.fn.hit_count + ... + 0); },
      commands);
  auto mixed_state = std::apply(
      [](const auto&... e) -> auto { return (e.fn.state ^ ... ^ 0ULL); },
      commands);
  benchmark::DoNotOptimize(total_hits);
  benchmark::DoNotOptimize(mixed_state);
  state.counters["hits"] = static_cast<double>(total_hits);
  state.SetLabel(kCaseNames.at(static_cast<std::size_t>(mode)));
}

#define REGISTER_DATASET(TypeName, Prefix)                   \
  static void Prefix##_meta_match(benchmark::State& state) { \
    bench<TypeName, MetaMatchRunner>(state);                 \
  }                                                          \
  static void Prefix##_if_match(benchmark::State& state) {   \
    bench<TypeName, IfMatchRunner>(state);                   \
  }                                                          \
  static void Prefix##_glaze(benchmark::State& state) {      \
    bench<TypeName, GlazeMatchRunner>(state);                \
  }                                                          \
  BENCHMARK(Prefix##_meta_match)                             \
      ->DenseRange(0, kModes)                                \
      ->Unit(benchmark::kNanosecond);                        \
  BENCHMARK(Prefix##_if_match)                               \
      ->DenseRange(0, kModes)                                \
      ->Unit(benchmark::kNanosecond);                        \
  BENCHMARK(Prefix##_glaze)->DenseRange(0, kModes)->Unit(benchmark::kNanosecond)

REGISTER_DATASET(HttpMethods10, bench_http_methods_10);
REGISTER_DATASET(JsonFields10, bench_json_fields_10);
REGISTER_DATASET(PrefixFamily10, bench_prefix_family_10);
REGISTER_DATASET(FirstByteDistinct10, bench_first_byte_distinct_10);
REGISTER_DATASET(TailDistinct10, bench_tail_distinct_10);
REGISTER_DATASET(HttpHeaders50, bench_http_headers_50);
REGISTER_DATASET(HashPrefix4byte255, bench_hash_prefix_4byte_255);

#undef REGISTER_DATASET
#undef HTTP_HEADERS_50
#undef TAIL_DISTINCT_10
#undef FIRST_BYTE_DISTINCT_10
#undef PREFIX_FAMILY_10
#undef JSON_FIELDS_10
#undef HTTP_METHODS_10

}  // namespace

BENCHMARK_MAIN();
