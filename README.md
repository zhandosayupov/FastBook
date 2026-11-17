# FastBook

A limit order book in C++20, built as the core of a NASDAQ TotalView-ITCH 5.0 feed handler: read
the binary feed, rebuild the order book per instrument (market-by-order), serve best bid/ask and
market depth.

This is **not a matching engine**. The exchange has already matched the orders; ITCH reports the
outcome (add, execute, cancel, delete, replace). FastBook reproduces the book state from those
messages, it does not match anything.

## Status

Work in progress, built in small steps. Each step is merged only with green CI.

| Step | What | State |
|------|------|-------|
| F0 | CMake presets (debug, release, asan, tsan), CI, GoogleTest, Google Benchmark | done |
| F1 | Order book: add, execute, cancel, remove, replace, best bid/ask, depth | done |
| F2 | ITCH 5.0 binary parser | not started |
| F3 | Replay of a recorded ITCH day, baseline measurements | not started |
| F4 | Differential check on a full day, optimizations one at a time, each measured | not started |
| F5 | Lock-free SPSC queue, two-thread pipeline | not started |

There are **no performance numbers yet**, on purpose. They will be added only when measured, with
the hardware, compiler, command, and method next to them.

## Design (F1)

Full reasoning, with rejected alternatives, is in [docs/DESIGN.md](docs/DESIGN.md). Short version:

- **Prices are integers** (ITCH 4 implied decimals), never `double`: exact comparison of price
  levels, no rounding.
- **Levels**: `std::map` per side (bids descending, asks ascending). Each level keeps its total
  quantity and a FIFO queue of orders (`std::list`), so time priority within a price is explicit.
- **Index**: `unordered_map<OrderID, {side, price, list iterator}>`. An ITCH cancel/execute/delete
  carries only the order id, so the book finds the order in O(1) and removes it from its queue in
  O(1). Iterators of a `std::list` stay valid when other elements are inserted or erased, which is
  why they are safe to store.
- **Priority rules**: partial cancel keeps the queue position; `replace` takes the side of the old
  order and puts the new order at the tail (priority is lost).
- **Errors are counters, not exceptions.** Operations return `bool` and bump a per-kind counter;
  a rejected call leaves the book unchanged. On a correct feed all counters must stay at zero, so a
  non-zero value points to a parser or book bug.
- **Known costs, left for F4 on purpose:** `add` allocates 2 nodes (3 when it creates a price
  level), and `best_bid`/`best_ask`/`depth` return copies of the level including its order queue.
  The F1 version is the "before" row of the optimization table; it is deliberately simple.

## Correctness

- **Unit tests** for every operation and for queue priority (FIFO, partial cancel keeps position,
  replace loses it).
- **Invariants**: `OrderBook::check_invariants()` verifies the internal structure (level total equals
  the sum of its orders, no empty levels, every queue entry is in the index and points to itself,
  index size equals the number of queued orders).
- **Differential random test** against a deliberately naive reference book (one vector, linear
  search): the same random operation stream goes to both books, and results, best prices, depth and
  queue order are compared after every operation. Failures print the seed and step.
- **Sanitizers**: the whole test suite runs under ASan+UBSan and under TSan in CI.
- **Not asserted**: best bid < best ask. Auctions and special market states can violate it on real
  data, so it is something to count, not to assert.

## Build and test

Requirements: CMake 3.20+, Ninja, a C++20 compiler. Developed and tested with GCC 13.3 on
Ubuntu 24.04 (WSL2) and in CI on `ubuntu-24.04`; Clang has not been tried. Google Test and Google
Benchmark are fetched by CMake.

```bash
cmake --preset release && cmake --build --preset release && ctest --preset release
```

Other presets: `debug`, `asan` (ASan + UBSan), `tsan`. Each builds in `build/<preset>/`. Warnings
are errors (`-Wall -Wextra -Wpedantic -Werror`) for our own code.

CI runs all four presets plus a `clang-format` check on every push and pull request.

## Layout

```
include/fastbook/   public headers: types.h, order_book.h
src/                order book implementation
tests/              unit tests, random differential test, reference book (test oracle)
bench/              Google Benchmark targets (placeholder until F3)
docs/DESIGN.md      design decisions: chosen, why, rejected, known cost
```

## What is not here (yet)

- ITCH parsing and replay (F2, F3).
- Any multi-instrument routing: one `OrderBook` per instrument indexed by Stock Locate is the
  planned design (see DESIGN.md, D-009).
- Multithreading. The book itself is single-threaded; the two-thread pipeline is F5.
- Performance claims of any kind, until F3.
