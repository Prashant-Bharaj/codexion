*This project has been created as part of the 42 curriculum by prasingh.*

# Codexion

## Description

Codexion is a concurrency simulation where multiple coders (threads) compete for limited USB dongles in a circular co-working hub. Each coder needs two dongles simultaneously to compile quantum code, then cycles through debug and refactor phases. The goal is to coordinate access using POSIX threads, mutexes, and condition variables—preventing deadlock and burnout while ensuring fair scheduling (FIFO or EDF).

## Instructions

### Compilation

```bash
cd coders/
make
```

### Execution

```bash
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug \
  time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

**Arguments:**

- `number_of_coders`: Number of coders and dongles
- `time_to_burnout` (ms): Max time since last compile start before burnout
- `time_to_compile` (ms): Compile duration (holding 2 dongles)
- `time_to_debug` (ms): Debug duration
- `time_to_refactor` (ms): Refactor duration
- `number_of_compiles_required`: Simulation stops when all coders reach this count
- `dongle_cooldown` (ms): Delay before a released dongle can be taken again
- `scheduler`: `fifo` or `edf`

**Constraints enforced by the program:**

- All times (`time_to_burnout`, `time_to_compile`, `time_to_debug`,
  `time_to_refactor`, `dongle_cooldown`) must be **≥ 60 ms**.
- `number_of_compiles_required` must be **≥ 1**.

**Basic example:**

```bash
./codexion 4 410 200 200 200 5 60 fifo
```

## Test scenarios

These are verified working test cases:

```bash
# 2 coders FIFO — baseline, alternating compiles
./codexion 2 2000 200 200 200 3 60 fifo

# 3 coders FIFO — sequential round-robin, one compile at a time
./codexion 3 1500 200 200 200 4 80 fifo

# 4 coders EDF — coders 1&3 and 2&4 compile in parallel pairs
./codexion 4 1200 200 200 200 4 80 edf

# Cooldown stress test — 500ms cooldown, verifies dongle unavailability
./codexion 2 2000 200 200 200 3 500 fifo

# Burnout trigger — cycle time exceeds burnout, should fire correctly
./codexion 3 400 200 200 200 5 80 fifo
```

## Feasibility analysis

Not all parameter combinations are valid. The simulation will always burn out
if `time_to_burnout` is shorter than the minimum time a coder must wait before
its next compile.

### Guaranteed safe formula

The following condition guarantees **no coder ever burns out**, regardless of
scheduling order or OS timing variance. It assumes the absolute worst case:
all other N-1 coders compile sequentially before this coder gets a turn.

```
time_to_burnout > (N - 1) × (time_to_compile + dongle_cooldown)
               + time_to_compile + time_to_debug + time_to_refactor
```

This is a **pessimistic upper bound** — in practice coders compile in parallel
pairs so the actual required burnout is lower, but this formula is always safe.

**Example (N=5, compile=debug=refactor=200ms, cooldown=80ms):**
```
time_to_burnout > (5-1) × (200 + 80) + 200 + 200 + 200
               > 1120 + 600
               > 1720ms
```
Verified: `./codexion 5 1800 200 200 200 3 80 edf` completes with no burnout.

### Quick reference — guaranteed no burnout (compile=debug=refactor=200ms, cooldown=80ms)

| N | Guaranteed safe `time_to_burnout` | Verified command |
|:---:|:---:|:---|
| 1 | impossible | — |
| 2 | > 880ms | `./codexion 2 900 200 200 200 3 80 fifo` |
| 3 | > 1160ms | `./codexion 3 1200 200 200 200 3 80 fifo` |
| 4 | > 1440ms | `./codexion 4 1500 200 200 200 3 80 edf` |
| 5 | > 1720ms | `./codexion 5 1800 200 200 200 3 80 edf` |

To scale to your own parameters, apply the formula above. The rule of thumb:
set `time_to_burnout` to at least `N × time_to_compile + time_to_debug + time_to_refactor + N × dongle_cooldown`.

### Minimum cycle formula (tighter, topology-dependent)

The actual minimum burnout needed depends on how many coders compile simultaneously:

```
min_cycle = time_to_compile + time_to_debug + time_to_refactor
          + (max_coders_waiting × (time_to_compile + dongle_cooldown))
