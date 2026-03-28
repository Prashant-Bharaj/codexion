*This project has been created as part of the 42 curriculum by prasingh.*

# Codexion

## Description

Codexion is a concurrency simulation where multiple coders (threads) compete for limited USB dongles in a circular co-working hub. Each coder needs two dongles simultaneously to compile quantum code, then cycles through debug and refactor phases. The goal is to coordinate access using POSIX threads, mutexes, and condition variables—preventing deadlock and burnout while ensuring fair scheduling (FIFO or EDF).

## Instructions

### Compilation

From the directory that contains the `Makefile` (project root; `coders/` holds sources only):

```bash
make
```

### Execution

```bash
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug \
  time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

**Exit status:** `0` if every coder finishes the required number of compiles with no burnout; **non-zero** if arguments are invalid, init fails, or any coder burns out (`main` returns `sim.burnout_coder != 0`).

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

Unless noted, commands use **compile / debug / refactor = 200 ms**, **cooldown =
80 ms**, and **3 compiles required** — the same baseline as the feasibility
examples and § Mathematical accuracy reference checks.

### Exit `0` — smoke / success

```bash
# N=2 FIFO — baseline (alternating compiles)
./codexion 2 2000 200 200 200 3 60 fifo

# N=3 FIFO — one compiler at a time; 4 compiles required
./codexion 3 1500 200 200 200 4 80 fifo

# N=4 EDF — parallel pairs; comfortable burnout
./codexion 4 1200 200 200 200 4 80 edf

# N=4 EDF — tight but feasible (reference: succeeds at ≥ ~1020 ms burnout)
./codexion 4 1020 200 200 200 3 80 edf

# N=5 EDF — comfortable margin (guaranteed-safe style)
./codexion 5 1800 200 200 200 3 80 edf

# N=5 EDF — tight but feasible (reference: succeeds at ≥ ~850 ms burnout)
./codexion 5 850 200 200 200 3 80 edf

# Cooldown stress — long cooldown vs compile
./codexion 2 2000 200 200 200 3 500 fifo

# N=6 EDF — shorter phases, multi-queue contention
./codexion 6 2000 100 100 100 3 100 edf
```

### Non-zero exit — burnout or invalid args

```bash
# Parse error — times must be ≥ 60 ms
./codexion 2 50 200 200 200 3 80 fifo

# N=1 — cannot hold two dongles; always burns out
./codexion 1 500 200 200 200 3 80 fifo

# N=2 — burnout below ~601 ms gap (see Per-N examples)
./codexion 2 600 200 200 200 3 80 fifo

# N=3 — burnout 800 ms fails (see Per-N examples)
./codexion 3 800 200 200 200 3 80 fifo

# N=4 EDF — below reference floor (~600 ms)
./codexion 4 600 200 200 200 3 80 edf

# N=5 EDF — below reference floor (~800 ms)
./codexion 5 800 200 200 200 3 80 edf

# Burnout stress — many compiles, short burnout window (monitor stops sim)
./codexion 3 400 200 200 200 5 80 fifo
```

Re-run boundary cases after changing code, scheduler load, or VM; thresholds are
documented in **§ Mathematical accuracy** and **Per-N examples**.

## How it works — the complete story

### The world

N coders sit in a circle. Between every adjacent pair sits exactly one USB
dongle. There are therefore exactly N dongles, numbered 0 to N-1. Coder `i`
owns the dongle to its left (`(i-2+N) % N`) and the dongle to its right
(`(i-1) % N`). To compile, a coder must hold **both** neighbouring dongles
simultaneously for the entire compile duration, then release them. If a coder
goes too long without starting a new compile, it burns out and the simulation
ends immediately.

```
        Coder 1
    D0         D(N-1)
 Coder 2         Coder N
    D1         D(N-2)
          ...
