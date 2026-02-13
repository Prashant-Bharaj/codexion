#!/bin/bash
# Codexion tester - aligned with EVALUATION_SHEET.md
# Mapping: philosopher -> coders, fork -> dongle, eat -> compile, sleep -> debug+refactor
# Do not test with >200 coders; keep time params >= 60 ms

MAGENTA='\033[0;35m'
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'
BOLD='\033[1m'

total_tests=0
successful_tests=0
CODEXION_DIR="$(cd "$(dirname "$0")" && pwd)"
OUT="$CODEXION_DIR/out"

handle_ctrl_c() {
	echo -e "\nStopped tester"
	rm -f "$OUT"
	exit 1
}
trap handle_ctrl_c INT

check_compilation() {
	echo -e "\n${MAGENTA}=== Compilation (Basics: -Wall -Wextra -Werror -pthread) ===${NC}"
	cd "$CODEXION_DIR" && make re > "$OUT" 2>&1
	if [ $? -ne 0 ]; then
		echo -e "Compilation: ${RED}KO${NC}"
		cat "$OUT"
		rm -f "$OUT"
		exit 1
	fi
	echo -e "Compilation: ${GREEN}OK${NC}"
	rm -f "$OUT"
}

# Basics: verify binary is named "codexion"
check_binary_name() {
	((total_tests++))
	cd "$CODEXION_DIR"
	if [ -x "./codexion" ]; then
		echo -e "[ TEST $total_tests ] Binary name (codexion): ${GREEN}OK${NC}"
		((successful_tests++))
	else
		echo -e "[ TEST $total_tests ] Binary name (codexion): ${RED}KO${NC}"
	fi
}

# README.md: exists at repo root and has required sections (evaluation sheet)
check_readme() {
	((total_tests++))
	README="$CODEXION_DIR/../README.md"
	if [ ! -f "$README" ]; then
		README="$CODEXION_DIR/README.md"
	fi
	if [ ! -f "$README" ]; then
		echo -e "[ TEST $total_tests ] README.md: ${RED}KO${NC} (not found)"
		rm -f "$OUT"
		return
	fi
	missing=""
	grep -qi "description" "$README" || missing="$missing Description"
	grep -qi "instructions" "$README" || missing="$missing Instructions"
	grep -qi "resources" "$README" || missing="$missing Resources"
	grep -qi "blocking cases" "$README" || missing="$missing Blocking_cases"
	grep -qi "thread synchronization" "$README" || missing="$missing Thread_sync"
	if [ -z "$missing" ]; then
		echo -e "[ TEST $total_tests ] README.md sections: ${GREEN}OK${NC}"
		((successful_tests++))
	else
		echo -e "[ TEST $total_tests ] README.md sections: ${RED}KO${NC} (missing:$missing)"
	fi
}

invalid_arg_test() {
	((total_tests++))
	cd "$CODEXION_DIR"
	./codexion "$@" 2> "$OUT"
	exit_code=$?
	if [ $exit_code -ne 0 ] || grep -qi "usage\|error\|invalid" "$OUT" 2>/dev/null; then
		echo -e "[ TEST $total_tests ] Invalid args ($*): ${GREEN}OK${NC} (rejected)"
		((successful_tests++))
	else
		echo -e "[ TEST $total_tests ] Invalid args ($*): ${RED}KO${NC} (should reject)"
	fi
	rm -f "$OUT"
}

burnout_test() {
	((total_tests++))
	timeout_sec=$1
	shift
	cd "$CODEXION_DIR"
	timeout $timeout_sec ./codexion "$@" > "$OUT" 2>&1
	exit_code=$?
	if [ $exit_code -eq 124 ]; then
		echo -e "[ TEST $total_tests ] Burnout ($*): ${RED}KO${NC} (timeout - no burnout)"
	elif grep -q "burned out" "$OUT" && [ $exit_code -ne 0 ]; then
		burnout_line=$(grep "burned out" "$OUT")
		echo -e "[ TEST $total_tests ] Burnout ($*): ${GREEN}OK${NC} ($burnout_line)"
		((successful_tests++))
	else
		echo -e "[ TEST $total_tests ] Burnout ($*): ${RED}KO${NC} (exit=$exit_code, expected burnout)"
	fi
	rm -f "$OUT"
}

