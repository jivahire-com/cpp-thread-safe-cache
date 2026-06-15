# NOTES

## Who was affected?

1. Operators on call needed visibility into whether the cache was effective.
2. Systems depending on cached data needed notification when entries were evicted.

## What changed?

- Fixed the LRU eviction bug by ensuring eviction occurs when the cache reaches capacity before inserting a new item.
- Added thread safety using a single mutex protecting all shared state.
- Added cache metrics:
  - hits
  - misses
  - evictions
  - hit rate
- Added optional eviction callbacks.

## Why this approach?

A single mutex is simple and correct because both get() and put() modify cache state. More complex locking strategies would increase maintenance burden without improving concurrency.

Metrics provide operational visibility with minimal overhead through atomics.

Callbacks allow integration with other systems without coupling the cache to specific behaviors.

## What would I do next?

- Add TTL support for stale entries.
- Benchmark under realistic workloads.
- Explore segmented LRU or sharded caches to improve throughput under heavy contention.
- Add Prometheus/OpenTelemetry integration for metrics export.