```

---

### Step 1 — Birth (`run_simulation` → `pthread_create`)

`run_simulation` records `start_time` and sets every coder's
`last_compile_start` to that same timestamp (their burnout clock starts now).
It then creates N+1 threads: one `coder_routine` per coder (IDs 1..N) and one
`monitor_routine`. All threads are released at essentially the same instant.
The race begins.

---

### Step 2 — A coder wakes up (`coder_routine` → `coder_loop`)

`coder_loop` calls `get_coder_dongles` once to resolve the two physical dongle
indices for this coder. It then enters a loop:

```
check stop flag → attempt one full compile cycle → check completion → repeat
```

`get_coder_dongles` swaps left/right for odd-numbered coders (a remnant of the
odd/even dining-philosophers strategy). The caller `acquire_two_dongles`
immediately calls `order_indices`, which re-sorts the pair to
`f = min(left, right), s = max(left, right)`. The effective result is always
**f = lower-index dongle, s = higher-index dongle**, regardless of the swap.
This global ordering is what prevents deadlock.

---

### Step 3 — Two queues per dongle (`request_queue` vs `request_queue_s`)

After `order_indices`, every pair is **`f` = lower index, `s` = higher index**.
Each physical dongle stores **two** separate min-heaps (each a fixed 2-slot
`t_priority_queue`):

| Field | Meaning |
|:---|:---|
| `request_queue` | Waiters for whom this dongle is **`f`** (they are acquiring it **first**, Phase 1). |
| `request_queue_s` | Waiters for whom this dongle is **`s`** (they already hold their `f` and are acquiring this dongle **second**, Phase 2). |

Why two queues? The same edge is shared by two coders in different roles. One
coder’s pair is `(D, next)` so **D** is their `f`; another’s pair is `(prev, D)`
so **D** is their `s`. Mixing both roles into a **single** heap caused **head-of-line blocking**: a Phase‑1 waiter could sit at the head of D’s queue while blocked waiting on their **other** dongle, preventing a Phase‑2 waiter who only needed D as `s` from being ordered fairly. Splitting **f-role** and **s-role** waiters removes that coupling.

Priority for both enqueue sites (same compile attempt):

```
FIFO: priority = get_time_ms()
EDF:  f(deadline, cid) with deadline = last_compile_start + time_to_burnout
      (implemented as deadline × (N+1) − cid for strict ordering)
