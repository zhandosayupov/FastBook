# Design decisions

Format: what was chosen, why, what was rejected.

## D-001 Release optimization level: -O2

Chosen: `-O2 -DNDEBUG`.
Later: Use -O3 and benchmark.

## D-002 Sanitizers in separate presets

ASan+UBSan in `asan`, TSan in `tsan`: they cannot be combined in one binary.

## D-003 -Werror only on own code

`-Wall -Wextra -Wpedantic -Werror` is applied through the `fastbook_warnings` interface library to
our own targets, tests and benchmarks included. It is not applied to the fetched third-party code
(GoogleTest, Google Benchmark), which we do not control.

## D-004 Plain integer aliases for Price, Quantity, OrderID

Chosen: `using Price = uint32_t; using Quantity = uint32_t; using OrderID = uint64_t;`, and
`enum class Side : uint8_t`.
Why: simplest thing that works for F1; ITCH prices are integers with 4 implied decimals, so no
`double` anywhere in the book (no rounding, exact equality on price levels).
Rejected for now: strong typedefs (wrapper structs). They would stop mixing up price and quantity.
Known cost: `Price` and `Quantity` are the same type, so the compiler does not catch a swap. This
already happened once: `Level{price, quantity, list}` was built with the fields in the wrong order
and compiled silently. Revisit if more of these show up.

## D-005 The index stores where an order lives, the queue stores only id and quantity

Chosen: `Order` in the level queue is `{orderId, quantity}`. The index
`unordered_map<OrderID, OrderLoc>` holds `OrderLoc{side, price, list iterator}`.
Why: cancel, execute and remove find the order in O(1) from the id alone (the ITCH messages carry
no side or price), and the side and price are stored once per order instead of in every queue
element.
Rejected: keeping `side` and `price` inside `Order` (duplicates what the level already knows);
searching the level queue with `remove_if` (O(n) per level).

## D-006 std::list for the level queue

Chosen: `std::list<Order>` per price level (FIFO: new orders go to the tail).
Why: removing an order from the middle by iterator is O(1), and iterators and references to other
elements stay valid on insert and erase, so storing list iterators in the index is safe. A rehash of
the index (`unordered_map`) invalidates only the map's own iterators, not the stored list iterators.
Rejected: `std::vector` and `std::deque`: removing from the middle shifts elements (O(n)) and
invalidates iterators.
Cost: one allocation per order (list node). `add` costs 2 allocations (list node and index node),
3 when it creates a new price level (map node), plus an occasional index rehash. This is the
baseline for the F4 optimizations.

## D-007 Errors are counters and a bool result, not exceptions

Chosen: operations return `bool`; every rejected call increments a counter per kind
(`ORDER_ID_EXISTS`, `ORDER_ID_NOT_FOUND`, `INSUFFICIENT_QUANTITY`, `INVALID_QUANTITY`), readable
through `OrderBook::error_count(ErrorCode)`. A rejected call leaves the book unchanged.
Why: no exceptions in the hot path; on a correct feed the counters must stay 0, so a non-zero value
points to a parser or book bug.
Rejected: throwing exceptions; storing a list of failed orders (allocations in the hot path).

## D-008 replace argument order (deviates from PLAN.md)

Chosen: `replace(oldId, newId, quantity, price)`. `PLAN.md` has `replace(old_id, new_id, price, qty)`.
Why kept: it is what was implemented and tested.
Known cost: `add` takes `(id, side, price, quantity)`, so `replace` is inconsistent with it, and
both fields are `uint32_t`, so swapping them compiles silently (see D-004). Revisit together with
strong types: the natural order is `(oldId, newId, price, quantity)`.
Semantics: the side is taken from the old order; the new order goes to the tail of its queue (loses
priority); all checks happen before any change, so a failed `replace` changes nothing.

## D-009 One book per instrument, index per book (not implemented yet)

Decision for F2/F3: `std::vector<OrderBook>` indexed by the ITCH Stock Locate (`uint16_t`), each
book with its own order index.
Why: the locate is a ready-made dense index, books do not share state, and each index stays small.
Open: whether a shared index would be faster or smaller; decide with a measurement on real data in
F3/F4. Nothing in the current code depends on this choice.

## D-010 best_bid, best_ask and depth return copies

Chosen (for F1): they return `Level` / `vector<Level>` by value, including a copy of the order
queue.
Why: simplest and safe against later changes to the book; tests can inspect queue order.
Known cost: copies a whole `std::list` per call. Replace with a lightweight view (price and total
quantity) in F4, and measure it.

## D-011 Testing strategy: unit tests, invariants, reference book

Chosen: three layers.
1. Unit tests for each operation and for queue priority (FIFO, cancel keeps position, replace loses
   it).
2. `OrderBook::check_invariants()` (level total equals sum of orders, no empty levels, every queue
   entry is in the index and points to itself, index size equals total orders), called after every
   operation in the random test.
3. A random differential test against `tests/reference_book.h`, a deliberately naive book (one
   vector of orders, linear search), comparing results, `best_*`, `depth` and queue order after
   every operation.
Why: `check_invariants` catches broken internal structure, the reference book catches wrong answers;
they find different bugs. The reference book is only a test oracle and is reused in F4 to check
optimizations.
Not an invariant: best bid < best ask. It can legitimately be violated (auctions, special states),
so it is counted, not asserted.

## D-012 Formatting: clang-format, 4-space indent, checked in CI

Chosen: `.clang-format` based on Google style with 4-space indent, column limit 100, access
modifiers flush with the class, include blocks preserved. CI has a `format` job running
`clang-format --dry-run --Werror` on all C++ files.
Why: 4-space indent is the author's preference; the check keeps formatting diffs out of reviews.
