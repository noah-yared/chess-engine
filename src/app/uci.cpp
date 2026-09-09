#include "app/uci.h"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string_view>

#include "board/constants.h"
#include "engine_config.h"
#include "move/move_generator.h"
#include "move/uci.h"
#include "search/perft.h"
#include "search/search_types.h"

namespace
{

std::string trim(std::string_view text)
{
    const auto start = text.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos)
        return {};
    const auto end = text.find_last_not_of(" \t\r\n");
    return std::string(text.substr(start, end - start + 1));
}

bool parseInt(std::string_view text, int& value)
{
    const auto trimmed = trim(text);
    if (trimmed.empty())
        return false;
    const char* begin = trimmed.data();
    const char* end = begin + trimmed.size();
    const auto result = std::from_chars(begin, end, value);
    return result.ec == std::errc{} && result.ptr == end;
}

std::string joinPv(const std::vector<Move>& pv)
{
    std::ostringstream oss;
    for (std::size_t i = 0; i < pv.size(); ++i)
    {
        if (i > 0)
            oss << ' ';
        oss << pv[i].uci();
    }
    return oss.str();
}

std::string formatInfoScore(int score, Color sideToMove)
{
    const int stmScore = sideToMove == Color::WHITE ? score : -score;
    constexpr int kMateBand = 25000;

    if (stmScore > kMateBand)
    {
        const int mate = std::max((MATE_SCORE - stmScore + MATE_DEPTH_PENALTY - 1) / MATE_DEPTH_PENALTY,
                                  1);
        return "score mate " + std::to_string(mate);
    }
    if (stmScore < -kMateBand)
    {
        const int mate = std::max((MATE_SCORE + stmScore + MATE_DEPTH_PENALTY - 1) / MATE_DEPTH_PENALTY,
                                  1);
        return "score mate -" + std::to_string(mate);
    }
    return "score cp " + std::to_string(stmScore);
}

int allocateSearchTimeMS(Color sideToMove, int wtimeMS, int btimeMS, int wincMS, int bincMS,
                         std::optional<int> movestogo, int moveOverheadMS)
{
    const int remaining = sideToMove == Color::WHITE ? wtimeMS : btimeMS;
    const int increment = sideToMove == Color::WHITE ? wincMS : bincMS;
    const int movesLeft = std::max(movestogo.value_or(30), 30);
    int budget = remaining / movesLeft + (3 * increment) / 4 - moveOverheadMS;
    const int usable = remaining - moveOverheadMS;
    if (usable <= 0)
        return MIN_SEARCH_TIME_BUDGET;
    if (usable < MIN_SEARCH_TIME_BUDGET)
        return usable;
    return std::clamp(budget, MIN_SEARCH_TIME_BUDGET, usable);
}

struct GoLimits
{
    std::optional<int> depth;
    std::optional<int> movetimeMS;
    std::optional<int> wtimeMS;
    std::optional<int> btimeMS;
    int wincMS{0};
    int bincMS{0};
    std::optional<int> movestogo;
    bool infinite{false};
    std::optional<int> perftDepth;
};

GoLimits parseGoLimits(std::string_view line)
{
    GoLimits limits{};
    std::istringstream iss{std::string(line)};
    std::string token;
    iss >> token; // "go"

    while (iss >> token)
    {
        if (token == "depth")
        {
            int depth = 0;
            if (iss >> depth)
                limits.depth = std::max(depth, 1);
        }
        else if (token == "movetime")
        {
            int movetime = 0;
            if (iss >> movetime)
                limits.movetimeMS = movetime;
        }
        else if (token == "wtime")
        {
            int wtime = 0;
            if (iss >> wtime)
                limits.wtimeMS = wtime;
        }
        else if (token == "btime")
        {
            int btime = 0;
            if (iss >> btime)
                limits.btimeMS = btime;
        }
        else if (token == "winc")
        {
            int winc = 0;
            if (iss >> winc)
                limits.wincMS = winc;
        }
        else if (token == "binc")
        {
            int binc = 0;
            if (iss >> binc)
                limits.bincMS = binc;
        }
        else if (token == "movestogo")
        {
            int movestogo = 0;
            if (iss >> movestogo)
                limits.movestogo = movestogo;
        }
        else if (token == "infinite")
        {
            limits.infinite = true;
        }
        else if (token == "perft")
        {
            int depth = 0;
            if (iss >> depth)
                limits.perftDepth = depth;
        }
    }

    return limits;
}

