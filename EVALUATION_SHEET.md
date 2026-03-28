# Codexion — Evaluation Sheet

---

## Before Starting

- [ ] Clone repo into an **empty folder**: `git clone <repo> && cd codexion`
- [ ] Confirm the repo belongs to the evaluated student
- [ ] Check for no malicious aliases
- [ ] Both evaluator and evaluated have reviewed any helper scripts
- [ ] If you haven't done this project yourself, read the full subject first

> During the entire defence: any segfault, deadlock, data race, interleaved log line,
> or memory leak → **final grade is 0**.

---

## 1. Preliminaries — Basics

### 1.1 Compilation

```bash
cd coders/ && make
```

- [ ] Compiles cleanly with **no warnings** using `-Wall -Wextra -Werror -pthread`
- [ ] Binary is named **`codexion`** inside `coders/`
- [ ] Makefile contains rules: `$(NAME)`, `all`, `clean`, `fclean`, `re`
- [ ] Makefile does **not** relink unnecessarily (`make` twice → no recompile on second run)
- [ ] No `libft` used

### 1.2 Arguments

Program must accept **exactly** these 8 arguments in order:

```
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug \
           time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

- [ ] Rejects invalid input: negative numbers, non-integers, wrong scheduler name
- [ ] Scheduler accepts only `fifo` or `edf` (exact strings)
- [ ] Each coder is represented by a **thread** (`pthread_create`)

### 1.3 Allowed Functions

Only these external functions are permitted:
`pthread_create`, `pthread_join`, `pthread_mutex_init`, `pthread_mutex_lock`,
`pthread_mutex_unlock`, `pthread_mutex_destroy`, `pthread_cond_init`,
`pthread_cond_wait`, `pthread_cond_timedwait`, `pthread_cond_broadcast`,
`pthread_cond_destroy`, `gettimeofday`, `usleep`, `write`, `malloc`, `free`,
`printf`, `fprintf`, `strcmp`, `strlen`, `atoi`, `memset`

- [ ] No forbidden functions used

### 1.4 Log Format

Every state change must be printed as:

```
timestamp_in_ms X has taken a dongle
timestamp_in_ms X is compiling
timestamp_in_ms X is debugging
timestamp_in_ms X is refactoring
timestamp_in_ms X burned out
```

- [ ] Format matches exactly
- [ ] Messages are **never interleaved** (log mutex used)
- [ ] Burnout message printed **within 10 ms** of actual burnout

> On slow hardware, timing may vary slightly — discuss borderline cases before marking as failure.

**Basics check:** `[ ] Yes   [ ] No`

---

## 2. Global Variables

Inspect the source code:

- [ ] **No global mutable variable** managing: dongle state, scheduling policy, or logging

> If any forbidden global is found → **evaluation stops here** (no exercise grading).

**No forbidden globals:** `[ ] Yes   [ ] No`

---

## 3. README.md

Verify the README at repository root contains **all** of:

- [ ] First line is italicized and reads exactly:
  *This project has been created as part of the 42 curriculum by \<login\>*
- [ ] **Description** section — project goal and brief overview
- [ ] **Instructions** section — compilation, installation, execution
- [ ] **Resources** section — references + how AI was used (which tasks, which parts)
- [ ] **Blocking cases handled** section — deadlock prevention (Coffman's conditions),
  starvation prevention, cooldown handling, burnout detection, log serialization
- [ ] **Thread synchronization mechanisms** section — explains `pthread_mutex_t`,
  `pthread_cond_t`, how race conditions are prevented, how coders and monitor communicate
- [ ] README is written in **English**

**README complete:** `[ ] Yes   [ ] No`

---

## 4. Codexion Testing — Easy

> Max 200 coders. No timing values under 60 ms.

```bash
cd coders/

# Basic FIFO — 4 coders, comfortable timings
./codexion 4 410 200 200 200 5 60 fifo

# Basic EDF — 5 coders
./codexion 5 850 200 200 100 6 80 edf

