#include <benchmark/benchmark.h>
#include <frozen/string.h>
#include <frozen/unordered_map.h>

#include <array>
#include <cstdint>
#include <string_view>
#include <tuple>
#include <type_traits>

#include "meta_match.hh"

namespace {

using namespace meta_match;
using namespace std::literals;
using namespace frozen::string_literals;

#define COMMAND_LIST(X)                         \
  X(0,  "550e8400-e29b-41d4-a716-446655440000")  \
  X(1,  "123e4567-e89b-12d3-a456-426614174000")  \
  X(2,  "9f1c2d3e-4b5a-4c6d-8e7f-1029384756aa")  \
  X(3,  "7a8b9c0d-1e2f-4a3b-9c8d-776655443322")  \
  X(4,  "de305d54-75b4-431b-adb2-eb6b9e546014")  \
  X(5,  "0f8fad5b-d9cb-469f-a165-70867728950e")  \
  X(6,  "3d6f0a21-7c4b-4e19-9d2a-88c1ef7b5a60")  \
  X(7,  "a987fbc9-4bed-4078-8f07-9141ba07c9f3")  \
  X(8,  "6ba7b810-9dad-11d1-80b4-00c04fd430c8")  \
  X(9,  "6ba7b811-9dad-11d1-80b4-00c04fd430c8")  \
  X(10, "16fd2706-8baf-433b-82eb-8c7fada847da") \
  X(11, "886313e1-3b8a-5372-9b90-0c9aee199e5d") \
  X(12, "c4a760a8-dbcf-44d0-9f3d-6b7e2c1a9d55") \
  X(13, "f47ac10b-58cc-4372-a567-0e02b2c3d479") \
  X(14, "2c1f9864-52d7-4d8a-bf3e-1a9c7e5d4b20") \
  X(15, "8d92f3a1-6b4c-4e7d-9f20-3c5a7b1e8d44") \
  X(16, "b1d2c3e4-f5a6-4789-8abc-def012345678") \
  X(17, "4e2d1c0b-9a8f-4765-b432-10fedcba9876") \
  X(18, "d3b07384-d9a2-4c6f-8e1b-5a7c9d2e4f60") \
  X(19, "fe12ab34-cd56-4789-90ef-1234567890ab") \
  X(20, "0a1b2c3d-4e5f-4789-8abc-001122334455") \
  X(21, "1b2c3d4e-5f60-489a-9bcd-112233445566") \
  X(22, "2c3d4e5f-6071-49ab-acde-223344556677") \
  X(23, "3d4e5f60-7182-4abc-bdef-334455667788") \
  X(24, "4e5f6071-8293-4bcd-8ef0-445566778899") \
  X(25, "5f607182-93a4-4cde-9f01-5566778899aa") \
  X(26, "60718293-a4b5-4def-af12-66778899aabb") \
  X(27, "718293a4-b5c6-4ef0-b123-778899aabbcc") \
  X(28, "8293a4b5-c6d7-4f01-8123-8899aabbccdd") \
  X(29, "93a4b5c6-d7e8-4012-9234-99aabbccddee") \
  X(30, "a4b5c6d7-e8f9-4123-a345-aabbccddeeff") \
  X(31, "b5c6d7e8-f90a-4234-b456-bbccddeeff00") \
  X(32, "c6d7e8f9-0a1b-4345-8456-ccddeeff0011") \
  X(33, "d7e8f90a-1b2c-4456-9567-ddeeff001122") \
  X(34, "e8f90a1b-2c3d-4567-a678-eeff00112233") \
  X(35, "f90a1b2c-3d4e-4678-b789-ff0011223344") \
  X(36, "0b1c2d3e-4f50-4789-8899-102132435465") \
  X(37, "1c2d3e4f-5061-489a-99aa-213243546576") \
  X(38, "2d3e4f50-6172-49ab-aabb-324354657687") \
  X(39, "3e4f5061-7283-4abc-bbcc-435465768798") \
  X(40, "4f506172-8394-4bcd-8ccd-5465768798a9") \
  X(41, "50617283-94a5-4cde-9dde-65768798a9ba") \
  X(42, "61728394-a5b6-4def-aeef-768798a9bacb") \
  X(43, "728394a5-b6c7-4ef0-bff0-8798a9bacbdc") \
  X(44, "8394a5b6-c7d8-4f01-8011-98a9bacbdced") \
  X(45, "94a5b6c7-d8e9-4012-9122-a9bacbdcedfe") \
  X(46, "a5b6c7d8-e9fa-4123-a233-bacbdcedfe0f") \
  X(47, "b6c7d8e9-fa0b-4234-b344-cbdcedfe0f10") \
  X(48, "c7d8e9fa-0b1c-4345-8455-dcedfe0f1021") \
  X(49, "d8e9fa0b-1c2d-4456-9566-edfe0f102132") \
  X(50, "e9fa0b1c-2d3e-4567-a677-fe0f10213243") \
  X(51, "fa0b1c2d-3e4f-4678-b788-0f1021324354") \
  X(52, "ab10cd20-ef30-4789-8a40-123451234512") \
  X(53, "bc21de31-f041-489a-9b51-234562345623") \
  X(54, "cd32ef42-0152-49ab-ac62-345673456734") \
  X(55, "de43f053-1263-4abc-bd73-456784567845") \
  X(56, "ef540164-2374-4bcd-8e84-567895678956") \
  X(57, "f0651275-3485-4cde-9f95-6789a6789a67") \
  X(58, "10662186-4596-4def-a0a6-789ab789ab78") \
  X(59, "21773297-56a7-4ef0-b1b7-89abc89abc89") \
  X(60, "328843a8-67b8-4f01-82c8-9abcd9abcd9a") \
  X(61, "439954b9-78c9-4012-93d9-abcdedabcdea") \
  X(62, "54aa65ca-89da-4123-a4ea-bcdefebcdefb") \
  X(63, "65bb76db-9aeb-4234-b5fb-cdef0fcdef0c") \
  X(64, "76cc87ec-abfc-4345-860c-def120def120") \
  X(65, "87dd98fd-bc0d-4456-971d-ef2311ef2311") \
  X(66, "98eea90e-cd1e-4567-a82e-f34222f34222") \
  X(67, "a9ffba1f-de2f-4678-b93f-043333043333") \
  X(68, "ba10cb20-ef30-4789-8a40-154444154444") \
  X(69, "cb21dc31-f041-489a-9b51-265555265555") \
  X(70, "13579bdf-2468-4ace-8fed-0123456789ab") \
  X(71, "2468ace0-1357-4bdf-9cfe-123456789abc") \
  X(72, "3579bdf1-0246-4cea-ad0f-23456789abcd") \
  X(73, "468ace02-1357-4dfb-be10-3456789abcde") \
  X(74, "579bdf13-2468-4eac-8f21-456789abcdef") \
  X(75, "68ace024-3579-4fbd-9032-56789abcdef0") \
  X(76, "79bdf135-468a-40ce-a143-6789abcdef01") \
  X(77, "8ace0246-579b-41df-b254-789abcdef012") \
  X(78, "9bdf1357-68ac-42e0-8365-89abcdef0123") \
  X(79, "ace02468-79bd-43f1-9476-9abcdef01234") \
  X(80, "bdf13579-8ace-4502-a587-abcdef012345") \
  X(81, "ce02468a-9bdf-4613-b698-bcdef0123456") \
  X(82, "df13579b-ace0-4724-87a9-cdef01234567") \
  X(83, "e02468ac-bdf1-4835-98ba-def012345678") \
  X(84, "f13579bd-ce02-4946-a9cb-ef0123456789") \
  X(85, "02468ace-df13-4a57-badc-f0123456789a") \
  X(86, "11111111-2222-4333-8444-555555555555") \
  X(87, "22222222-3333-4444-8555-666666666666") \
  X(88, "33333333-4444-4555-8666-777777777777") \
  X(89, "44444444-5555-4666-8777-888888888888") \
  X(90, "55555555-6666-4777-8888-999999999999") \
  X(91, "66666666-7777-4888-8999-aaaaaaaaaaaa") \
  X(92, "77777777-8888-4999-8aaa-bbbbbbbbbbbb") \
  X(93, "88888888-9999-4aaa-8bbb-cccccccccccc") \
  X(94, "99999999-aaaa-4bbb-8ccc-dddddddddddd") \
  X(95, "aaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee") \
  X(96, "bbbbbbbb-cccc-4ddd-8eee-ffffffffffff") \
  X(97, "cccccccc-dddd-4eee-8fff-000000000000") \
  X(98, "dddddddd-eeee-4fff-8000-111111111111") \
  X(99, "eeeeeeee-ffff-4000-8111-222222222222")

#define COMMAND_AS_SV(_, literal) literal##sv,
constexpr std::array kCommands{COMMAND_LIST(COMMAND_AS_SV)};
#undef COMMAND_AS_SV

constexpr std::array kCases{
    "550e8400-e29b-41d4-a716-446655440000"sv,
    "ffffffff-ffff-4fff-8fff-ffffffffffff"sv,
};

constexpr auto kCaseNames = std::array{
    "hits_only_randomized",
    "hit_miss_randomized",
    "miss_only",
};

constexpr std::size_t kSequenceSize = 1024;

auto make_commands() {
#define COMMAND_AS_TYPE(_, literal) A<literal>{},
  return std::tuple{COMMAND_LIST(COMMAND_AS_TYPE)};
#undef COMMAND_AS_TYPE
}

using commands_t = decltype(make_commands());

[[nodiscard]]
auto next_random(std::uint64_t& state) -> std::uint64_t {
  state = state * 6364136223846793005ULL + 1442695040888963407ULL;
  return state;
}

[[nodiscard]]
auto make_sequence(int mode) -> std::array<std::string_view, kSequenceSize> {
  std::array<std::string_view, kSequenceSize> sequence{};
  std::uint64_t rng = 0x9e3779b97f4a7c15ULL +
                      static_cast<std::uint64_t>(mode) * 0xbf58476d1ce4e5b9ULL;

  for (auto& entry : sequence) {
    const auto value = next_random(rng);
    if (mode == 0) {
      entry = kCommands[value % kCommands.size()];
      continue;
    }

    if (mode == 2) {
      entry = kCases[1];
      continue;
    }

    const auto index = value % (kCommands.size() + 1);
    entry = index == kCommands.size() ? kCases[1] : kCommands[index];
  }

  return sequence;
}

template <std::size_t I, class Commands>
auto hit(Commands& commands) -> void {
  std::get<I>(commands)();
}

template <std::size_t I>
auto dispatch_hit(commands_t& commands) -> void {
  hit<I>(commands);
}

template <std::size_t I, class Commands>
[[nodiscard]]
inline __attribute__((always_inline)) auto matches_at(std::string_view value, Commands& commands) -> bool {
  using command_t = std::tuple_element_t<I, std::remove_reference_t<Commands>>;
  if (value == command_t::view()) {
    hit<I>(commands);
    return true;
  }
  return false;
}

#define COMMAND_AS_FROZEN_ENTRY(index, literal) \
  std::pair{literal##_s, static_cast<std::uint8_t>(index)},
constexpr auto kFrozenDispatch = frozen::make_unordered_map(
    std::array{COMMAND_LIST(COMMAND_AS_FROZEN_ENTRY)});
#undef COMMAND_AS_FROZEN_ENTRY

auto if_match(std::string_view value, decltype(make_commands())& commands)
    -> void {
  [&]<std::size_t... I>
    (std::index_sequence<I...>)  __attribute__((always_inline)){
    static_cast<void>((matches_at<I>(value, commands) || ...));
  }(std::make_index_sequence<std::tuple_size_v<commands_t>>{});
}

auto frozen_match(std::string_view value, commands_t& commands) -> void {
  if (const auto it = kFrozenDispatch.find(value); it != kFrozenDispatch.end()) {
    switch (it->second) {
#define COMMAND_AS_FROZEN_CASE(index, literal) \
  case index:                                  \
    return dispatch_hit<index>(commands);
      COMMAND_LIST(COMMAND_AS_FROZEN_CASE)
#undef COMMAND_AS_FROZEN_CASE
      default:
        return;
    }
  }
}

template <auto Fn>
void bench(benchmark::State& state) {
  const auto mode = static_cast<std::size_t>(state.range(0));
  const auto sequence = make_sequence(static_cast<int>(mode));
  auto commands = make_commands();
  std::size_t index = 0;
  for (auto _ : state) {
    Fn(sequence[index], commands);
    benchmark::ClobberMemory();
    index = (index + 1) % sequence.size();
  }
  auto hit_count = std::apply(
      [](const auto&... command) -> auto {
        return (command.hit_count + ... + 0);
      },
      commands);
  auto mixed_state = std::apply(
      [](const auto&... command) -> auto {
        return (command.state ^ ... ^ 0ULL);
      },
      commands);
  benchmark::DoNotOptimize(hit_count);
  benchmark::DoNotOptimize(mixed_state);
  state.counters["hits"] = static_cast<double>(hit_count);
  state.SetLabel(kCaseNames.at(mode));
}

auto run_meta_match(std::string_view value, decltype(make_commands())& commands)
    -> void {
  static_cast<void>(test<0>(value, commands));
}

auto run_if_match(std::string_view value, decltype(make_commands())& commands)
    -> void {
  if_match(value, commands);
}

auto run_frozen_match(std::string_view value, commands_t& commands) -> void {
  frozen_match(value, commands);
}

BENCHMARK(bench<run_meta_match>)
    ->DenseRange(0, static_cast<int>(kCaseNames.size() - 1))
    ->Unit(benchmark::kNanosecond);
BENCHMARK(bench<run_if_match>)
    ->DenseRange(0, static_cast<int>(kCaseNames.size() - 1))
    ->Unit(benchmark::kNanosecond);
BENCHMARK(bench<run_frozen_match>)
    ->DenseRange(0, static_cast<int>(kCaseNames.size() - 1))
    ->Unit(benchmark::kNanosecond);

}  // namespace

#undef COMMAND_LIST

BENCHMARK_MAIN();