SearchConfig buildSearchConfig(const GoLimits& limits, Color sideToMove, int threads,
                               int moveOverheadMS)
{
    SearchConfig config{};
    config.limits.maxDepth = limits.depth.value_or(MAX_SEARCH_DEPTH);
    config.setParallelism(threads);

    if (limits.infinite)
        return config;

    if (limits.movetimeMS.has_value())
    {
        config.options.useTimeManagement = true;
        config.limits.timeLimitMS = std::max(MIN_SEARCH_TIME_BUDGET, *limits.movetimeMS);
    }
    else if (limits.wtimeMS.has_value() && limits.btimeMS.has_value())
    {
        const int budget = allocateSearchTimeMS(sideToMove, *limits.wtimeMS, *limits.btimeMS,
                                                limits.wincMS, limits.bincMS, limits.movestogo,
                                                moveOverheadMS);
        config.options.useTimeManagement = true;
        config.limits.timeLimitMS = budget;
    }

    return config;
}

} // namespace

UciEngine::UciEngine(SearchThread& searchThread)
    : searchThread_{searchThread}, position_{Position::fromStartingPosition()}
{
}

void UciEngine::writeLine(const std::string& line)
{
    std::lock_guard lock(outputMutex_);
    std::cout << line << '\n' << std::flush;
}

bool UciEngine::execute(const std::string& line)
{
    const std::string trimmed = trim(line);
    if (trimmed.empty())
        return true;

    if (trimmed == "uci")
    {
        handleUci();
        return true;
    }
    if (trimmed == "isready")
    {
        writeLine("readyok");
        return true;
    }
    if (trimmed == "ucinewgame")
    {
        position_ = Position::fromStartingPosition();
        searchThread_.tryRun([](EngineController& engine, const std::atomic<bool>*) {
            engine.clearTranspositionTable();
        });
        return true;
    }
    if (trimmed == "stop")
    {
        handleStop();
        return true;
    }
    if (trimmed == "quit")
        return false;
    if (trimmed == "ponderhit")
        return true;

    if (trimmed.rfind("setoption ", 0) == 0)
    {
        handleSetOption(trimmed);
        return true;
    }
    if (trimmed.rfind("position ", 0) == 0)
    {
        handlePosition(trimmed);
        return true;
    }
    if (trimmed.rfind("go", 0) == 0 && (trimmed.size() == 2 || trimmed[2] == ' '))
    {
        handleGo(trimmed);
        return true;
    }

    if (trimmed == "fen")
    {
        printFen();
        return true;
    }
    if (trimmed == "legalmoves")
    {
        printLegalMoves();
        return true;
    }
    if (trimmed == "board")
    {
        printBoard();
        return true;
    }
    if (trimmed == "eval")
    {
        printEval();
        return true;
    }
    if (trimmed == "hash")
    {
        printHash();
        return true;
    }

    return true;
}

void UciEngine::handleUci()
{
    writeLine("id name ChessEngine " + std::to_string(ENGINE_VERSION_MAJOR) + '.' +
              std::to_string(ENGINE_VERSION_MINOR));
    writeLine("id author Noah Yared");
    writeLine("option name Threads type spin default 1 min 1 max " +
              std::to_string(maxSearchParallelism()));
    writeLine("option name Hash type spin default 16 min 1 max 4096");
    writeLine("option name Move Overhead type spin default 10 min 0 max 5000");
    writeLine("uciok");
}

void UciEngine::handleSetOption(const std::string& line)
{
    const auto namePos = line.find(" name ");
    const auto valuePos = line.find(" value ");
    if (namePos == std::string::npos || valuePos == std::string::npos || valuePos <= namePos)
        return;

    const std::string name = trim(line.substr(namePos + 6, valuePos - (namePos + 6)));
    const std::string value = trim(line.substr(valuePos + 7));

    int parsed = 0;
    if (!parseInt(value, parsed))
        return;

    if (name == "Threads")
        threads_ = clampSearchParallelism(parsed);
    else if (name == "Hash")
        hashMB_ = std::max(parsed, 1);
    else if (name == "Move Overhead")
        moveOverheadMS_ = std::max(parsed, 0);
}

void UciEngine::handlePosition(const std::string& line)
{
    std::size_t index = std::string("position ").size();
    if (line.compare(index, 8, "startpos") == 0)
    {
        position_ = Position::fromStartingPosition();
        index += 8;
    }
    else if (line.compare(index, 3, "fen") == 0)
    {
        index += 4;
        const auto movesPos = line.find(" moves ", index);
        const std::string fen =
            trim(movesPos == std::string::npos ? line.substr(index) : line.substr(index, movesPos - index));
        position_ = Position(fen);
        index = movesPos == std::string::npos ? line.size() : movesPos + 7;
    }
    else
    {
        return;
    }

    if (index < line.size() && line.compare(index, 6, " moves") == 0)
        index += 6;

    std::istringstream movesStream(index < line.size() ? line.substr(index) : std::string{});
    std::string uciMove;
    while (movesStream >> uciMove)
        position_.applyMove(uciToMove(uciMove, position_));
}