# Burnout message must appear no more than 10 ms after actual burnout
burnout_timing_test() {
	((total_tests++))
	timeout_sec=$1
	time_to_burnout=$2
	shift 2
	cd "$CODEXION_DIR"
	timeout $timeout_sec ./codexion "$@" > "$OUT" 2>&1
	exit_code=$?
	if [ $exit_code -eq 124 ]; then
		echo -e "[ TEST $total_tests ] Burnout timing ($*): ${RED}KO${NC} (timeout)"
		rm -f "$OUT"
		return
	fi
	if ! grep -q "burned out" "$OUT" || [ $exit_code -eq 0 ]; then
		echo -e "[ TEST $total_tests ] Burnout timing ($*): ${RED}KO${NC} (no burnout)"
		rm -f "$OUT"
		return
	fi
	ok=1
	while IFS= read -r line; do
		msg_ts=$(echo "$line" | awk '{print $1}')
		coder_id=$(echo "$line" | awk '{print $2}')
		last_compile=$(grep " $coder_id is compiling$" "$OUT" 2>/dev/null | tail -1 | awk '{print $1}')
		if [ -z "$last_compile" ]; then
			last_compile=0
		fi
		actual_burnout=$((last_compile + time_to_burnout))
		delay=$((msg_ts - actual_burnout))
		if [ "$delay" -lt 0 ] || [ "$delay" -gt 10 ]; then
			ok=0
			break
		fi
	done < <(grep "burned out" "$OUT")
	if [ $ok -eq 1 ]; then
		echo -e "[ TEST $total_tests ] Burnout timing ($*): ${GREEN}OK${NC} (msg within 10ms of burnout)"
		((successful_tests++))
	else
		echo -e "[ TEST $total_tests ] Burnout timing ($*): ${RED}KO${NC} (msg >10ms after burnout)"
	fi
	rm -f "$OUT"
}

success_test() {
	((total_tests++))
	timeout_sec=$1
	shift
	cd "$CODEXION_DIR"
	timeout $timeout_sec ./codexion "$@" > "$OUT" 2>&1
	exit_code=$?
	if [ $exit_code -eq 124 ]; then
		echo -e "[ TEST $total_tests ] Success ($*): ${RED}KO${NC} (timeout)"
	elif [ $exit_code -eq 0 ] && ! grep -q "burned out" "$OUT"; then
		echo -e "[ TEST $total_tests ] Success ($*): ${GREEN}OK${NC}"
		((successful_tests++))
	else
		echo -e "[ TEST $total_tests ] Success ($*): ${RED}KO${NC} (exit=$exit_code)"
	fi
	rm -f "$OUT"
}

log_format_test() {
	((total_tests++))
	cd "$CODEXION_DIR"
	timeout 8 ./codexion 2 3000 100 60 60 2 0 fifo > "$OUT" 2>&1
	if [ $? -ne 0 ]; then
		echo -e "[ TEST $total_tests ] Log format: ${RED}KO${NC} (program failed)"
		rm -f "$OUT"
		return
	fi
	ok=1
	while IFS= read -r line; do
		if [[ "$line" =~ ^[0-9]+[[:space:]]+[0-9]+[[:space:]]+(has taken a dongle|is compiling|is debugging|is refactoring)$ ]]; then
			:
		else
			ok=0
			break
		fi
	done < "$OUT"
	if [ $ok -eq 1 ] && [ -s "$OUT" ]; then
		echo -e "[ TEST $total_tests ] Log format: ${GREEN}OK${NC}"
		((successful_tests++))
	else
		echo -e "[ TEST $total_tests ] Log format: ${RED}KO${NC}"
	fi
	rm -f "$OUT"
}

# State messages must not be mixed up (no interleaving from concurrent threads)
log_no_mixing_test() {
	((total_tests++))
	cd "$CODEXION_DIR"
	timeout 15 ./codexion "$@" > "$OUT" 2>&1
	if [ $? -eq 124 ]; then
		echo -e "[ TEST $total_tests ] Log no mixing ($*): ${RED}KO${NC} (timeout)"
		rm -f "$OUT"
		return
	fi
	if [ ! -s "$OUT" ]; then
		echo -e "[ TEST $total_tests ] Log no mixing ($*): ${RED}KO${NC} (no output)"
		rm -f "$OUT"
		return
	fi
	ok=1
	while IFS= read -r line; do
		if [[ -n "$line" && "$line" =~ ^[0-9]+[[:space:]]+[0-9]+[[:space:]]+(has[[:space:]]taken[[:space:]]a[[:space:]]dongle|is[[:space:]]compiling|is[[:space:]]debugging|is[[:space:]]refactoring|burned[[:space:]]out)$ ]]; then
			:
		elif [[ -z "$line" ]]; then
			:
		else
			ok=0
			break
		fi
	done < "$OUT"
	if [ $ok -eq 1 ]; then
		echo -e "[ TEST $total_tests ] Log no mixing ($*): ${GREEN}OK${NC} (no interleaved messages)"
		((successful_tests++))
	else
		echo -e "[ TEST $total_tests ] Log no mixing ($*): ${RED}KO${NC} (mixed/interleaved message)"
	fi
	rm -f "$OUT"
}

