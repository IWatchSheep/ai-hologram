// Purpose: Converts between world states and their ten-line text format.
// Introduced in Stage 1.
// Architecture: Parsing and rendering connect readable layouts to the world.

#include "world/world_text.h"

namespace ai_hologram {
namespace {

// An unnamed namespace keeps these helper names private to this source file.
ParseResult rejectedWorld(const std::string& message) {
    // The std::nullopt value marks an error result that contains no parsed world.
    return {std::nullopt, message};
}

bool isBorderCell(const int x, const int y) {
    return x == 0 || x == gridSize - 1 || y == 0 || y == gridSize - 1;
}

struct EntityCounts {
    int agents = 0;
    int objects = 0;
    int destinations = 0;
};

bool haveCorrectLineLengths(const std::vector<std::string>& lines) {
    for (const std::string& line : lines) {
        if (line.size() != static_cast<std::size_t>(gridSize)) {
            return false;
        }
    }
    return true;
}

std::string applyCellCharacter(
    const char cell, const int x, const int y,
    WorldState& state, EntityCounts& counts) {
    switch (cell) {
        case '#':
            state.walls[static_cast<std::size_t>(y)]
                       [static_cast<std::size_t>(x)] = true;
            break;
        case '.':
            break;
        case 'A':
            ++counts.agents;
            state.agent = {x, y};
            break;
        case 'O':
            ++counts.objects;
            state.object = {x, y};
            break;
        case 'D':
            ++counts.destinations;
            state.destination = {x, y};
            break;
        case '*':
            ++counts.objects;
            ++counts.destinations;
            state.object = {x, y};
            state.destination = {x, y};
            break;
        case '+':
            ++counts.agents;
            ++counts.destinations;
            state.agent = {x, y};
            state.destination = {x, y};
            break;
        default:
            return "The world contains an unknown character.";
    }
    return "";
}

std::string parseCell(
    const char cell,
    const int x,
    const int y,
    WorldState& state,
    EntityCounts& counts) {
    if (isBorderCell(x, y) && cell != '#') {
        return "Every border cell must be a wall (#).";
    }
    return applyCellCharacter(cell, x, y, state, counts);
}

std::string parseCells(
    const std::vector<std::string>& lines,
    WorldState& state,
    EntityCounts& counts) {
    for (int y = 0; y < gridSize; ++y) {
        for (int x = 0; x < gridSize; ++x) {
            const char cell = lines[static_cast<std::size_t>(y)]
                                   [static_cast<std::size_t>(x)];
            const std::string error = parseCell(cell, x, y, state, counts);
            if (!error.empty()) {
                return error;
            }
        }
    }
    return "";
}

}  // namespace

ParseResult parseWorld(const std::vector<std::string>& lines) {
    if (lines.size() != static_cast<std::size_t>(gridSize)) {
        return rejectedWorld("A world must contain exactly 10 lines.");
    }

    // Check every row's length before looking at any character in that row.
    if (!haveCorrectLineLengths(lines)) {
        return rejectedWorld("Every world line must contain exactly 10 characters.");
    }

    WorldState state{};
    EntityCounts counts;
    const std::string cellError = parseCells(lines, state, counts);
    if (!cellError.empty()) {
        return rejectedWorld(cellError);
    }

    if (counts.agents != 1) {
        return rejectedWorld("A world must contain exactly one agent (A or +).");
    }
    if (counts.objects != 1) {
        return rejectedWorld("A world must contain exactly one object (O or *).");
    }
    if (counts.destinations != 1) {
        return rejectedWorld("A world must contain exactly one destination (D, * or +).");
    }
    if (!isValidWorld(state)) {
        return rejectedWorld("The parsed world state is not valid.");
    }

    return {std::optional<WorldState>{state}, ""};
}

std::vector<std::string> renderWorld(const WorldState& state) {
    // The caller must provide a valid state; only x/y loop indexes access walls.
    std::vector<std::string> lines(
        static_cast<std::size_t>(gridSize),
        std::string(static_cast<std::size_t>(gridSize), '.'));

    for (int y = 0; y < gridSize; ++y) {
        for (int x = 0; x < gridSize; ++x) {
            const Position cell{x, y};
            char character = '.';

            if (state.walls[static_cast<std::size_t>(y)]
                           [static_cast<std::size_t>(x)]) {
                character = '#';
            } else if (state.agent == cell && state.destination == cell) {
                character = '+';
            } else if (state.object == cell && state.destination == cell) {
                character = '*';
            } else if (state.agent == cell) {
                character = 'A';
            } else if (state.object == cell) {
                character = 'O';
            } else if (state.destination == cell) {
                character = 'D';
            }

            lines[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] = character;
        }
    }

    return lines;
}

}  // namespace ai_hologram