# Single coder edge case (only 1 dongle on the table)
./codexion 1 500 200 200 200 3 60 fifo
```

- [ ] No coder burns out
- [ ] All coders complete the required number of compiles
- [ ] Program exits cleanly after all compiles are done
- [ ] Log format is correct: two `has taken a dongle` before `is compiling`

**Easy tests pass:** `[ ] Yes   [ ] No`

---

## 5. Codexion Testing — Less Easy

```bash
# Tight burnout — must NOT burn out (EDF should save them)
./codexion 5 850 200 200 100 6 80 edf

# Slightly looser — also must pass
./codexion 5 980 200 200 100 6 80 edf

# 3 coders with moderate pressure
./codexion 3 850 200 200 100 6 80 edf

# Burnout scenario — time_to_burnout shorter than cycle time (should burn out)
./codexion 4 300 200 200 200 5 60 fifo
```

- [ ] Burnout edge cases handled: burnout triggers when expected, not before
- [ ] Log timestamps are correct and within timing tolerance
- [ ] No dongle ever held by two coders simultaneously
- [ ] Correct state transitions: `has taken dongle (×2)` → `compiling` → `debugging` → `refactoring` → repeat

**Less easy tests pass:** `[ ] Yes   [ ] No`

---

## 6. Codexion Testing — Medium

```bash
# Cooldown test — dongle must stay unavailable for cooldown duration
./codexion 4 800 200 200 200 5 200 fifo

# FIFO vs EDF comparison — observe scheduling order differences
./codexion 5 850 200 200 100 6 80 fifo
./codexion 5 850 200 200 100 6 80 edf

# Log serialization under load — check no lines are interleaved
./codexion 10 1000 200 200 200 5 80 fifo

# Refactoring timing — coder must refactor for correct duration before next compile
./codexion 3 1200 200 200 400 4 80 edf
```

- [ ] Cooldown respected: no dongle picked up before `dongle_cooldown` ms after release
- [ ] EDF and FIFO produce observably different scheduling (EDF prioritizes closest-to-burnout)
- [ ] No interleaved log lines under high coder count
- [ ] Refactor timing is correct before next compile attempt
- [ ] A priority queue (heap) is implemented for FIFO/EDF (verify in code)

**Medium tests pass:** `[ ] Yes   [ ] No`

---

## 7. Recode — EDF Tie-Breaker

> Ask the student to modify EDF so that when two coders have **equal deadlines**,
> the one with the **higher `coder_id`** is preferred.

```bash
# After modification — test with coders that will have identical deadlines
./codexion 4 800 200 200 200 5 60 edf
```

- [ ] On equal deadlines, higher `coder_id` is served first
- [ ] Verified via a contention scenario where multiple coders share the same deadline
- [ ] No regression in other tests after the change

**EDF tie-breaker works:** `[ ] Yes   [ ] No`

---

## 8. Memory & Safety

```bash
# Valgrind — check for leaks and errors
valgrind --leak-check=full --error-exitcode=1 \
  ./codexion 4 410 200 200 200 3 60 fifo

# Helgrind — check for data races
valgrind --tool=helgrind \
  ./codexion 4 410 200 200 200 3 60 fifo
```

- [ ] No memory leaks (all heap memory freed before exit)
- [ ] No data races
- [ ] No crash or unexpected termination throughout the entire defence

---

## 9. Rating

**Overall flag** (pick one):

| | Flag | | Flag |
|-|------|-|------|
| `[ ]` | Ok | `[ ]` | Outstanding project |
| `[ ]` | Empty work | `[ ]` | Incomplete work |
| `[ ]` | Invalid compilation | `[ ]` | Norme |
| `[ ]` | Cheat | `[ ]` | Crash |
| `[ ]` | Concerning situation | `[ ]` | Leaks |
| `[ ]` | Forbidden function | `[ ]` | Can't support / explain code |

---

## 10. Score Summary

| Section | Result |
|---------|--------|
| 1. Basics | Yes / No |
| 2. No global variables | Yes / No |
| 3. README complete | Yes / No |
| 4. Easy tests | Yes / No |
| 5. Less easy tests | Yes / No |
| 6. Medium tests | Yes / No |
| 7. EDF tie-breaker recode | Yes / No |

---

## 11. Conclusion

**Comment** (max 2048 chars):

> _Write your evaluation comment here._