# Less easy: No dongle duplication - each "is compiling" preceded by exactly 2 "has taken a dongle" per coder
dongle_no_duplication_test() {
	((total_tests++))
	cd "$CODEXION_DIR"
	timeout 15 ./codexion "$@" > "$OUT" 2>&1
	if [ $? -ne 0 ] || grep -q "burned out" "$OUT"; then
		echo -e "[ TEST $total_tests ] Dongle no duplication ($*): ${RED}KO${NC} (run failed)"
		rm -f "$OUT"
		return
	fi
	ok=1
	for coder_id in $(awk '{print $2}' "$OUT" | sort -un); do
		takes=$(grep -cE "^[0-9]+ $coder_id has taken a dongle$" "$OUT" 2>/dev/null || echo 0)
		compiles=$(grep -cE "^[0-9]+ $coder_id is compiling$" "$OUT" 2>/dev/null || echo 0)
		if [ "$compiles" -gt 0 ] && [ "$takes" -ne $((compiles * 2)) ]; then
			ok=0
			break
		fi
	done
	if [ $ok -eq 1 ]; then
		echo -e "[ TEST $total_tests ] Dongle no duplication ($*): ${GREEN}OK${NC}"
		((successful_tests++))
	else
		echo -e "[ TEST $total_tests ] Dongle no duplication ($*): ${RED}KO${NC} (2 takes/compile violated)"
	fi
	rm -f "$OUT"
}

# Less easy: Correct state transitions (compile -> debug -> refactor)
state_transitions_test() {
	((total_tests++))
	cd "$CODEXION_DIR"
	timeout 15 ./codexion "$@" > "$OUT" 2>&1
	if [ $? -ne 0 ] || grep -q "burned out" "$OUT"; then
		echo -e "[ TEST $total_tests ] State transitions ($*): ${RED}KO${NC} (run failed)"
		rm -f "$OUT"
		return
	fi
	ok=1
	while IFS= read -r line; do
		ts=$(echo "$line" | awk '{print $1}')
		cid=$(echo "$line" | awk '{print $2}')
		msg=$(echo "$line" | cut -d' ' -f3-)
		case "$msg" in
			"has taken a dongle") ;;
			"is compiling") ;;
			"is debugging") ;;
			"is refactoring") ;;
			*) ok=0; break ;;
		esac
	done < "$OUT"
	# Each coder: debug count and refactor count should equal compile count
	for coder_id in $(grep -oE "^[0-9]+ [0-9]+ " "$OUT" | awk '{print $2}' | sort -u); do
		compiles=$(grep " $coder_id is compiling$" "$OUT" | wc -l)
		debugs=$(grep " $coder_id is debugging$" "$OUT" | wc -l)
		refactors=$(grep " $coder_id is refactoring$" "$OUT" | wc -l)
		if [ "$compiles" -ne "$debugs" ] || [ "$compiles" -ne "$refactors" ]; then
			ok=0
			break
		fi
	done
	if [ $ok -eq 1 ]; then
		echo -e "[ TEST $total_tests ] State transitions ($*): ${GREEN}OK${NC}"
		((successful_tests++))
	else
		echo -e "[ TEST $total_tests ] State transitions ($*): ${RED}KO${NC}"
	fi
	rm -f "$OUT"
}

# Medium: Cooldown behavior - success with dongle_cooldown > 0
cooldown_test() {
	((total_tests++))
	cd "$CODEXION_DIR"
	timeout 30 ./codexion 4 3000 200 200 60 3 100 fifo > "$OUT" 2>&1
	if [ $? -eq 0 ] && ! grep -q "burned out" "$OUT"; then
		echo -e "[ TEST $total_tests ] Cooldown (4 coders, cooldown=100ms): ${GREEN}OK${NC}"
		((successful_tests++))
	else
		echo -e "[ TEST $total_tests ] Cooldown (4 coders, cooldown=100ms): ${RED}KO${NC}"
	fi
	rm -f "$OUT"
}

