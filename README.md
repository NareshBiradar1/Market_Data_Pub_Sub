# Low-Latency Market Data Pub/Sub

A C++17 market data system with dual-path distribution: shared memory (SPSC lock-free queue) and TCP (Boost.Asio).

## Structure

- `include/common.h` — Shared types: `MarketData`, lock-free `SPSCQueue`, `TSCClock`, CPU pinning
- `src/publisher.cpp` — Generates random market data, publishes via SHM + TCP (CPU 0)
- `src/shm_consumer.cpp` — Consumes from SHM ring buffer (CPU 1)
- `src/tcp_consumer.cpp` — Consumes from TCP stream (CPU 2)

## Build

```bash
mkdir build && cd build
cmake .. -DBoost_NO_BOOST_CMAKE=ON
make
```

## Run

```bash
./publisher &      # starts TCP listener on :9090
./shm_consumer &   # attaches to SHM segment
./tcp_consumer &   # connects to publisher via TCP
```

## Dependencies

- fmt
- Boost.Asio (+ system)
- nlohmann_json
- librt
