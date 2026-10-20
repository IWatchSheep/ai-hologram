// Purpose: Implements the project's own seeded random generator
// (SplitMix64, exactly as in plan 3.1).
// Introduced in Stage 4.
// Architecture: This is experimenter code in src/experiment/. No agent file
// may include its header, and no agent ever calls these functions.

#include "experiment/seeded_random.h"

namespace ai_hologram {

SeededRandom::SeededRandom(const std::uint64_t seed) : state_(seed) {}

std::uint64_t SeededRandom::next() {
    // All arithmetic here is std::uint64_t, so it wraps modulo 2^64 by
    // definition: a result too large for 64 bits keeps only its lowest
    // 64 bits. That wrapping is part of the algorithm, not an error.
    state_ = state_ + 0x9E3779B97F4A7C15ULL;
    std::uint64_t z = state_;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

int SeededRandom::below(const int n) {
    if (n < 1) {
        // Callers never do this. Return 0 without drawing a number, so
        // a mistaken call cannot silently shift the whole sequence.
        return 0;
    }
    // Rejection sampling: 2^64 is usually not an exact multiple of n, so
    // simply taking r % n would favour the smallest values slightly.
    // Numbers below the threshold are rejected and drawn again, which
    // makes every value from 0 to n-1 equally likely. The threshold is
    // (2^64 - n) mod n, computed in std::uint64_t as (0 - n) % n, which
    // equals 2^64 mod n. For n == 1 the threshold is 0, so the first
    // draw is always accepted: below(1) returns 0 but still draws once.
    const std::uint64_t bound = static_cast<std::uint64_t>(n);
    const std::uint64_t threshold = (0ULL - bound) % bound;
    std::uint64_t r = next();
    while (r < threshold) {
        r = next();
    }
    return static_cast<int>(r % bound);
}

}  // namespace ai_hologram
