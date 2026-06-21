#include "app/cli.h"
#include "board/constants.h"
#include "engine_config.h"
#include "move/move_generator.h"
#include "move/uci.h"
#include "search/strength.h"
#include <array>
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

// helper function
std::string trimWhitespace(const std::string& str)
{
    auto start = std::find_if(str.begin(), str.end(), [](char c) { return !iswspace(c); });
    auto end = std::find_if(str.rbegin(), str.rend(), [](char c) { return !iswspace(c); }).base();
    return {start, end};
}

std::vector<std::string> split(const std::string& str, char delimiter = ' ')
{
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;
    while (std::getline(iss, token, delimiter))
        tokens.push_back(token);
    return tokens;
}

namespace cli
{

void printEngineInfo()
{
    std::cout << '\n'
              << "Engine version: " << ENGINE_VERSION_MAJOR << '.' << ENGINE_VERSION_MINOR << '\n'
              << "Engine built: " << __DATE__ << " " << __TIME__ << '\n'
              << "Engine author: " << "Noah Yared" << "\n\n";
}

void printUsage(const char* pathToExe)
{
    std::cout
        << "Usage:\n"
        << "   " << pathToExe << " [--help|-h]\n"
        << "   " << pathToExe << " --perft --depth|-d <depth> [--fen|-f <fen>]\n"
        << "   " << pathToExe
        << " --simulate [--num-moves|-n <num_moves>] [--depth|-d <depth>] [--fen|-f <fen>] "
           "[--output|-o <outfile>]\n"
        << "   " << pathToExe << " --legal-moves [<fen>]\n"
        << "   " << pathToExe << " --find-best [<fen> [--depth|-d <depth>]]\n"
        << "   " << pathToExe << " --make-move [[--fen|-f] <fen>] [[--move|-m] <uci>]\n"
        << "   " << pathToExe << " --king-in-check <fen>\n\n"
        << "Flags:\n"
        << "   --help|-h: print this help message\n"
        << "   --perft: conduct a perft test by generating all possible moves up to a specified "
           "depth starting from an optionally specified fen\n"
        << "   --simulate: simulate self-play with optionally specified number of moves, search "
           "depth, starting fen, and output file (where positions are dumped)\n"
        << "   --legal-moves: print legal moves in uci format for a given fen, reads from stdin "
           "if no flags are provided\n"
        << "   --find-best: find the best move for a given fen and output in uci format with "
           "optionally specified search depth\n"
        << "   --make-move: make move in uci format for a given fen and print the new fen, reads "
           "from stdin if no flags are provided\n"
        << "   --king-in-check: print whether the king (current side to move) is in check for a "
           "given fen\n\n";
}

PerftArgs parsePerftArgs(int argc, const char** argv)
{
    PerftArgs args{};
    for (int i = 2; i < argc; i += 2)
    {
        if (argv[i] == std::string("-d") || argv[i] == std::string("--depth"))
        {
            for (char c : std::string(argv[i + 1]))
                if (!isdigit(c))
                {
                    std::cout << "Invalid depth (must be nonzero): " << argv[i + 1] << '\n';
                    exit(-1);
                }
            args.depth = std::stoi(argv[i + 1]);
        }
        else if (argv[i] == std::string("-f") || argv[i] == std::string("--fen"))
        {
            args.fen = argv[i + 1];
        }
    }
    return args;
}

void printPerftResults(const PerftArgs& args, std::ofstream& ofs)
{
    if (!args.depth)
    {
        std::cout << "Invalid depth passed in (must be nonzero)\n";
        exit(-1);
    }
    Position pos(args.fen.value_or(std::string(STARTING_FEN)));
    auto start = std::chrono::high_resolution_clock::now();
    auto nodeCount = pos.isWhiteToMove() ? perft<Color::WHITE>(pos, args.depth)
                                         : perft<Color::BLACK>(pos, args.depth);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    auto durationForNps = duration == 0 ? 1 : duration;
    std::cout << "Perft(" << args.depth << ") complete!\n\n"
              << "Time taken:       " << std::setw(12) << duration << " ms\n"
              << "Nodes generated:  " << std::setw(12) << nodeCount << " nodes\n"
              << "Nodes per second: " << std::setw(12) << nodeCount * 1000 / durationForNps
              << " nps\n\n";
}

SelfPlayArgs parseSelfPlayArgs(int argc, const char** argv)
{
    SelfPlayArgs args{};
    if (argc == 2)
        return args;
    for (int i = 2; i + 1 < argc; i += 2)
    {
        if (std::string(argv[i]) == "-n" || std::string(argv[i]) == "--num-moves")
        {
            args.numMoves = std::stoi(argv[i + 1]);
        }
        else if (std::string(argv[i]) == "-d" || std::string(argv[i]) == "--depth")
        {
            args.searchDepth = std::stoi(argv[i + 1]);
        }
        else if (std::string(argv[i]) == "-f" || std::string(argv[i]) == "--fen")
        {
            args.fen = std::string(argv[i + 1]);
        }
        else if (std::string(argv[i]) == "-o" || std::string(argv[i]) == "--output")
        {
            args.outputFile = std::string(argv[i + 1]);
        }
        else
        {
            std::cout << "Invalid flag: " << argv[i] << '\n';
            exit(-1);
        }
    }
    return args;
}

void simulateSelfPlay(const SelfPlayArgs& args, const char* exePath)
{
    // aggregate arguments
    int numMoves = args.numMoves.value_or(100), searchDepth = args.searchDepth.value_or(6);
    std::string fen = args.fen.value_or(std::string(STARTING_FEN)); // default fen if none provided

    // create search engine
    EngineController engine(fen);
    u64 nodesSearched = 0ULL;

    // create default output file name
    const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::stringstream ss;
    ss << "engine_output_" << std::put_time(std::localtime(&time), "%Y-%m-%d_%H-%M-%S") << ".txt";
    std::string outputFile = args.outputFile.value_or(ss.str());

    // create output directory if it doesnt exist and create output file stream
    auto outputDirectory =
        std::filesystem::canonical(exePath).parent_path().parent_path() / "output";
    std::error_code ec;
    std::filesystem::create_directory(outputDirectory, ec);
    bool outputToFile = !ec;
    if (ec)
        std::cerr << "Error creating output directory: " << ec.message() << '\n'
                  << "Outputting to console instead.\n\n";
    std::cout << "Simulating self-play with " << numMoves << " moves at depth " << searchDepth
              << ".\n";
    std::cout << "Dumping positions to "
              << (outputToFile ? (outputDirectory / outputFile).string() : std::string("console"))
              << ".\n\n";

    // create output file stream
    std::ofstream ofs;
    if (outputToFile)
    {
        ofs.open((outputDirectory / outputFile).string());
        if (!ofs)
        {
            std::cerr << "Error opening output file. Outputting to console instead.\n\n";
            outputToFile = false;
        }
    }
    auto& os = outputToFile ? static_cast<std::ostream&>(ofs) : std::cout;

    // start self-play
    auto start = std::chrono::high_resolution_clock::now();
    for (int moveNumber = 1; moveNumber <= numMoves; ++moveNumber)
    {
        auto result = engine.search(searchDepth);
        auto move = result.bestMove;
        nodesSearched += result.stats.nodesSearched;
        engine.advance(move);
        os << "Move #" << moveNumber << ": "
           << std::visit([](auto&& arg) { return arg.uci(); }, move) << '\n';
        os << engine.position() << '\n';
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    auto durationForNps = duration == 0 ? 1 : duration;

    // print results to console
    std::cout << "Simulation complete!\n\n"
              << "Time taken:       " << std::setw(12) << static_cast<double>(duration) / 1000.0
              << " secs\n"
              << "Nodes searched:   " << std::setw(12) << nodesSearched << " nodes\n"
              << "Nodes per second: " << std::setw(12) << nodesSearched * 1000 / durationForNps
              << " nps\n\n";
}

void printLegalMoves(const Position& pos)
{
    MoveList moves{};
    pos.isWhiteToMove() ? MoveGenerator::pushLegalMoves<Color::WHITE>(pos, moves)
                        : MoveGenerator::pushLegalMoves<Color::BLACK>(pos, moves);
    std::cout << moves;
}

void printLegalMoves(const std::string& fen) { printLegalMoves(Position(fen)); }

void printBestMove(const std::string& fen, int depth)
{
    EngineController engine(fen);
    auto result = engine.search(depth);
    std::cout << std::visit([](auto&& arg) { return arg.uci(); }, result.bestMove) << '\n';
}

MakeMoveArgs parseMakeMoveArgs(int argc, const char* argv[])
{
    MakeMoveArgs args{};
    for (int i = 2; i + 1 < argc; i += 2)
    {
        if (std::string(argv[i]) == "-f" || std::string(argv[i]) == "--fen")
        {
            args.oldFen = std::string(argv[i + 1]);
        }
        else if (std::string(argv[i]) == "-m" || std::string(argv[i]) == "--move")
        {
            args.uciMove = std::string(argv[i + 1]);
        }
        else
        {
            std::cout << "Invalid flag: " << argv[i] << '\n';
            exit(-1);
        }
    }
    return args;
}

void printNewFen(const MakeMoveArgs& args)
{
    EngineController engine(args.oldFen);
    engine.advance(uciToMove(args.uciMove, engine.position()));
    std::cout << engine.position().toFen() << '\n';
}

void printIsKingInCheck(const std::string& fen)
{
    Position pos(fen);
    std::cout << std::boolalpha
              << (pos.isWhiteToMove() ? MoveGenerator::isKingInCheck<Color::WHITE>(pos)
                                      : MoveGenerator::isKingInCheck<Color::BLACK>(pos))
              << '\n';
}

int handlePerftCommand(int argc, const char* argv[])
{
    if (argc != 4 && argc != 6)
        return -1;

    std::ofstream ofs("fens.txt");
    printPerftResults(parsePerftArgs(argc, argv), ofs);
    return 0;
}

int handleHelpCommand(int argc, const char* argv[])
{
    if (argc != 2)
        return -1;

    printEngineInfo();
    printUsage(argv[0]);
    return 0;
}

int handleSimulateCommand(int argc, const char* argv[])
{
    if (argc > 10 || argc % 2 != 0)
        return -1;

    simulateSelfPlay(parseSelfPlayArgs(argc, argv), argv[0]);
    return 0;
}

int handleLegalMovesCommand(int argc, const char* argv[])
{
    if (argc == 3)
    {
        printLegalMoves(argv[2]);
        return 0;
    }
    else if (argc == 2)
    {
        // read from stdin: <fen>\n...
        std::string line;
        while (std::getline(std::cin, line))
        {
            auto trimmedLine = trimWhitespace(line);
            if (trimmedLine.empty())
                continue;
            printLegalMoves(trimmedLine);
        }
        return 0;
    }
    return -1;
}

int handleFindBestCommand(int argc, const char* argv[])
{
    if (argc == 5 && (std::string(argv[3]) == "-d" || std::string(argv[3]) == "--depth"))
    {
        printBestMove(argv[2], std::stoi(argv[4]));
        return 0;
    }
    else if (argc == 3)
    {
        printBestMove(argv[2]);
        return 0;
    }
    else if (argc == 2)
    {
        // read from stdin: <fen>[,<depth>]\n
        std::string line;
        while (std::getline(std::cin, line))
        {
            auto trimmedLine = trimWhitespace(line);
            if (trimmedLine.empty())
                continue;
            auto firstComma = trimmedLine.find_first_of(',');
            if (firstComma != std::string::npos) // contains depth
            {
                printBestMove(trimmedLine.substr(0, firstComma),
                              std::stoi(trimmedLine.substr(firstComma + 1)));
            }
            else // no depth
            {
                printBestMove(trimmedLine);
            }
        }
        return 0;
    }
    return -1;
}

int handleMakeMoveCommand(int argc, const char* argv[])
{
    if (argc == 6)
    {
        // with flags: --make-move --fen <fen> --move <move>
        printNewFen(parseMakeMoveArgs(argc, argv));
        return 0;
    }
    else if (argc == 4)
    {
        // without flags: --make-move <fen> <move>
        printNewFen({argv[2], argv[3]});
        return 0;
    }
    else if (argc == 2)
    {
        // read from stdin: [start_fen],[move_uci],...\n
        std::string line;
        while (std::getline(std::cin, line))
        {
            auto trimmedLine = trimWhitespace(line);
            if (trimmedLine.empty())
                continue;
            auto firstComma = trimmedLine.find_first_of(','),
                 lastComma = trimmedLine.find_last_of(',');
            if (firstComma != std::string::npos && lastComma != std::string::npos)
            {
                printNewFen({trimmedLine.substr(0, firstComma),
                             trimmedLine.substr(firstComma + 1, lastComma - firstComma - 1)});
            }
            else
            {
                std::cout << "Invalid line: " << line << '\n';
                return -1;
            }
        }
        return 0;
    }
    return -1;
}

int handleKingInCheckCommand(int argc, const char* argv[])
{
    if (argc != 3)
        return -1;

    printIsKingInCheck(argv[2]);
    return 0;
}

int handleAppModeCommand(int argc, const char* argv[])
{
    if (argc != 2)
        return -1;

    EngineController engine;

    std::string line;
    while (std::getline(std::cin, line))
    {
        auto tokens = split(line, ',');
        if (tokens.empty())
            continue;
        auto command = tokens[0];
        if (command == "make-move")
        { // make-move,<move>[,<fen>]
            if (tokens.size() == 3)
            {
                printNewFen({.oldFen = tokens[2], .uciMove = tokens[1]});
                std::cout << std::endl;
            }
            else if (tokens.size() == 2)
            {
                engine.advance(uciToMove(tokens[1], engine.position()));
                std::cout << engine.position().toFen() << '\n' << std::endl;
            }
            else
            {
                std::cout << "Invalid command: " << line << '\n';
                return -1;
            }
        }
        else if (command == "legal-moves")
        { // legal-moves[,<fen>]
            if (tokens.size() == 2)
            {
                printLegalMoves(tokens[1]);
                std::cout << std::endl;
            }
            else if (tokens.size() == 1)
            {
                printLegalMoves(engine.position());
                std::cout << std::endl;
            }
            else
            {
                std::cout << "Invalid command: " << line << '\n';
                return -1;
            }
        }
        else if (command == "find-best")
        { // find-best,<strength>
            if (tokens.size() != 2)
            {
                std::cout << "Invalid command: " << line << '\n';
                return -1;
            }

            int strengthLevel = std::stoi(tokens[1]);
            if (strengthLevel < 0 || strengthLevel >= static_cast<int>(Strength::NUM_LEVELS))
            {
                std::cout << "Invalid strength level: " << tokens[1] << '\n';
                return -1;
            }

            auto move = engine.playEngineMove(Strength(strengthLevel));
            std::cout << std::visit([](auto&& arg) { return arg.uci(); }, move) << '\n'
                      << std::endl;
        }
        else if (command == "king-in-check")
        { // king-in-check[,<fen>]
            if (tokens.size() == 2)
            {
                printIsKingInCheck(tokens[1]);
                std::cout << std::endl;
            }
            else if (tokens.size() == 1)
            {
                std::cout << std::boolalpha
                          << (engine.turn() == Color::WHITE
                                  ? MoveGenerator::isKingInCheck<Color::WHITE>(engine.position())
                                  : MoveGenerator::isKingInCheck<Color::BLACK>(engine.position()))
                          << '\n'
                          << std::endl;
            }
            else
            {
                std::cout << "Invalid command: " << line << '\n';
                return -1;
            }
        }
        else if (command == "current-fen")
        { // current-fen
            if (tokens.size() != 1)
            {
                std::cout << "Invalid command: " << line << '\n';
                return -1;
            }
            std::cout << engine.position().toFen() << '\n' << std::endl;
        }
        else if (command == "set-position")
        { // set-position,<fen>
            if (tokens.size() != 2)
            {
                std::cout << "Invalid command: " << line << '\n';
                return -1;
            }
            engine.setPosition(tokens[1]);
        }
        else
        {
            std::cout << "Invalid command: " << command << '\n';
            return -1;
        }
    }
    return 0;
}

int runCli(int argc, const char* argv[])
{
    // Handle no arguments case
    if (argc == 1)
    {
        printEngineInfo();
        printUsage(argv[0]);
        return 0;
    }

    // Get the command
    std::string command = argv[1];

    // Route to appropriate command handler
    if (command == "--perft")
        return handlePerftCommand(argc, argv);
    else if (command == "--help" || command == "-h")
        return handleHelpCommand(argc, argv);
    else if (command == "--simulate")
        return handleSimulateCommand(argc, argv);
    else if (command == "--legal-moves")
        return handleLegalMovesCommand(argc, argv);
    else if (command == "--find-best")
        return handleFindBestCommand(argc, argv);
    else if (command == "--make-move")
        return handleMakeMoveCommand(argc, argv);
    else if (command == "--king-in-check")
        return handleKingInCheckCommand(argc, argv);
    else if (command == "--app-mode")
        return handleAppModeCommand(argc, argv);

    // Unknown command
    std::cout << "Invalid arguments passed in!\n\n";
    printUsage(argv[0]);
    return -1;
}

} // namespace cli
