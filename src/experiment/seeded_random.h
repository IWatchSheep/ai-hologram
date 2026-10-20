// Purpose: Declares the project's own seeded random generator (SplitMix64).
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/. No agent file
// may include this header, and no agent ever calls these functions. It is
// used only to generate experiment material (layouts and, later, faults).

#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace ai_hologram {

// Why the project has its own generator: the C++ standard library fixes
// some engines exactly, but not its distributions (for example
// std::uniform_int_distribution may give different numbers with different
// compilers). An experiment must give the same material on any machine, so
// this project uses one small, fully specified generator (plan 3.1, D-030).
//
// A seed is the starting number of the generator. The same seed always
// gives the same sequence of numbers, in the same order. That is what
// makes an experiment reproducible: anyone can regenerate exactly the
// same layouts and fault lists from the recorded seeds.
//
// The order of draws must never change. Each draw advances the state, so
// one extra (or missing) draw changes every later number. Callers must
// therefore draw in exactly the order the plan fixes, even for draws
// whose result turns out not to be needed.
class SeededRandom {
public:
    // The state starts as the seed itself.
    explicit SeededRandom(std::uint64_t seed);

    // The next 64-bit number in the sequence (SplitMix64, plan 3.1).
    std::uint64_t next();

    // A whole number from 0 to n-1, every value equally likely, for
    // n >= 1. below(1) always returns 0 but still uses one next() call.
    // If n < 1, returns 0 without calling next(); callers never do this.
    int below(int n);

    // Fisher-Yates shuffle, from the end: for i = size-1 down to 1, pick
    // j = below(i + 1) and swap elements i and j. Every ordering of the
    // list is equally likely. An empty or one-element list is unchanged
    // and draws no numbers.
    template <class T>
    void shuffle(std::vector<T>& list) {
        if (list.size() < 2) {
            return;
        }
        for (std::size_t i = list.size() - 1; i >= 1; --i) {
            const int j = below(static_cast<int>(i + 1));
            std::swap(list[i], list[static_cast<std::size_t>(j)]);
        }
    }

    // Picks k distinct elements: the first k of a partial Fisher-Yates
    // shuffle from the front. For i = 0 to k-1, pick
    // j = i + below(size - i) and swap elements i and j; the first k
    // elements, in that order, are the result. The list is taken by
    // value (a copy) so the caller's list is unchanged. Callers always
    // have k <= size.
    template <class T>
    std::vector<T> pickDistinct(std::vector<T> list, int k) {
        const int size = static_cast<int>(list.size());
        for (int i = 0; i < k; ++i) {
            const int j = i + below(size - i);
            std::swap(list[static_cast<std::size_t>(i)],
                      list[static_cast<std::size_t>(j)]);
        }
        std::vector<T> result;
        for (int i = 0; i < k; ++i) {
            result.push_back(list[static_cast<std::size_t>(i)]);
        }
        return result;
    }

private:
    // The whole state is this one number.
    std::uint64_t state_;
};

}  // namespace ai_hologram
