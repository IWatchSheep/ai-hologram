// Purpose: Checks text parsing, rendering, and world-state validity.
// Introduced in Stage 1.
// Architecture: These checks protect the shared world representation.
/*
 * This file is slightly over ~200 lines because hand-drawn test layouts stay readable.
 * has_value() checks for a world; -> accesses its members after that check, and * gets the
 * contained world.
 */

#include "test_support.h"
#include "test_suites.h"
#include "world/world_text.h"

#include <iostream>
#include <string>
#include <vector>

namespace ai_hologram {
namespace {

std::vector<std::string> demoLayout() {
    return {
        "##########",
        "#........#",
        "#........#",
        "#..A.O..D#",
        "#........#",
        "#........#",
        "#...##...#",
        "#........#",
        "#........#",
        "##########"
    };
}

std::vector<std::string> objectOnDestinationLayout() {
    return {
        "##########",
        "#........#",
        "#........#",
        "#..A.....#",
        "#....*...#",
        "#........#",
        "#........#",
        "#........#",
        "#........#",
        "##########"
    };
}

std::vector<std::string> agentOnDestinationLayout() {
    return {
        "##########",
        "#........#",
        "#........#",
        "#..+.....#",
        "#....O...#",
        "#........#",
        "#........#",
        "#........#",
        "#........#",
        "##########"
    };
}

WorldState directValidWorld() {
    WorldState state{};
    for (int index = 0; index < gridSize; ++index) {
        state.walls[0][static_cast<std::size_t>(index)] = true;
        state.walls[static_cast<std::size_t>(gridSize - 1)]
                   [static_cast<std::size_t>(index)] = true;
        state.walls[static_cast<std::size_t>(index)][0] = true;
        state.walls[static_cast<std::size_t>(index)]
                   [static_cast<std::size_t>(gridSize - 1)] = true;
    }
    state.agent = {2, 2};
    state.object = {4, 4};
    state.destination = {7, 7};
    return state;
}

void checkParseFailures(TestCounter& counter) {
    std::vector<std::string> layout = demoLayout();
    layout.pop_back();
    const ParseResult wrongLineCount = parseWorld(layout);
    check(counter, !wrongLineCount.world.has_value() && !wrongLineCount.error.empty(),
          "Parsing rejects the wrong number of lines with an explanation");

    layout = demoLayout();
    layout[1] = "#.......#";
    check(counter, !parseWorld(layout).world.has_value(),
          "Parsing rejects a line with the wrong length");

    layout = demoLayout();
    layout[0][4] = '.';
    check(counter, !parseWorld(layout).world.has_value(),
          "Parsing rejects a gap in the outer wall");
}

void checkEntityCounts(TestCounter& counter) {
    std::vector<std::string> layout = demoLayout();
    layout[3][3] = '.';
    check(counter, !parseWorld(layout).world.has_value(),
          "Parsing rejects a world with no agent");
    layout = demoLayout();
    layout[3][4] = 'A';
    check(counter, !parseWorld(layout).world.has_value(),
          "Parsing rejects a world with two agents");

    layout = demoLayout();
    layout[3][5] = '.';
    check(counter, !parseWorld(layout).world.has_value(),
          "Parsing rejects a world with no object");
    layout = demoLayout();
    layout[3][4] = 'O';
    check(counter, !parseWorld(layout).world.has_value(),
          "Parsing rejects a world with two objects");

    layout = demoLayout();
    layout[3][8] = '.';
    check(counter, !parseWorld(layout).world.has_value(),
          "Parsing rejects a world with no destination");
    layout = demoLayout();
    layout[3][7] = 'D';
    check(counter, !parseWorld(layout).world.has_value(),
          "Parsing rejects a world with two destinations");
}

void checkDestinationMarkers(TestCounter& counter) {
    const ParseResult objectOnDestination = parseWorld(objectOnDestinationLayout());
    check(counter, objectOnDestination.world.has_value() &&
                       objectOnDestination.world->object == Position{5, 4} &&
                       objectOnDestination.world->destination == Position{5, 4},
          "Parsing reads * as the object on the destination");

    const ParseResult agentOnDestination = parseWorld(agentOnDestinationLayout());
    check(counter, agentOnDestination.world.has_value() &&
                       agentOnDestination.world->agent == Position{3, 3} &&
                       agentOnDestination.world->destination == Position{3, 3},
          "Parsing reads + as the agent on the destination");
}

void checkUnsafeInputs(TestCounter& counter) {
    const std::vector<std::string> emptyLines;
    check(counter, !parseWorld(emptyLines).world.has_value(),
          "Parsing rejects an empty list of lines safely");

    std::vector<std::string> lines(static_cast<std::size_t>(gridSize), "");
    check(counter, !parseWorld(lines).world.has_value(),
          "Parsing rejects empty lines safely");

    lines = demoLayout();
    lines[1] += "A much longer line";
    check(counter, !parseWorld(lines).world.has_value(),
          "Parsing rejects lines much longer than the grid safely");
}

void checkValidity(TestCounter& counter) {
    const ParseResult parsed = parseWorld(demoLayout());
    check(counter, parsed.world.has_value() && isValidWorld(*parsed.world),
          "The parsed demo world is valid");

    WorldState state = directValidWorld();
    state.walls[2][2] = true;
    check(counter, !isValidWorld(state),
          "A world with the agent on a wall is invalid");

    state = directValidWorld();
    state.object = state.agent;
    check(counter, !isValidWorld(state),
          "A world with the agent and object on the same cell is invalid");

    state = directValidWorld();
    state.walls[0][1] = false;
    check(counter, !isValidWorld(state),
          "A world with a gap in its border wall is invalid");

    state = directValidWorld();
    state.agent = {10, 2};
    check(counter, !isValidWorld(state),
          "A world with a position beyond the grid is invalid");
    state.agent = {2, -1};
    check(counter, !isValidWorld(state),
          "A world with a negative position is invalid");

    check(counter, isWall(state, {-1, 0}) && isWall(state, {10, 0}) &&
                       isWall(state, {0, -1}) && isWall(state, {0, 10}),
          "isWall treats all four outside edges as walls");
}

}  // namespace

void runWorldTextTests(TestCounter& counter) {
    std::cout << "--- World text and validity ---\n";

    const ParseResult parsed = parseWorld(demoLayout());
    check(counter, parsed.world.has_value() &&
                       parsed.world->agent == Position{3, 3} &&
                       parsed.world->object == Position{5, 3} &&
                       parsed.world->destination == Position{8, 3} &&
                       parsed.world->walls[0][0] &&
                       parsed.world->walls[6][4] &&
                       !parsed.world->walls[3][4],
          "A valid layout parses its positions, wall, and floor correctly");

    std::vector<std::string> roundTripInput = demoLayout();
    const ParseResult firstWorld = parseWorld(roundTripInput);
    check(counter, firstWorld.world.has_value() &&
                       renderWorld(*firstWorld.world) == roundTripInput,
          "Rendering the demo layout reproduces its original text");

    roundTripInput = objectOnDestinationLayout();
    const ParseResult objectDestinationWorld = parseWorld(roundTripInput);
    check(counter, objectDestinationWorld.world.has_value() &&
                       renderWorld(*objectDestinationWorld.world) == roundTripInput,
          "Rendering a valid layout with * reproduces its original text");

    roundTripInput = agentOnDestinationLayout();
    const ParseResult agentDestinationWorld = parseWorld(roundTripInput);
    check(counter, agentDestinationWorld.world.has_value() &&
                       renderWorld(*agentDestinationWorld.world) == roundTripInput,
          "Rendering a valid layout with + reproduces its original text");

    checkParseFailures(counter);
    checkEntityCounts(counter);

    std::vector<std::string> unknown = demoLayout();
    unknown[1][1] = '?';
    check(counter, !parseWorld(unknown).world.has_value(),
          "Parsing rejects an unknown character");

    checkDestinationMarkers(counter);
    checkUnsafeInputs(counter);
    checkValidity(counter);
}

}  // namespace ai_hologram