```

---

### Step 4 — Two-phase acquisition (`phase1_take_f` → `phase2_take_s`)

Acquisition is **not** one atomic “grab both”; it is **ordered**: take **`f` first**, then **`s`**. The implementation **never holds both dongle mutexes at once**, which avoids lock-order cycles.

**Phase 1 — lower dongle `f`**

1. Lock `dongles[f].mutex`.
2. `dongle_request_queue_add` on `dongles[f].request_queue` (f-queue only).
3. Wait with `pthread_cond_timedwait` on `dongles[f].cond` until
   `holder[f] < 0`, cooldown has passed, and **peek** says this `cid` is at the heap root.
4. `dongle_request_queue_remove_front` — **leave the f-queue as soon as `f` is taken**, then set `holder[f] = cid`, unlock `dongles[f].mutex`.

**Phase 2 — higher dongle `s`**

5. Lock `dongles[s].mutex`.
6. `dongle_request_queue_add` on `dongles[s].request_queue_s` (**s-queue** on the upper dongle).
7. Wait on `dongles[s].cond` until `holder[s] < 0`, cooldown OK, and this `cid` is at the root of **that** heap.
8. Remove from s-queue, set `holder[s] = cid`, unlock.

If Phase 2 is aborted (`sim->stop`), Phase 1 is rolled back: clear `holder[f]`
and `wake_all_dongles`. If a wait is cancelled early, `dongle_request_queue_remove_coder` removes the `cid` from the relevant heap.

---

### Step 5 — Why this cannot deadlock

A mutex deadlock requires a cycle of **mutex** waits. Here, each phase locks
**at most one** dongle mutex at a time (Phase 1: only `f`; Phase 2: only `s`).
Dongles are always claimed in **increasing index order** (`f` then `s`), matching
the resource order used in `release_two_dongles`. No thread holds mutex A while
waiting to lock mutex B in a way that reverses that order.

Empirically verified for N=2..10: the (f,s) pairs produced by the combined
`get_coder_dongles + order_indices` pipeline contain **no symmetric pairs**
(no (X,Y) and (Y,X) both present), proving no two coders can hold what the
other needs.

Example for N=5: pairs are (0,4) (0,1) (1,2) (2,3) (3,4). No symmetric pair.

---

### Step 6 — Compiling, releasing, cooling down

Once `acquire_two_dongles` finishes both phases, the coder has both dongles:

1. `safe_log` prints two `"has taken a dongle"` messages then `"is compiling"`.
2. `update_last_compile` locks the coder's own data mutex, records
   `last_compile_start = now`, and increments `compile_count`. This is the
   timestamp the monitor uses for all future burnout checks.
3. `usleep(time_to_compile × 1000)` — the coder sleeps while holding both
   dongles. No other coder that shares either dongle can compile during this
   time.
4. `release_two_dongles` calls `dongle_release` on each dongle in sequence:
   - Sets `holder = -1` (dongle is free).
   - Sets `cooldown_until = now + dongle_cooldown`. Wait loops in Phase 1/2
     reject the dongle until this timestamp expires.
   - Calls `wake_all_dongles`.
5. `wake_all_dongles` loops through **every** dongle, briefly locks its mutex,
   and calls `pthread_cond_broadcast` on its condition variable. Every coder
   waiting in Phase 1 or Phase 2 wakes and re-evaluates.

The coder then sleeps through debug (`usleep(time_to_debug × 1000)`) and
refactor (`usleep(time_to_refactor × 1000)`) before looping back.

---

### Step 7 — The monitor watches for burnout (`monitor_routine`)

The monitor loops forever, sleeping exactly 1ms between checks
(`usleep(1000)`). On each tick it calls `check_burnout`, which iterates every
coder and reads — under each coder's own `coder_data[i].mutex` — the
`last_compile_start` and `compile_count`.

For each coder that has not yet finished:

```
deadline = last_compile_start + time_to_burnout
if now >= deadline  →  burnout
```

On burnout:
1. Lock `stop_mutex`, record `burnout_coder`, unlock.
2. `signal_stop(sim)`: sets `sim->stop = 1`, then calls `wake_all_dongles` so
   every blocked coder wakes immediately and sees `is_stopped() == true`.
3. `safe_log` prints `"X burned out"`.
4. Monitor returns — simulation ends.

The 1ms poll guarantees the burnout message appears within ~1ms of the actual
deadline, comfortably within the required 10ms tolerance.

---

### Step 8 — Successful finish

When a coder finishes its last required compile, `check_done` increments
`num_coders_finished` under `stop_mutex`. When the last coder finishes,
`num_coders_finished == num_coders`: it calls `signal_stop` and exits.
Every other coder blocked in `phase1_take_f` or `phase2_take_s` receives the broadcast from
`wake_all_dongles`, reads `is_stopped() == 1`, and exits cleanly without
printing anything further (because `safe_log` suppresses non-burnout messages
once `sim->stop` is set).

---

### Step 9 — Shutdown and cleanup (`join_and_cleanup`)

`run_simulation` joins the monitor thread first, then all N coder threads in
order. After every thread has returned, `cleanup_simulation` destroys every
`pthread_mutex_t` and `pthread_cond_t`, frees **each** dongle's two
`malloc`'d `t_priority_queue` heaps (`request_queue` and `request_queue_s`),
frees the dongle and coder-data arrays, and returns. No heap memory is leaked.

---

### The complete data flow

```
main()
 └─ parse_and_init()        parse args, allocate sim/dongles/queues/mutexes
 └─ run_simulation()
      ├─ record start_time, init all last_compile_start
      │
      ├─ pthread_create × N ──► coder_routine(id)
      │                              └─ coder_loop()
      │                                   └─ [loop until stop or done]
      │                                        do_one_compile()
      │                                         ├─ acquire_two_dongles()
      │                                         │    ├─ order_indices()       f=min, s=max
      │                                         │    ├─ phase1_take_f()       f-queue, then holder[f]
      │                                         │    └─ phase2_take_s()       s-queue on dongle s, holder[s]
      │                                         ├─ update_last_compile()   stamp + count++
      │                                         ├─ usleep(compile)         hold dongles
      │                                         ├─ release_two_dongles()   cooldown + broadcast
      │                                         ├─ usleep(debug)
      │                                         └─ usleep(refactor)
      │
      ├─ pthread_create × 1 ──► monitor_routine()
      │                              └─ [loop every 1ms]
      │                                   check_burnout()
      │                                    └─ if now >= deadline
      │                                         signal_stop() + log "burned out"
      │
      └─ join monitor → join coders → cleanup_simulation() → free all