```

`time_to_burnout` must be **strictly greater** than `min_cycle`.

### Topology constraints

Coders sit in a circle and each needs their two neighbouring dongles.
The maximum number of coders that can compile simultaneously is `floor(N/2)`,
so a coder may wait for up to `ceil(N/2) - 1` others before it gets a turn.

| N (coders) | Max simultaneous compilers | Max others to wait for |
|:---:|:---:|:---:|
| 1 | 0 — **always burns out** (only 1 dongle exists, need 2) | — |
| 2 | 1 | 1 |
| 3 | 1 | 2 |
| 4 | 2 | 1 |
| 5 | 2 | 2 |
| N | floor(N/2) | ceil(N/2) - 1 |

### Per-N examples (compile=debug=refactor=200ms, cooldown=80ms)

---

#### N=1 — always infeasible

Only 1 dongle exists in a circle of 1. The program requires 2 dongles to
compile and exits the acquire function immediately. The coder always burns out.

```bash
# Burns out immediately — expected, not a bug
./codexion 1 500 200 200 200 3 80 fifo
```

---

#### N=2

Empirically, coder 1 gets its second compile at t≈601ms.
Burnout must be **> 601ms**.

```bash
# Infeasible — burnout 600ms, coder burns out at deadline
./codexion 2 600 200 200 200 3 80 fifo

# Feasible — burnout 1000ms, all coders complete
./codexion 2 1000 200 200 200 3 80 fifo
```

---

#### N=3

Only 1 coder compiles at a time (triangle: all share at least one dongle).
Coder 1 waits for both others before its next turn.
Empirically, second compile at t≈845ms. Burnout must be **> 844ms**.

```bash
# Infeasible — burnout 800ms
./codexion 3 800 200 200 200 3 80 fifo

# Feasible — burnout 1000ms
./codexion 3 1000 200 200 200 3 80 fifo
```

---

#### N=4

Pairs (1,3) and (2,4) compile simultaneously. Each coder waits one full
round for the opposite pair. Empirically, second compile at t≈1126ms.
Burnout must be **> 1125ms**.

```bash
# Infeasible — burnout 1100ms
./codexion 4 1100 200 200 200 3 80 edf

# Feasible — burnout 1200ms
./codexion 4 1200 200 200 200 3 80 edf
```

---

#### N=5

Beyond the mathematical constraint, queue-based EDF scheduling creates a
starvation risk: coder 5 holds its place in dongle 4's queue while blocked
on dongle 3 (held by coder 4), preventing coder 1 from using dongle 4 even
when it is physically free. This starvation chain resolves only after coder 5
compiles. As a result, many burnout values that look mathematically safe still
fail in practice. Burnout must be **significantly > 1126ms** (empirically ~1300ms).

```bash
# Infeasible — 950ms fails despite seeming large enough
./codexion 5 950 200 200 200 3 80 edf