void UciEngine::handleGo(const std::string& line)
{
    const GoLimits limits = parseGoLimits(line);
    if (limits.perftDepth.has_value())
    {
        handlePerft(*limits.perftDepth);
        return;
    }

    if (!hasLegalMoves())
    {
        writeLine("bestmove (none)");
        return;
    }

    const Position searchPosition = position_;
    const Color sideToMove = position_.sideToMove();
    const SearchConfig config = buildSearchConfig(limits, sideToMove, threads_, moveOverheadMS_);

    const bool accepted = searchThread_.tryRun(
        [this, searchPosition, config, sideToMove](EngineController& engine,
                                                   const std::atomic<bool>* stopFlag)
        {
            engine.setPosition(searchPosition);
            engine.setHashSizeMB(hashMB_);

            const auto onDepth = [this, sideToMove](const Position& root, const Searcher::DepthInfo& info)
            {
                constexpr u64 msInNs = 1'000'000ULL;
                constexpr u64 nsInSec = 1'000'000'000ULL;
                const auto elapsedNs = info.elapsed.count();
                const auto timeMs = elapsedNs / msInNs;
                const u64 nps =
                    elapsedNs == 0
                        ? (info.nodesSearched * nsInSec)
                        : ((info.nodesSearched * nsInSec) /
                           static_cast<u64>(std::max(elapsedNs, 1LL)));
                std::ostringstream oss;
                oss << "info depth " << info.depth << ' ' << formatInfoScore(info.score, sideToMove)
                    << " nodes " << info.nodesSearched << " time " << timeMs << " nps " << nps
                    << " pv " << joinPv(info.pv);
                writeLine(oss.str());
            };

            const SearchResult result = engine.search(config, stopFlag, onDepth);
            writeLine("bestmove " + result.bestMove.uci());
        });

    (void)accepted;
}

void UciEngine::handleStop() { searchThread_.cancel(); }

void UciEngine::handlePerft(int depth)
{
    if (depth <= 0)
        return;

    const Position searchPosition = position_;
    searchThread_.tryRun([this, searchPosition, depth](EngineController&, const std::atomic<bool>*)
                         {
                             Position pos = searchPosition;
                             const auto start = std::chrono::high_resolution_clock::now();
                             const u64 nodes = pos.isWhiteToMove()
                                                   ? perft<Color::WHITE>(pos, depth)
                                                   : perft<Color::BLACK>(pos, depth);
                             const auto end = std::chrono::high_resolution_clock::now();
                             const auto duration =
                                 std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
                                     .count();
                             const auto durationForNps = duration == 0 ? 1 : duration;

                             std::ostringstream oss;
                             oss << "Perft(" << depth << ") complete!\n\n"
                                 << "Time taken:       " << std::setw(12) << duration << " ms\n"
                                 << "Nodes generated:  " << std::setw(12) << nodes << " nodes\n"
                                 << "Nodes per second: " << std::setw(12)
                                 << nodes * 1000 / durationForNps << " nps\n";
                             writeLine(oss.str());
                         });
}

bool UciEngine::hasLegalMoves() const
{
    MoveList moves{};
    position_.isWhiteToMove() ? MoveGenerator::pushLegalMoves<Color::WHITE>(position_, moves)
                              : MoveGenerator::pushLegalMoves<Color::BLACK>(position_, moves);
    return moves.size() > 0;
}

void UciEngine::printFen() { writeLine(position_.toFen()); }

void UciEngine::printLegalMoves()
{
    MoveList moves{};
    position_.isWhiteToMove() ? MoveGenerator::pushLegalMoves<Color::WHITE>(position_, moves)
                              : MoveGenerator::pushLegalMoves<Color::BLACK>(position_, moves);

    std::vector<std::string> ucis;
    ucis.reserve(moves.size());
    for (const auto& move : moves)
        ucis.push_back(move.uci());
    std::sort(ucis.begin(), ucis.end());

    std::ostringstream oss;
    for (std::size_t i = 0; i < ucis.size(); ++i)
    {
        if (i > 0)
            oss << ' ';
        oss << ucis[i];
    }
    writeLine(oss.str());
}

void UciEngine::printBoard()
{
    std::ostringstream oss;
    oss << position_;
    writeLine(oss.str());
}

void UciEngine::printEval()
{
    const int score = position_.evaluation();
    const int stmScore = position_.isWhiteToMove() ? score : -score;
    writeLine(std::to_string(stmScore));
}

void UciEngine::printHash()
{
    std::ostringstream oss;
    oss << std::hex << position_.getHash() << std::dec;
    writeLine(oss.str());
}
