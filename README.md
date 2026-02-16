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

**Example:**

```bash
./codexion 4 410 200 200 200 5 60 fifo
```

## Blocking cases handled

- **Deadlock prevention (Coffman's conditions):** Dongles are always acquired in a global order (lower index first). When a coder needs left and right dongles, they acquire `min(left_idx, right_idx)` before `max(left_idx, right_idx)`, ensuring a consistent lock ordering and breaking the circular wait condition.

- **Starvation prevention:** Under EDF scheduling, coders are served by earliest burnout deadline (`last_compile_start + time_to_burnout`). A min-heap priority queue orders requests so the coder closest to burnout gets dongles first when they compete.

- **Cooldown handling:** After a dongle is released, `cooldown_until` is set to `now + dongle_cooldown`. Coders waiting on that dongle are woken by `pthread_cond_broadcast` but only proceed when `now >= cooldown_until`, ensuring the cooldown is respected.

- **Precise burnout detection:** A dedicated monitor thread checks every 1 ms whether any coder has exceeded their burnout deadline. Burnout is detected within 1 ms of the actual time, satisfying the &lt;10 ms requirement.

- **Log serialization:** All log output goes through `safe_log()`, which locks a `log_mutex` before `printf`. This prevents two messages from interleaving on a single line.

## Thread synchronization mechanisms

- **`pthread_mutex_t` (per dongle):** Each dongle has a mutex protecting its state (`cooldown_until`, `holder`) and its request queue. Coders lock the dongle mutex when acquiring or releasing.

- **`pthread_cond_t` (per dongle):** Each dongle has a condition variable. Coders wait on it when the dongle is unavailable or they are not at the front of the queue. On release, `pthread_cond_broadcast` wakes all waiters so they can re-check conditions.

- **`pthread_mutex_t` (log_mutex):** Serializes all `printf` calls so log lines are atomic.

- **`pthread_mutex_t` (stop_mutex):** Protects `sim->stop` and `sim->num_coders_finished`. When burnout or success is detected, `signal_stop()` sets `stop = 1` and broadcasts on all dongle condvars so blocked coders wake and exit.

- **`pthread_mutex_t` (per coder_data):** Protects `last_compile_start` and `compile_count` so the monitor can safely read them while coders update them.

- **Race condition prevention:** The monitor never holds `stop_mutex` while locking `coder_data[i].mutex`, avoiding deadlock. Coders acquire dongles in fixed order to prevent circular wait.

## Resources

- [POSIX Threads (pthreads) - IEEE Std 1003.1](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/pthread.h.html)
- AI was used to clarify the subject, design the dongle/coder layout, and discuss deadlock prevention strategies. All implementation was written and reviewed by the author.
- [Mutex and condition variable usage](https://man7.org/linux/man-pages/man3/pthread_mutex_lock.3.html)
- [Earliest Deadline First scheduling](https://en.wikipedia.org/wiki/Earliest_deadline_first_scheduling)
- [Coffman conditions (deadlock)](https://en.wikipedia.org/wiki/Deadlock#Necessary_conditions)