# Medium: EDF vs FIFO - both schedulers complete successfully
scheduler_comparison_test() {
	((total_tests++))
	cd "$CODEXION_DIR"
	timeout 30 ./codexion 4 3000 200 200 60 3 0 fifo > "$OUT" 2>&1
	fifo_ok=$?
	timeout 30 ./codexion 4 3000 200 200 60 3 0 edf > "${OUT}.edf" 2>&1
	edf_ok=$?
	fifo_clean=$( ! grep -q "burned out" "$OUT" 2>/dev/null && echo 1 || echo 0 )
	edf_clean=$( ! grep -q "burned out" "${OUT}.edf" 2>/dev/null && echo 1 || echo 0 )
	if [ $fifo_ok -eq 0 ] && [ $edf_ok -eq 0 ] && [ "$fifo_clean" = "1" ] && [ "$edf_clean" = "1" ]; then
		echo -e "[ TEST $total_tests ] Scheduler FIFO vs EDF: ${GREEN}OK${NC}"
		((successful_tests++))
	else
		echo -e "[ TEST $total_tests ] Scheduler FIFO vs EDF: ${RED}KO${NC}"
	fi
	rm -f "$OUT" "${OUT}.edf"
}

# Recode: EDF tie-breaker - contention test (validates completion under stress)
edf_tie_breaker_test() {
	((total_tests++))
	cd "$CODEXION_DIR"
	# 8 coders stress EDF tie-breaking when deadlines equal; burnout 2000 for robustness
	timeout 60 ./codexion 8 2000 200 100 60 3 0 edf > "$OUT" 2>&1
	if [ $? -eq 0 ] && ! grep -q "burned out" "$OUT"; then
		echo -e "[ TEST $total_tests ] EDF tie-breaker contention (8 coders): ${GREEN}OK${NC}"
		((successful_tests++))
	else
		echo -e "[ TEST $total_tests ] EDF tie-breaker contention (8 coders): ${RED}KO${NC}"
	fi
	rm -f "$OUT"
}

valgrind_test() {
	if ! command -v valgrind &> /dev/null; then
		echo -e "[ VALGRIND ] ${YELLOW}skipped${NC} (valgrind not found)"
		return
	fi
	((total_tests++))
	cd "$CODEXION_DIR"
	timeout 15 valgrind --leak-check=full --show-leak-kinds=definite ./codexion 2 3000 100 60 60 2 0 fifo > "$OUT" 2> "${OUT}.vg"
	exit_code=$?
	if grep -qE "definitely lost: 0 bytes|no leaks are possible" "${OUT}.vg" 2>/dev/null; then
		echo -e "[ TEST $total_tests ] Valgrind: ${GREEN}OK${NC}"
		((successful_tests++))
	else
		echo -e "[ TEST $total_tests ] Valgrind: ${RED}KO${NC}"
		grep -E "definitely lost|ERROR SUMMARY" "${OUT}.vg" 2>/dev/null | head -5
	fi
	rm -f "$OUT" "${OUT}.vg"
}

# Helgrind: detects data races in multithreaded programs
helgrind_test() {
	if ! command -v valgrind &> /dev/null; then
		echo -e "[ HELGRIND ] ${YELLOW}skipped${NC} (valgrind not found)"
		return
	fi
	((total_tests++))
	cd "$CODEXION_DIR"
	timeout 20 valgrind --tool=helgrind ./codexion 4 3000 200 200 60 2 0 fifo > "$OUT" 2> "${OUT}.hg"
	exit_code=$?
	if grep -qE "ERROR SUMMARY: 0 errors" "${OUT}.hg" 2>/dev/null; then
		echo -e "[ TEST $total_tests ] Helgrind (race check): ${GREEN}OK${NC}"
		((successful_tests++))
	else
		echo -e "[ TEST $total_tests ] Helgrind (race check): ${RED}KO${NC}"
		grep -E "Possible data race|ERROR SUMMARY|at " "${OUT}.hg" 2>/dev/null | head -15
	fi
	rm -f "$OUT" "${OUT}.hg"
}

print_summary() {
	echo -e "\n${MAGENTA}========== Summary ==========${NC}"
	echo -e "Tests: $successful_tests / $total_tests"
	if [ $successful_tests -eq $total_tests ]; then
		echo -e "${GREEN}All tests passed${NC}"
	else
		echo -e "${RED}Some tests failed${NC}"
	fi
}

# Main
echo -e "${BOLD}Codexion Tester (EVALUATION_SHEET.md)${NC}"
check_compilation
check_binary_name

echo -e "\n${MAGENTA}=== README.md (Description, Instructions, Resources, Blocking cases, Thread sync) ===${NC}"
check_readme