# Feasible — 1500ms provides enough margin
./codexion 5 1500 200 200 200 3 80 edf
```

---

#### Summary table (compile=debug=refactor=200ms, cooldown=80ms)

| N | 2nd compile at | Min burnout (empirical) | Guaranteed safe burnout `(N-1)*(C+D)+C+Dbg+R` | Guaranteed safe example |
|:---:|:---:|:---:|:---:|:---|
| 1 | never | always fails | — | — |
| 2 | ~601ms | > 601ms | > 880ms | `./codexion 2 900 200 200 200 3 80 fifo` |
| 3 | ~845ms | > 844ms | > 1160ms | `./codexion 3 1200 200 200 200 3 80 fifo` |
| 4 | ~1126ms | > 1125ms | > 1440ms | `./codexion 4 1500 200 200 200 3 80 edf` |
| 5 | ~1126ms | > ~1300ms | > 1720ms | `./codexion 5 1800 200 200 200 3 80 edf` |

`C` = compile, `D` = cooldown, `Dbg` = debug, `R` = refactor (all 200ms, cooldown 80ms above).

## Blocking cases handled

- **Deadlock prevention (Coffman's conditions):**
  - Each coder always requests its two neighbouring dongles as an **atomic pair**:
    either it acquires both, or neither.
  - For each pair, dongle mutexes are taken in a fixed global order
    (`min(left_idx, right_idx)` then `max(left_idx, right_idx)`), which breaks
    circular wait.

- **Starvation prevention (EDF + tie‑breaker):**
  - Under EDF, each request is prioritized by
    `deadline = last_compile_start + time_to_burnout`.
  - A per‑dongle min‑heap priority queue orders requests by
    `priority = deadline * (num_coders + 1) + (num_coders - coder_id)`,
    so the coder closest to burnout (and, on ties, lowest ID) gets served first
    at every dongle.

- **Cooldown handling and wake‑up logic:**
  - When a dongle is released, `cooldown_until` is set to
    `now + dongle_cooldown`.
  - Releasing a dongle calls `wake_all_dongles()`, which briefly locks each
    dongle mutex and broadcasts on its condition variable. Waiting coders wake
    up and re‑check their pair; they only proceed when both dongles are free
    and `now >= cooldown_until` for each.

- **Precise burnout detection:** A dedicated monitor thread checks approximately
  every 1 ms whether any coder has exceeded its burnout deadline. Burnout is
  detected well within the required 10 ms tolerance.

- **Log serialization:** All log output goes through `safe_log()`, which locks
  a `log_mutex` before `printf`. This prevents two messages from interleaving on
  a single line.

## Thread synchronization mechanisms

- **`pthread_mutex_t` (per dongle):** Each dongle has a mutex protecting its state (`cooldown_until`, `holder`) and its request queue. Coders lock the dongle mutex when acquiring or releasing.

- **`pthread_cond_t` (per dongle):**
  - Each dongle has a condition variable. Coders waiting for a pair call
    `pthread_cond_timedwait` on the lower‑indexed dongle’s condvar.
  - `dongle_release()` and `signal_stop()` call `wake_all_dongles()`, which,
    while holding each dongle’s mutex, broadcasts on every condvar so all
    waiters can re‑evaluate whether their pair is now available or the
    simulation should stop.

- **`pthread_mutex_t` (log_mutex):** Serializes all `printf` calls so log lines are atomic.

- **`pthread_mutex_t` (stop_mutex):** Protects `sim->stop` and `sim->num_coders_finished`. When burnout or success is detected, `signal_stop()` sets `stop = 1` and broadcasts on all dongle condvars so blocked coders wake and exit.

- **`pthread_mutex_t` (per coder_data):** Protects `last_compile_start` and `compile_count` so the monitor can safely read them while coders update them.

- **Race condition prevention:** The monitor never holds `stop_mutex` while
  locking `coder_data[i].mutex`, avoiding deadlock. Coders acquire dongles in
  a fixed order and only ever take a pair atomically, preventing circular wait
  and partial‑allocation races.

## Resources

- AI was used to clarify the subject, and discuss deadlock prevention strategies.

- [POSIX Threads (pthreads) - IEEE Std 1003.1](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/pthread.h.html)
- [Mutex and condition variable usage](https://man7.org/linux/man-pages/man3/pthread_mutex_lock.3.html)
- [Earliest Deadline First scheduling](https://en.wikipedia.org/wiki/Earliest_deadline_first_scheduling)
- [Coffman conditions (deadlock)](https://en.wikipedia.org/wiki/Deadlock#Necessary_conditions)
