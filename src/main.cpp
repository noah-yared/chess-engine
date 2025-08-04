#include <chrono>
#include <iomanip>
#include <iostream>
#include <optional>

#include "engine.h"
#include "engine_config.h"
#include "move_utils.h"
#include "position.h"

void printEngineInfo() {
  std::cout << '\n'
            << "Engine version: " << ENGINE_VERSION_MAJOR << '.' << ENGINE_VERSION_MINOR << '\n'
            << "Engine built: " << __DATE__ << " " << __TIME__ << '\n'
            << "Engine author: " << "Noah Yared" << "\n\n";
}

void printUsage(const char* pathToExe) {
  std::cout << "Usage:\n"
            << "   " << pathToExe << " --simulate|-sim [--num-moves|-n <num_moves>] [--search-depth|-d <search_depth>]\n"
            << "   " << pathToExe << " --legal-moves|-lm <fen>\n"
            << "   " << pathToExe << " --find-best|-fb <fen>\n"
            << "   " << pathToExe << " --make-move|-mm --fen|-f <fen> --move|-m <uci_move>\n\n";
}

struct MakeMoveArgs { std::string oldFen, uciMove; };
MakeMoveArgs parseMakeMoveArgs(int argc, const char* argv[]) {
  MakeMoveArgs args{};
  for (int i = 2; i+1 < argc; i+=2) {
    if (std::string(argv[i]) == "-f" || std::string(argv[i]) == "--fen") {
      args.oldFen = std::string(argv[i + 1]);
    } else if (std::string(argv[i]) == "-m" || std::string(argv[i]) == "--move") {
      args.uciMove = std::string(argv[i + 1]);
    } else {
      std::cout << "Invalid argument: " << argv[i] << '\n';
      exit(-1);
    }
  }
  return args;
}

struct SelfPlayArgs { std::optional<int> numMoves, searchDepth; };
SelfPlayArgs parseSelfPlayArgs(int argc, const char* argv[]) {
  SelfPlayArgs args{};
  if (argc == 2)
    return args;
  for (int i = 2; i+1 < argc; i+=2) {
    if (std::string(argv[i]) == "-n" || std::string(argv[i]) == "--num-moves") {
      for (char c : std::string(argv[i + 1]))
        if (!isdigit(c)) {
          std::cout << "Invalid number of moves: " << argv[i + 1] << '\n';
          exit(-1);
        }
      args.numMoves = std::stoi(argv[i + 1]);
    } else if (std::string(argv[i]) == "-d" || std::string(argv[i]) == "--search-depth") {
      for (char c : std::string(argv[i + 1]))
        if (!isdigit(c)) {
          std::cout << "Invalid search depth: " << argv[i + 1] << '\n';
          exit(-1);
        }
      args.searchDepth = std::stoi(argv[i + 1]);
    } else {
      std::cout << "Invalid argument: " << argv[i] << '\n';
      exit(-1);
    }
  }
  return args;
}

void simulateSelfPlay(SelfPlayArgs args) {
  int numMoves = args.numMoves.value_or(100), searchDepth = args.searchDepth.value_or(6);
  std::cout << "Simulating self-play with " << numMoves << " moves at depth " << searchDepth << "...\n\n";
  auto start = std::chrono::high_resolution_clock::now();
  SearchEngine engine;
  for (int i = 0; i < numMoves; ++i) {
    engine.search(searchDepth);
    engine.dumpPosition();
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  std::cout << "Simulation complete!\n\n"
            << "Time taken:       " << std::setw(12) << static_cast<double>(duration) / 1000.0 << " secs\n"
            << "Nodes searched:   " << std::setw(12) << engine.getNodesSearchedCount() << " nodes\n"
            << "Nodes per second: " << std::setw(12) << engine.getNodesSearchedCount() * 1000 / duration << " nps\n\n";
}

int main(int argc, const char* argv[]) {
  // print engine info and usage
  if (argc == 1) {
    printEngineInfo();
    printUsage(argv[0]);
    return 0;
  }

  // simulate self-play with variable number of moves and variable search depth from default position
  if (argc <= 6 && (std::string(argv[1]) == "--simulate" || std::string(argv[1]) == "-sim")) {
    simulateSelfPlay(parseSelfPlayArgs(argc, argv)); // defaults to simulation of 100 moves at depth 6
    return 0;
  }

  // generate legal moves from fen
  if (argc == 3 && (std::string(argv[1]) == "--legal-moves" || std::string(argv[1]) == "-lm")) {
    std::string fen = argv[2];
    Position pos(fen);
    std::cout << pos.legalMoves() << '\n';
    return 0;
  }

  // calculate best move from fen
  if (argc == 3 && (std::string(argv[1]) == "--find-best" || std::string(argv[1]) == "-fb")) {
    SearchEngine engine(argv[2]);
    std::cout << std::visit([](auto&& arg){ return arg.uci(); }, engine.search()) << '\n';
    return 0;
  }

  // compute update fen from uci move applied to fen
  if (argc == 6 && (std::string(argv[1]) == "--make-move" || std::string(argv[1]) == "-mm")) {
    auto [oldFen, uciMove] = parseMakeMoveArgs(argc, argv);
    SearchEngine engine(oldFen);
    engine.advance(uciToMove(uciMove, engine.getPosition()));
    std::cout << engine.getPosition().toFen() << '\n';
    return 0;
  }

  // bad arguments
  std::cout << "Invalid arguments passed in!\n\n";
  printUsage(argv[0]);
  return -1;
}