```

---

### Why the scheduler lives on dongles, not coders

Each dongle carries **two** heaps (`request_queue` and `request_queue_s`), not
`coder_data`. A coder has no global turn-order. **Phase 1** ordering is decided
by the f-queue on the **lower** index of the pair; **Phase 2** ordering by the
s-queue on the **higher** index. EDF/FIFO priorities are evaluated at **each**
enqueue (same priority value used for both phases of one compile attempt). A
coder close to burnout is promoted to the heap root on **each** dongle it waits
on in sequence, without a central scheduler.

---

### Head-of-line blocking (the issue) and two-phase + dual queues (the solution)

**Issue:** If every waiter for a dongle **D** shared one heap, a coder could be
at the root while still unable to proceed because their **partner** dongle was
busy. That “ghost” head-of-line slot blocked other coders who needed **D** in a
different role (e.g. as the **second** dongle after already holding `f`). Queue-based EDF/FIFO then created **starvation chains**: empirically, safe burnout
margins for N≥5 exceeded naive cycle formulas because waiters stayed in the
wrong queue for too long.

**Solution:** (1) **Split heaps** — `request_queue` only for “this dongle is my
`f`”, `request_queue_s` only for “this dongle is my `s`”. (2) **Two phases** —
remove from the f-queue **as soon as Phase 1 succeeds** (holder on `f`), **before**
waiting on the s-queue. A Phase‑2 waiter on D is no longer stuck behind a Phase‑1
waiter on D who is blocked elsewhere.

---

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

### Mathematical accuracy (formulas vs runs)

You can get **much closer** to the tight, topology-based intuition now than with
the older single-queue design:

- **What was wrong before:** Head-of-line “ghost” waiters inflated real
  worst-case gaps beyond what cycle-time math suggested, so empirical safe
  burnout (e.g. N=5) could sit far above the nominal second-compile spacing.
- **What holds now:** Separation of **f-queue** and **s-queue** removes that
  extra starvation; measured minimum burnout on a given machine tracks the
  **same order of magnitude** as the feasible schedule, not an arbitrary
  hundred-ms penalty on top.
- **What will never be exact:** Runs are still non-deterministic (`usleep`,
  scheduler, VM/WSL). No implementation guarantees **bit-for-bit** equality with
  a closed form for every OS. Use the **guaranteed safe** formula when you need
  a proof-level margin; use empirical sweeps when tuning tight burnout.

Reference checks (same 200/200/200 compile, 80ms cooldown, 3 compiles, this
tree): N=4 EDF fails at `time_to_burnout=600`, succeeds at `1020`; N=5 EDF
fails at `800`, succeeds at `850` (re-run if you change code or environment).
Values near the threshold (e.g. N=4 at `605` ms) can be flaky under scheduler
jitter.

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
round for the opposite pair. Logs often show a **second compile** for some
coders around **t≈1126ms**, but the **minimum burnout** that still completes
all coders is lower than that wall-clock marker (the monitor uses per-coder
intervals between **successive** compile starts, not the global round time).

```bash
# Infeasible — burnout 600ms (verified exit ≠ 0)
./codexion 4 600 200 200 200 3 80 edf

# Feasible — tight (verified exit 0)
./codexion 4 1020 200 200 200 3 80 edf

# Comfortable margin
./codexion 4 1200 200 200 200 3 80 edf
```

---

#### N=5

Older designs that used a **single** queue per dongle (or kept a waiter at the
head of the f-queue until **both** dongles could be taken atomically) suffered
**head-of-line blocking**: a coder could occupy the root slot on dongle D while
waiting on their other edge, blocking another coder who needed D only as the
**second** dongle. The current code uses **separate f- and s-queues** and
**two-phase** acquisition so that does not happen.

Margins are still **jitter-sensitive**; the guaranteed-safe formula below
remains a valid pessimistic bound. After the f/s queue fix, empirical floors are
much closer to tight schedules than under ghosting (re-measure after parameter
or code changes).

```bash
# Infeasible — reference run (exit ≠ 0)
./codexion 5 800 200 200 200 3 80 edf

