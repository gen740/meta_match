# ============================================================
# meta_match — Makefile
# ============================================================
# Targets
#   make               → check-deps + example + benchmark
#   make run-example   → run the usage demo
#   make bench         → run the runtime benchmark
#   make bench-report  → run compile/runtime benchmarks and write graphs
#   make compile-syntax  → template/constexpr instantiation cost (-fsyntax-only)
#   make compile-object  → full compile cost incl. codegen (-c -O2)
#   make check-deps    → verify include paths are found
#   make clean
#
# Optional: install hyperfine in the nix shell for compile-time benchmarking.
#   Add pkgs.hyperfine to flake.nix devShell packages, then re-enter the shell.
# ============================================================

CXX := clang++

# ── flags from the Nix devShell ───────────────────────────────
# NIX_CFLAGS_COMPILE is injected by `nix develop` and already contains
# -isystem flags for every package in the devShell (glaze,
# benchmark, libcxx, …).  No manual find/pkg-config needed for includes.
BENCH_LIBS := $(shell pkg-config --libs benchmark 2>/dev/null || echo "-lbenchmark")

INCLUDES   := -I. $(NIX_CFLAGS_COMPILE)
CXXFLAGS   := -std=c++23 -O3 -march=native $(INCLUDES) -Wall -Wextra

HYPERFINE := hyperfine

define measure_compile
	@printf "%-34s " "$(1):"
	@$(HYPERFINE) --warmup 1 --runs 10 --style basic --time-unit second \
	  "$(CXX) $(CT_FLAGS) $(2)" 2>/dev/null \
	  | awk -F': ' '/^  Time \(mean ± σ\):/{sub(/    \[User.*/, "", $$2); print $$2}'
endef

# ── compile flags ─────────────────────────────────────────────
BASE_FLAGS := -std=c++23 -O3 -march=native $(INCLUDES)
# -fsyntax-only: template instantiation + semantic analysis, no codegen
CT_SYNTAX  := $(BASE_FLAGS) -fsyntax-only
# -c:           template + codegen (object file, no link) — measures real build cost
CT_OBJECT  := $(BASE_FLAGS) -c

CT_DRIVERS := _ct_meta.cc _ct_if.cc _ct_glaze.cc

# ── targets ──────────────────────────────────────────────────

.PHONY: all run-example bench bench-report \
        compile-syntax compile-object compile-time \
        compile_commands.json \
        check-deps clean

all: check-deps example benchmark compile_commands.json

# ── dependency guard ─────────────────────────────────────────
# NIX_CFLAGS_COMPILE is only set inside `nix develop`.
check-deps:
	@test -n "$(NIX_CFLAGS_COMPILE)" \
	  || { echo "ERROR: NIX_CFLAGS_COMPILE is empty — run: nix develop"; exit 1; }
	@command -v $(HYPERFINE) >/dev/null \
	  || { echo "ERROR: hyperfine not found"; exit 1; }

# ── binaries ─────────────────────────────────────────────────
example: example.cc meta_match.hh
	$(CXX) $(CXXFLAGS) -o $@ $<
	@echo "→ built: $@"

benchmark: benchmark.cc meta_match.hh
	$(CXX) $(CXXFLAGS) -o $@ $< $(BENCH_LIBS)
	@echo "→ built: $@"

# ── compile_commands.json ────────────────────────────────────
# clang -MJ writes one JSON fragment per TU; merge into a valid array.
compile_commands.json: example.cc benchmark.cc meta_match.hh
	@$(CXX) $(CXXFLAGS) -MJ /tmp/_mm_ex.json -c example.cc   -o /dev/null
	@$(CXX) $(CXXFLAGS) -MJ /tmp/_mm_bm.json -c benchmark.cc -o /dev/null
	@{ echo '['; \
	   cat /tmp/_mm_ex.json /tmp/_mm_bm.json | sed '$$s/,$$//'; \
	   echo ']'; } > $@
	@rm -f /tmp/_mm_ex.json /tmp/_mm_bm.json
	@echo "→ written: $@"

# ── run ──────────────────────────────────────────────────────
run-example: example
	./example

bench: benchmark
	./benchmark --benchmark_format=console

bench-report: check-deps benchmark
	python3 scripts/bench_report.py

# ── compile-time: syntax-only ────────────────────────────────
# Template instantiation + semantic analysis cost; no codegen.
# Use as a lower-bound / header-cost estimate.
compile-syntax: check-deps $(CT_DRIVERS)
	@echo "=== Compile-time: -fsyntax-only (template instantiation, no codegen) ==="
	@echo ""
	$(eval CT_FLAGS := $(CT_SYNTAX))
	$(call measure_compile,meta_match  (trie),_ct_meta.cc)
	$(call measure_compile,if_match    (linear fold),_ct_if.cc)
	$(call measure_compile,glaze       (perfect hash),_ct_glaze.cc)
	@echo ""

# ── compile-time: full object ─────────────────────────────────
# Template + optimization + codegen; closest to real-world build cost.
compile-object: check-deps $(CT_DRIVERS)
	@echo "=== Compile-time: -c -O2 (template + codegen) ==="
	@echo ""
	$(eval CT_FLAGS := $(CT_OBJECT))
	$(call measure_compile,meta_match  (trie),_ct_meta.cc -o /tmp/_mm_meta.o)
	$(call measure_compile,if_match    (linear fold),_ct_if.cc -o /tmp/_mm_if.o)
	$(call measure_compile,glaze       (perfect hash),_ct_glaze.cc -o /tmp/_mm_glaze.o)
	@echo ""

compile-time: compile-syntax compile-object

# ── assembly output ───────────────────────────────────────────
ASM_DIR := artifacts/asm
ASM_FLAGS := $(BASE_FLAGS) -S -fverbose-asm

.PHONY: asm asm-ct clean-asm

asm: benchmark.s example.s

benchmark.s: benchmark.cc meta_match.hh
	@mkdir -p $(ASM_DIR)
	$(CXX) $(ASM_FLAGS) -o $(ASM_DIR)/benchmark.s benchmark.cc
	@echo "→ written: $(ASM_DIR)/benchmark.s"

example.s: example.cc meta_match.hh
	@mkdir -p $(ASM_DIR)
	$(CXX) $(ASM_FLAGS) -o $(ASM_DIR)/example.s example.cc
	@echo "→ written: $(ASM_DIR)/example.s"

asm-ct: $(CT_DRIVERS)
	@mkdir -p $(ASM_DIR)
	$(CXX) $(ASM_FLAGS) -o $(ASM_DIR)/ct_meta.s   _ct_meta.cc
	$(CXX) $(ASM_FLAGS) -o $(ASM_DIR)/ct_if.s     _ct_if.cc
	$(CXX) $(ASM_FLAGS) -o $(ASM_DIR)/ct_glaze.s  _ct_glaze.cc
	@echo "→ written: $(ASM_DIR)/ct_*.s"

clean-asm:
	rm -rf $(ASM_DIR)

# ── clean ─────────────────────────────────────────────────────
clean:
	rm -f example benchmark compile_commands.json
	rm -f /tmp/_mm_meta.o /tmp/_mm_if.o /tmp/_mm_glaze.o
	rm -rf artifacts/asm
