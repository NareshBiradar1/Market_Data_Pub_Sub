#ifndef COMMON_H
#define COMMON_H

#define _GNU_SOURCE  
#include <fmt/format.h>
#include <atomic>
#include <cstdint>
#include <x86intrin.h>
#include <thread>
#include <random>
#include <vector>
#include <sched.h>
#include <chrono>
#include <pthread.h>
#include <cstdio>

struct MarketData {
    char instrument[16];
    double bid;
    double ask;
    uint64_t timestamp_ns;

    size_t to_json(char* buf, size_t len) const {
        auto res = fmt::format_to_n(buf, len,
            "{{\"instrument\":\"{}\",\"bid\":{:.2f},\"ask\":{:.2f},\"timestamp_ns\":{}}}\n",
            instrument, bid, ask, timestamp_ns);
        return res.size;
    }
};

constexpr size_t RING_BUFFER_SIZE = 4096;
constexpr size_t MASK = RING_BUFFER_SIZE - 1;

struct SPSCQueue {
    alignas(64) std::atomic<size_t> write_idx;
    alignas(64) std::atomic<size_t> read_idx;
    alignas(64) MarketData buffer[RING_BUFFER_SIZE];
    void init(){
        write_idx.store(0,std::memory_order_relaxed);
        read_idx.store(0,std::memory_order_relaxed);
    }
    bool push(const MarketData& data) {
        size_t w = write_idx.load(std::memory_order_relaxed);
        size_t r = read_idx.load(std::memory_order_acquire);

        if (w - r >= RING_BUFFER_SIZE)
            return false;

        buffer[w & MASK] = data;
        write_idx.store(w + 1, std::memory_order_release);
        return true;
    }

    bool try_pop(MarketData& data) {
        size_t w = write_idx.load(std::memory_order_acquire);
        size_t r = read_idx.load(std::memory_order_relaxed);

        if (r == w)
            return false;

        data = buffer[r & MASK];
        read_idx.store(r + 1, std::memory_order_release);
        return true;
    }
};

struct TSCClock {
    double ns_per_cycle;
    uint64_t base_tsc;
    uint64_t base_ns;

    TSCClock() {
        auto t1 = std::chrono::high_resolution_clock::now();
        uint64_t r1 = __rdtsc();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        auto t2 = std::chrono::high_resolution_clock::now();
        uint64_t r2 = __rdtsc();
        base_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1.time_since_epoch()).count();
        base_tsc = r1;
        ns_per_cycle = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count() / (r2 - r1);
    }

    inline uint64_t now_ns() const {
        return base_ns + static_cast<uint64_t>((__rdtsc() - base_tsc) * ns_per_cycle);
    }
};

inline void pin_thread(int cpu_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_id, &cpuset);
    pthread_t current_thread = pthread_self();
    pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

#endif