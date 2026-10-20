// Purpose: Checks RN1-RN8: the seeded random generator gives exactly
// the values of the reference implementation (plan 3.1).
// Introduced in Stage 4.
// Architecture: These checks verify experimenter code only; no agent is
// involved. Every expected value comes from plan 3.1 and is compared
// exactly, because one different draw would change all later material.

#include "experiment/seeded_random.h"
#include "test_suites.h"

#include <iostream>
#include <vector>

namespace ai_hologram {
namespace {

void checkNextValues(TestCounter& counter) {
    SeededRandom first(0);
    check(counter,
          first.next() == 16294208416658607535ULL &&
              first.next() == 7960286522194355700ULL &&
              first.next() == 487617019471545679ULL,
          "RN1: SeededRandom(0) gives the three reference next() values");
    SeededRandom second(1234567);
    check(counter,
          second.next() == 6457827717110365317ULL &&
              second.next() == 3203168211198807973ULL &&
              second.next() == 9817491932198370423ULL,
          "RN2: SeededRandom(1234567) gives the three reference values");
}

void checkBelowShufflePick(TestCounter& counter) {
    SeededRandom belowRandom(42);
    std::vector<int> draws;
    for (int i = 0; i < 10; ++i) {
        draws.push_back(belowRandom.below(10));
    }
    check(counter, draws == std::vector<int>({3, 1, 8, 4, 0, 2, 5, 8, 5, 4}),
          "RN3: SeededRandom(42) gives the ten reference below(10) values");

    SeededRandom shuffleRandom(42);
    std::vector<int> list{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    shuffleRandom.shuffle(list);
    check(counter, list == std::vector<int>({1, 10, 6, 9, 7, 5, 8, 3, 2, 4}),
          "RN4: SeededRandom(42) shuffles 1..10 to the reference order");

    SeededRandom pickRandom(7);
    const std::vector<int> picked =
        pickRandom.pickDistinct(std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}, 3);
    check(counter, picked == std::vector<int>({8, 1, 5}),
          "RN5: SeededRandom(7) pickDistinct(1..10, 3) gives {8, 1, 5}");
}

void checkBelowOneDraw(TestCounter& counter) {
    // below(1) must return 0 and use exactly one draw: afterwards the
    // generator must be in the same state as one that made one next().
    SeededRandom viaBelow(5);
    const int value = viaBelow.below(1);
    const std::uint64_t afterBelow = viaBelow.next();
    SeededRandom viaNext(5);
    viaNext.next();
    const std::uint64_t afterOneNext = viaNext.next();
    SeededRandom fresh(5);
    fresh.next();
    const std::uint64_t secondOfFresh = fresh.next();
    check(counter,
          value == 0 && afterBelow == afterOneNext &&
              afterBelow == secondOfFresh,
          "RN6: below(1) returns 0 and uses exactly one draw");
}

void checkRepeatability(TestCounter& counter) {
    SeededRandom a(99);
    SeededRandom b(99);
    bool same = true;
    for (int i = 0; i < 1000; ++i) {
        if (a.next() != b.next()) {
            same = false;
        }
    }
    SeededRandom one(1);
    SeededRandom two(2);
    check(counter, same && one.next() != two.next(),
          "RN7: same seed repeats 1000 values; seeds 1 and 2 differ first");
}

void checkBelowRanges(TestCounter& counter) {
    bool inRange = true;
    bool seen[7] = {false, false, false, false, false, false, false};
    const int sizes[] = {1, 2, 3, 7, 100, 1000};
    for (const int n : sizes) {
        SeededRandom random(2026);
        for (int i = 0; i < 1000; ++i) {
            const int value = random.below(n);
            if (value < 0 || value >= n) {
                inRange = false;
            }
            if (n == 7 && value >= 0 && value < 7) {
                seen[value] = true;
            }
        }
    }
    bool allSeen = true;
    for (const bool s : seen) {
        allSeen = allSeen && s;
    }
    check(counter, inRange && allSeen,
          "RN8: below(n) stays in 0..n-1 and below(7) shows every value");
}

}  // namespace

void runSeededRandomTests(TestCounter& counter) {
    std::cout << "--- Seeded random ---\n";
    checkNextValues(counter);
    checkBelowShufflePick(counter);
    checkBelowOneDraw(counter);
    checkRepeatability(counter);
    checkBelowRanges(counter);
}

}  // namespace ai_hologram