# Feasible — tight (reference run, exit 0)
./codexion 5 850 200 200 200 3 80 edf

# Comfortable margin
./codexion 5 1500 200 200 200 3 80 edf
```

---

#### Summary table (compile=debug=refactor=200ms, cooldown=80ms)

| N | Typical 2nd-compile log (wall) | Min burnout (reference runs, EDF) | Guaranteed safe burnout `(N-1)*(C+D)+C+Dbg+R` | Guaranteed safe example |
|:---:|:---:|:---:|:---:|:---|
| 1 | never | always fails | — | — |
| 2 | ~601ms | > 601ms | > 880ms | `./codexion 2 900 200 200 200 3 80 fifo` |
| 3 | ~845ms | > 844ms | > 1160ms | `./codexion 3 1200 200 200 200 3 80 fifo` |
| 4 | ~1126ms | > ~605ms (see § Mathematical accuracy) | > 1440ms | `./codexion 4 1500 200 200 200 3 80 edf` |
| 5 | ~1126ms | > ~800ms (see § Mathematical accuracy) | > 1720ms | `./codexion 5 1800 200 200 200 3 80 edf` |

`C` = compile, `D` = cooldown, `Dbg` = debug, `R` = refactor (all 200ms, cooldown 80ms above).

## Blocking cases handled

- **Deadlock prevention (Coffman's conditions):**
  - Each coder takes its two dongles in **strict index order**: Phase 1 on `f`,
    Phase 2 on `s` (`f < s`). Mutexes are **not** nested across the two phases;
    only `holder[]` bridges the gap. This breaks circular **mutex** wait.
  - Between attempts, a coder holds **neither** mutex; while compiling it holds
    both dongles only via `holder` (not via mutex).

- **Starvation prevention (dual queues + EDF / FIFO):**
  - Under EDF, priority is derived from
    `last_compile_start + time_to_burnout` (with a deterministic tie-break).
  - Each dongle has **two** fixed 2-slot min-heaps (`t_priority_queue`): one for
    waiters whose ordered pair has **`f` = this index**, one for **`s` = this
    index**. At most two coders ever touch a given edge, but f-role and s-role
    waiters no longer share one heap, avoiding head-of-line blocking between
    phases.

- **Cooldown handling and wake‑up logic:**
  - When a dongle is released, `cooldown_until` is set to
    `now + dongle_cooldown`.
  - Releasing a dongle calls `wake_all_dongles()`, which briefly locks each
    dongle mutex and broadcasts on its condition variable. Waiting coders wake
    and re‑check Phase 1 or Phase 2 conditions (holder, cooldown, heap root).

- **Precise burnout detection:** A dedicated monitor thread checks approximately
  every 1 ms whether any coder has exceeded its burnout deadline. Burnout is
  detected well within the required 10 ms tolerance.

- **Log serialization:** All log output goes through `safe_log()`, which locks
  a `log_mutex` before `printf`. This prevents two messages from interleaving on
  a single line.

## Thread synchronization mechanisms

- **`pthread_mutex_t` (per dongle):** Each dongle has a mutex protecting its state (`cooldown_until`, `holder`) and **both** heap structures (`request_queue`, `request_queue_s`). Coders lock that mutex during Phase 1 **or** Phase 2 waits on this dongle, and during release.

- **`pthread_cond_t` (per dongle):**
  - Each dongle has one condition variable. Phase 1 waiters use
    `pthread_cond_timedwait` on **`dongles[f].cond`**; Phase 2 waiters on
    **`dongles[s].cond`**.
  - `dongle_release()` and `signal_stop()` call `wake_all_dongles()`, which,
    while holding each dongle’s mutex, broadcasts on every condvar so all
    waiters can re‑evaluate or exit on stop.

- **`pthread_mutex_t` (log_mutex):** Serializes all `printf` calls so log lines are atomic.

- **`pthread_mutex_t` (stop_mutex):** Protects `sim->stop` and `sim->num_coders_finished`. When burnout or success is detected, `signal_stop()` sets `stop = 1` and broadcasts on all dongle condvars so blocked coders wake and exit.

- **`pthread_mutex_t` (per coder_data):** Protects `last_compile_start` and `compile_count` so the monitor can safely read them while coders update them.

- **Race condition prevention:** The monitor never holds `stop_mutex` while
  locking `coder_data[i].mutex`, avoiding deadlock. If Phase 2 aborts after Phase
  1, `holder[f]` is cleared and `wake_all_dongles` runs so no dongle stays
  stuck in a partial allocation.

## FAQ

### Are FIFO and EDF applied to coders or to dongles?

They are applied **per dongle**, not per coder. Every dongle has **two** heaps:
`request_queue` (f-role) and `request_queue_s` (s-role). When a coder compiles,
it enqueues on **`dongles[f].request_queue`** for Phase 1 and on
**`dongles[s].request_queue_s`** for Phase 2, using the same priority for both:

```
FIFO: priority = get_time_ms()
EDF:  derived from last_compile_start + time_to_burnout (see `compute_priority`)
```

A coder can only pass a phase when it sits at **position 0** in the heap it is
currently using. The scheduler does not schedule threads directly — only heap
order.

---

### How does FIFO scheduling work in practice?

FIFO assigns `priority = get_time_ms()` at enqueue time. The 2-slot min-heap
serves whoever enqueued first at each dongle. `heapify_up` is a single
compare-and-swap: if `nodes[1].priority < nodes[0].priority`, swap; otherwise
leave. Equal timestamps preserve insertion order.

In practice, coders enqueue in startup order (1, 2, 3, …), so FIFO produces
a stable round-robin: 1 → 2 → 3 → 4 → 1 → 2 → 3 → 4 …

---

### How does EDF scheduling work in practice?

EDF assigns `priority = last_compile_start + time_to_burnout` — the coder's
absolute burnout deadline. The coder closest to burning out is served first
at each dongle.

All coders start with `last_compile_start = sim->start_time`, so their initial
deadlines are equal and the first round is identical to FIFO. After round 1,
the coder that compiled first (e.g. coder 1 at t=0ms) gets a new deadline of
`0 + burnout`, while coder 2 (compiled at t=282ms) gets `282 + burnout`. Coder
1's deadline is earlier → EDF serves it first again. Under comfortable
parameters the ordering stays 1 → 2 → 3 → 4, indistinguishable from FIFO.

EDF diverges from FIFO only when a coder has **fallen behind schedule** — its
`last_compile_start` is older than the others', making its deadline the
earliest. EDF then promotes it to the front to prevent burnout.

---

### Do FIFO or EDF increase throughput?

No. **Both schedulers produce the same throughput** under comfortable
parameters — empirically within 1–2ms of each other.

The throughput bottleneck is the dongle topology and contention on shared
edges, not scheduling policy. Coders 1 and 2 both use dongle **0** as **`f`**, so
they compete on **`dongles[0].request_queue`** in Phase 1; that serializes
their starts even when other pairs (e.g. 1 and 3) are disjoint in the abstract
graph.

Empirical data (N=4, burnout=2000ms, compile=debug=refactor=200ms,
cooldown=80ms, 40 total compiles):

| Scheduler | Last compile timestamp |
|-----------|----------------------|
| FIFO      | ~10690ms             |
| EDF       | ~10691ms             |

---

### If throughput is the same, what is the difference?

FIFO and EDF differ only at **marginal burnout values** — when the cycle time
is close to `time_to_burnout`.

Empirical data (N=3, burnout=855ms, compile=debug=refactor=200ms,
cooldown=80ms, 10 runs each):

| Scheduler | Burnout events | Runs succeeded |
|-----------|---------------|----------------|
| FIFO      | 1 run failed  | 9/10           |
| EDF       | 0 runs failed | 10/10          |

When OS jitter causes one coder to fall behind, EDF detects the shrinking
deadline and promotes that coder toward the front of each **relevant** heap
(f-queue then s-queue as it progresses). FIFO does not react to deadline
proximity at all.

**Recommendation:** use EDF when `time_to_burnout` is tight relative to the
cycle time; use FIFO when the guaranteed-safe formula is comfortably met.

---

### Can EDF cause burnouts that FIFO avoids?

Yes. At intermediate burnout values EDF can trigger a burnout that FIFO would
survive.

Empirical data (N=3, burnout=1000ms, 5 compiles required):

| Scheduler | Total compiles | Burnouts |
|-----------|---------------|---------|
| FIFO      | 15 (all done) | 0       |
| EDF       | 9 (stopped)   | 1       |

EDF's reordering helps the most-at-risk coder but can inadvertently delay
another coder past its deadline if the priority calculations conflict with the
current wait state. FIFO's fixed insertion-order is less clever but more
predictable.

---

### Why does N=1 always burn out?

In a circle of 1, only 1 dongle exists. The program requires 2 dongles to
compile. `acquire_two_dongles` returns -1 immediately when `num_coders < 2`,
so the coder never compiles and the monitor fires the burnout. This is expected
behavior, not a bug.

---

### Why does compilation look sequential even when N=4 allows parallel pairs?

Topology says coders 1 and 3 share no dongles and could compile simultaneously.
In practice, **Phase 1** contention on shared **f** queues (e.g. coders 1 and 2
both need `f = 0`) and **Phase 2** ordering on **s** heaps still serialize work
often enough that logs look like a round-robin (1 → 2 → 3 → 4 → …) even when
the graph would allow more parallelism in principle.

---

### What does `safe_log` do and why is it needed?

`safe_log` serializes all output through `log_mutex` so that two threads
cannot interleave their `printf` calls on a single line. It also checks
`sim->stop` before printing: if the simulation has stopped, only `"burned out"`
messages are allowed through, so no state-change lines appear after the
simulation ends.

```c
void safe_log(t_sim *sim, int coder_id, const char *msg)
{
    pthread_mutex_lock(&sim->stop_mutex);
    if (sim->stop && strcmp(msg, "burned out") != 0)
        return (pthread_mutex_unlock(&sim->stop_mutex), (void)0);
    pthread_mutex_unlock(&sim->stop_mutex);
    pthread_mutex_lock(&sim->log_mutex);
    ts = get_time_ms() - sim->start_time;
    printf("%ld %d %s\n", ts, coder_id, msg);
    pthread_mutex_unlock(&sim->log_mutex);
}
```

---

## Faced issues

### 1. Coder ghosting (head-of-line blocking)

**Reason:** A single wait queue per dongle mixed everyone who needed that dongle, no matter whether it was their **first** dongle (`f`) or **second** (`s`) in the ordered pair `(f, s)`. A thread could sit at the **root** of that heap while still unable to proceed because its **partner** dongle was busy or in cooldown. Other coders who only needed the same physical dongle as their **`s`** were blocked behind that waiter even when the dongle was otherwise free for them—queue-based scheduling amplified into **starvation** and burnout margins worse than simple cycle-time math suggested.

**Solution:** Split each dongle into two heaps—`request_queue` (Phase 1: this index is `f`) and `request_queue_s` (Phase 2: this index is `s`)—and acquire in **two phases**: dequeue from the f-queue as soon as the lower dongle is taken (`holder[f]`), then enqueue and wait on the **s-queue** for the upper dongle. Phase‑1 and Phase‑2 waiters no longer share one head-of-line slot.

---

## Resources

- AI was used to clarify the subject, and discuss deadlock prevention strategies.

- [POSIX Threads (pthreads) - IEEE Std 1003.1](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/pthread.h.html)
- [Mutex and condition variable usage](https://man7.org/linux/man-pages/man3/pthread_mutex_lock.3.html)
- [Earliest Deadline First scheduling](https://en.wikipedia.org/wiki/Earliest_deadline_first_scheduling)
- [Coffman conditions (deadlock)](https://en.wikipedia.org/wiki/Deadlock#Necessary_conditions)
