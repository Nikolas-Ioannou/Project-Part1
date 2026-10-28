// Minimal thread pool helper built on std::thread, used to run independent work items
// (one image, one block of points) on all CPU cores

#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <exception>
#include <mutex>
#include <thread>
#include <vector>

// Number of worker threads: all hardware threads (at least 1)
inline unsigned numThreads() {
    unsigned t = std::thread::hardware_concurrency();
    return t ? t : 1;
}

// Calls f(i) for every i in [0, n) on numThreads() threads. Indices are handed out in
// blocks of `grain`, so uneven work (e.g. images of different sizes) stays balanced.
// f must be safe to call concurrently for different i. Results only depend on i, never on
// which thread ran it, so the output is the same for any number of threads
// The first exception thrown by f stops the remaining work and is rethrown here
template <typename F>
void parallelFor(size_t n, F&& f, size_t grain = 1) {
    if (n == 0) return;
    grain = std::max<size_t>(grain, 1);

    std::atomic<size_t> next{0};  // start of the next block to hand out
    std::exception_ptr error;
    std::mutex errorMutex;

    auto worker = [&]() {
        for (;;) {
            const size_t begin = next.fetch_add(grain);
            if (begin >= n) return;
            const size_t end = std::min(n, begin + grain);
            try {
                for (size_t i = begin; i < end; ++i) f(i);
            } catch (...) {
                std::lock_guard<std::mutex> lock(errorMutex);
                if (!error) error = std::current_exception();
                next = n;  // stop handing out work
                return;
            }
        }
    };

    const size_t blocks = (n + grain - 1) / grain;
    const size_t t = std::min<size_t>(numThreads(), blocks);
    std::vector<std::thread> threads;
    threads.reserve(t);
    for (size_t i = 0; i < t; ++i) threads.emplace_back(worker);
    for (auto& th : threads) th.join();
    if (error) std::rethrow_exception(error);
}
