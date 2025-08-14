#pragma once

#include "engine.h"
#include "move_generator.h"
#include "move_utils.h"
#include "position.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <variant>

namespace cli
{

// Engine info and usage functions
void printEngineInfo();
void printUsage(const char* pathToExe);

// Perft functionality
struct PerftArgs
{
    int depth;
    std::optional<std::string> fen;
};

// Template function implementation
template <Color color>
u64 perft(Position& pos, int depth)
{
    MoveList moves{};
    MoveGenerator::pushLegalMoves<color>(pos, moves);
    if (depth == 1)
        return moves.size();
    u64 nodeCount = 0ULL;
    for (auto& move : moves)
    {
        auto snapshot = pos.getStateSnapshot();
        std::visit([&](auto&& arg) { pos.applyMove(arg); }, move);
        nodeCount += perft<opposite<color>()>(pos, depth - 1);
        std::visit([&](auto&& arg) { pos.undoMove(arg, snapshot); }, move);
    }
    return nodeCount;
}

PerftArgs parsePerftArgs(int argc, const char** argv);
void printPerftResults(const PerftArgs& args, std::ofstream& ofs);

// Self-play simulation functionality
struct SelfPlayArgs
{
    std::optional<int> numMoves, searchDepth;
    std::optional<std::string> fen, outputFile;
};

SelfPlayArgs parseSelfPlayArgs(int argc, const char** argv);
void simulateSelfPlay(const SelfPlayArgs& args, const char* exePath);

// Legal moves functionality
void printLegalMoves(const std::string& fen);

// Best move functionality
void printBestMove(const std::string& fen, int depth = 5);

// Make move functionality
struct MakeMoveArgs
{
    std::string oldFen, uciMove;
};

MakeMoveArgs parseMakeMoveArgs(int argc, const char* argv[]);
void printNewFen(const MakeMoveArgs& args);

// Main CLI entry point
int runCli(int argc, const char* argv[]);

// Command handlers
int handlePerftCommand(int argc, const char* argv[]);
int handleHelpCommand(int argc, const char* argv[]);
int handleSimulateCommand(int argc, const char* argv[]);
int handleLegalMovesCommand(int argc, const char* argv[]);
int handleFindBestCommand(int argc, const char* argv[]);
int handleMakeMoveCommand(int argc, const char* argv[]);

} // namespace cli
