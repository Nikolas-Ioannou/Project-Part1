// Keeps the k closest candidates seen so far (used by every search method)

#pragma once

#include <algorithm>
#include <limits>
#include <utility>
#include <vector>

class TopK {
public:
    explicit TopK(int k) : k_(k) { heap_.reserve(k); }

    // Offers candidate idx at (squared) distance dist. It is kept if it is among the k
    // closest so far. Equal distances are ordered by index, so results are deterministic
    void push(float dist, int idx) {
        const Entry e{dist, idx};
        if (static_cast<int>(heap_.size()) < k_) {
            heap_.push_back(e);
            std::push_heap(heap_.begin(), heap_.end());
        } else if (k_ > 0 && e < heap_.front()) {
            // Replace the current worst (the top of the max-heap)
            std::pop_heap(heap_.begin(), heap_.end());
            heap_.back() = e;
            std::push_heap(heap_.begin(), heap_.end());
        }
    }

    // Distance of the worst kept candidate, or infinity while fewer than k are kept
    // A candidate farther than this cannot enter the result
    float worst() const {
        return static_cast<int>(heap_.size()) < k_ ? std::numeric_limits<float>::infinity()
                                                   : heap_.front().first;
    }

    // Indices of the kept candidates, closest first (at most k)
    std::vector<int> sorted() const {
        std::vector<Entry> v = heap_;
        std::sort(v.begin(), v.end());
        std::vector<int> out;
        out.reserve(v.size());
        for (const auto& e : v) out.push_back(e.second);
        return out;
    }

private:
    // (distance, index); pair's operator< compares distance first, then index
    using Entry = std::pair<float, int>;
    int k_;
    std::vector<Entry> heap_;  // max-heap: the worst kept candidate is at the front
};