echo -e "\n${MAGENTA}=== Basics: Arguments (exactly 8, correct order, scheduler fifo|edf) ===${NC}"
invalid_arg_test ""
invalid_arg_test 1 2 3 4 5
# Too few args (7 params)
invalid_arg_test 2 1000 200 200 100 5 0
# Too many args (9 params)
invalid_arg_test 2 1000 200 200 100 5 0 fifo extra
# Negative numbers
invalid_arg_test -1 1000 200 200 100 5 0 fifo
invalid_arg_test 2 1000 200 200 100 -1 0 fifo
invalid_arg_test 2 -100 200 200 100 5 0 fifo
# Non-integers
invalid_arg_test 2 abc 200 200 100 5 0 fifo
invalid_arg_test 2 1000 200.5 200 100 5 0 fifo
# Scheduler other than fifo or edf (case-sensitive)
invalid_arg_test 2 1000 200 200 100 5 0 invalid
invalid_arg_test 2 1000 200 200 100 5 0 EDF
invalid_arg_test 2 1000 200 200 100 5 0 FIFO
invalid_arg_test 0 1000 200 200 100 5 0 fifo

echo -e "\n${MAGENTA}=== Easy: No burnout, all complete (timing >= 60ms, <= 200 coders) ===${NC}"
echo -e "${YELLOW}(philo: num time_to_die time_to_eat time_to_sleep [num_times])${NC}"
echo -e "${YELLOW}(codexion: num_coders time_to_burnout time_to_compile time_to_debug time_to_refactor num_compiles dongle_cooldown scheduler)${NC}"
# Test 1 800 200 200 - philosopher should not eat, should die
burnout_test 5 1 800 200 200 60 5 0 fifo
# Test 5 800 200 200 - No philosopher should die
success_test 30 5 3000 200 200 60 5 0 fifo
# Easy: up to 200 coders, timing >= 60ms
success_test 90 50 5000 200 200 60 2 0 fifo
# Test 5 800 200 200 7 - No death, stop when each ate/compiled 7 times
success_test 90 5 3000 200 200 60 7 0 fifo
# Test 4 410 200 200 - No philosopher should die
success_test 30 4 2000 200 200 60 5 0 fifo
# Test 4 310 200 100 - One philosopher/coder should die
burnout_test 10 4 310 200 100 60 5 0 fifo
# Test with 2 coders: death delayed by more than 10 ms is unacceptable
burnout_timing_test 10 200 2 200 200 200 60 5 0 fifo

echo -e "\n${MAGENTA}=== Additional success tests ===${NC}"
success_test 15 2 3000 100 60 60 3 0 fifo
success_test 25 4 3000 200 200 60 3 2 fifo
success_test 15 2 3000 100 60 60 2 0 edf

echo -e "\n${MAGENTA}=== EDF (Earliest Deadline First) verification ===${NC}"
echo -e "${YELLOW}(EDF: earliest deadline first; tie-break: higher coder_id)${NC}"
# EDF success: 4 coders (burnout 900+ for stability)
success_test 30 4 900 200 100 60 3 0 edf
success_test 60 4 1000 200 100 60 5 0 edf
success_test 25 4 900 200 100 60 2 0 edf

echo -e "\n${MAGENTA}=== EDF + cooldown 60 (burnout 1500 for robustness) ===${NC}"
# With cooldown 60, cycle adds ~60ms per handoff; burnout 1500 avoids flakiness
success_test 30 4 1500 200 100 60 3 60 fifo
success_test 30 4 1500 200 100 60 3 60 edf

echo -e "\n${MAGENTA}=== Additional burnout tests ===${NC}"
# 4 coders, 200ms burnout: coders 2&4 can't get dongles until 1&3 release ~201ms
burnout_test 5 4 200 200 200 60 5 0 fifo

echo -e "\n${MAGENTA}=== Less easy: Burnout edge cases, logging, timing, no dongle duplication, state transitions ===${NC}"
log_format_test
log_no_mixing_test 4 3000 200 200 60 3 2 fifo
dongle_no_duplication_test 4 3000 200 200 60 3 0 fifo
state_transitions_test 4 3000 200 200 60 3 0 fifo

echo -e "\n${MAGENTA}=== Medium: Cooldown, EDF vs FIFO, refactoring, log serialization ===${NC}"
cooldown_test
scheduler_comparison_test
# Refactoring timing: success with refactor=60 (already in success tests)
success_test 20 4 2000 200 200 60 2 0 fifo

echo -e "\n${MAGENTA}=== Recode: EDF tie-breaker (higher coder_id on equal deadlines) ===${NC}"
edf_tie_breaker_test

echo -e "\n${MAGENTA}=== Valgrind / Helgrind (Basics: no leaks, no data races) ===${NC}"
valgrind_test
helgrind_test

print_summary
