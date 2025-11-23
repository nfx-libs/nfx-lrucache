# nfx-lrucache TODO

Project roadmap and task tracking for the nfx-lrucache library.

### Todo

- [ ] Add optional capacity limits by memory (bytes) in addition to item count
- [ ] Expose runtime metrics (hits, misses, evictions, average latency)
- [ ] Add an eviction observer callback API for resource cleanup
- [ ] Stress-test thread-safety with sanitizers (ASan, TSan, UBSan) in CI
- [ ] Add optional lock-striping or sharded caches for lower contention
- [ ] Consider `std::shared_mutex` for read-heavy workloads (reduce lock contention)
- [ ] Add `set()` method for explicit cache insertion without factory function
- [ ] Add `contains()` method for existence check without value retrieval
- [ ] Add iterator support for cache traversal in LRU order
- [ ] Make `MAX_CLEANUP_PER_CYCLE` configurable and optionally adaptative (currently hardcoded to 10, may cause memory bloat in high-churn scenarios)
- [ ] Add adaptive cleanup strategy (clean more entries when expiration rate is high)
- [ ] Add usage examples for high-concurrency scenarios
- [ ] Add memory profiling benchmarks

### In Progress

- NIL

### Done ✓

- NIL
