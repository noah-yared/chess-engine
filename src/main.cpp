#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <sstream>
#include <system_error>

#include "constants.h"
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
            << "   " << pathToExe << " [--help|-h]\n"
            << "   " << pathToExe << " --perft --depth|-d <depth> [--fen|-f <fen>]\n"
            << "   " << pathToExe << " --simulate [--num-moves|-n <num_moves>] [--depth|-d <depth>] [--fen|-f <fen>] [--output|-o <outfile>]\n"
            << "   " << pathToExe << " --legal-moves <fen>\n"
            << "   " << pathToExe << " --find-best <fen> [--depth|-d <depth>]\n"
            << "   " << pathToExe << " --make-move <fen> <uci>\n"
            << "   " << pathToExe << " --make-move --fen|-f <fen> --move|-m <uci>\n\n"
            << "Flags:\n"
            << "   --help|-h: print this help message\n"
            << "   --perft: conduct a perft test by generating all possible moves up to a specified depth starting from an optionally specified fen\n"
            << "   --simulate: simulate self-play with optionally specified number of moves, search depth, starting fen, and output file (where positions are dumped)\n"
            << "   --legal-moves: print legal moves in uci format for a given fen\n"
            << "   --find-best: find the best move for a given fen and output in uci format with optionally specified search depth\n"
            << "   --make-move: make move in uci format for a given fen and print the new fen\n\n";
}

u64 perft(Position& pos, int depth) {
  if (depth == 1) return pos.legalMoves().size();
  u64 generatedNodes = 0ULL;
  for (auto move : pos.legalMoves()) {
    auto tmp = pos;
    std::visit([&, move](auto&& arg) { tmp.applyMove(arg); }, move);
    generatedNodes += perft(tmp, depth-1);
  }
  return generatedNodes;
}

struct PerftArgs { int depth; std::optional<std::string> fen; };
PerftArgs parsePerftArgs(int argc, const char** argv) {
  PerftArgs args{};
  for (int i = 2; i < argc; i+=2) {
    if (argv[i] == std::string("-d") || argv[i] == std::string("--depth")) {
      for (char c : std::string(argv[i + 1]))
        if (!isdigit(c)) {
          std::cout << "Invalid depth (must be nonzero): " << argv[i + 1] << '\n';
          exit(-1);
        }
      args.depth = std::stoi(argv[i + 1]);
    } else if (argv[i] == std::string("-f") || argv[i] == std::string("--fen")) {
      args.fen = argv[i + 1];
    }
  }
  return args;
}

void printPerftResults(const PerftArgs& args) {
  if (!args.depth) {
    std::cout << "Invalid depth passed in (must be nonzero)";
  }
  int depth = args.depth;
  std::string fen = args.fen.value_or(std::string(STARTING_FEN));
  Position startingPos(fen);

  auto start = std::chrono::high_resolution_clock::now();
  u64 nodes = perft(startingPos, depth);
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  std::cout << "Perft(" << depth << ") complete!\n\n"
            << "Time taken:       " << std::setw(12) << duration << " ms\n"
            << "Nodes generated:  " << std::setw(12) << nodes << " nodes\n"
            << "Nodes per second: " << std::setw(12) << nodes * 1000 / duration << " nps\n\n";
}

struct SelfPlayArgs { std::optional<int> numMoves, searchDepth;
                      std::optional<std::string> fen, outputFile; };
SelfPlayArgs parseSelfPlayArgs(int argc, const char* argv[]) {
  SelfPlayArgs args{};
  if (argc == 2) return args;
  for (int i = 2; i+1 < argc; i+=2) {
    if (std::string(argv[i]) == "-n" || std::string(argv[i]) == "--num-moves") {
      for (char c : std::string(argv[i + 1]))
        if (!isdigit(c)) {
          std::cout << "Invalid number of moves: " << argv[i + 1] << '\n';
          exit(-1);
        }
      args.numMoves = std::stoi(argv[i + 1]);
    } else if (std::string(argv[i]) == "-d" || std::string(argv[i]) == "--depth") {
      for (char c : std::string(argv[i + 1]))
        if (!isdigit(c)) {
          std::cout << "Invalid search depth: " << argv[i + 1] << '\n';
          exit(-1);
        }
      args.searchDepth = std::stoi(argv[i + 1]);
    } else if (std::string(argv[i]) == "-f" || std::string(argv[i]) == "--fen") {
      args.fen = std::string(argv[i + 1]);
    } else if (std::string(argv[i]) == "-o" || std::string(argv[i]) == "--output") {
      args.outputFile = std::string(argv[i + 1]);
    } else {
      std::cout << "Invalid flag: " << argv[i] << '\n';
      exit(-1);
    }
  }
  return args;
}

void simulateSelfPlay(const SelfPlayArgs& args, const char* exePath) {
  int numMoves = args.numMoves.value_or(100), searchDepth = args.searchDepth.value_or(6);
  std::string fen = args.fen.value_or(std::string(STARTING_FEN)); // default fen if none provided

  // create default output file name
  const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::stringstream ss;
  ss << "engine_output_" << std::put_time(std::localtime(&time), "%Y-%m-%d_%H-%M-%S") << ".txt";
  std::string outputFile = args.outputFile.value_or(ss.str());

  // create output directory if it doesnt exist and create output file stream
  auto outputDirectory = std::filesystem::canonical(exePath).parent_path().parent_path() / "output";
  std::error_code ec;
  std::filesystem::create_directory(outputDirectory, ec);
  bool outputToFile = !ec;
  if (ec)
    std::cerr << "Error creating output directory: " << ec.message() << '\n'
              << "Outputting to console instead.\n\n";

  std::ofstream ofs((outputDirectory / outputFile).string());
  std::cout << "Simulating self-play with " << numMoves << " moves at depth " << searchDepth << ".\n";
  std::cout << "Dumping positions to "
            << (outputToFile ? (outputDirectory / outputFile).string() : std::string("console")) << ".\n\n";

  auto& os = outputToFile ? ofs : std::cout;
  auto start = std::chrono::high_resolution_clock::now();
  SearchEngine engine(fen);
  for (int i = 0; i < numMoves; ++i) {
    engine.search(searchDepth);
    engine.dumpPosition(os);
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  std::cout << "Simulation complete!\n\n"
            << "Time taken:       " << std::setw(12) << static_cast<double>(duration) / 1000.0 << " secs\n"
            << "Nodes searched:   " << std::setw(12) << engine.getNodesSearchedCount() << " nodes\n"
            << "Nodes per second: " << std::setw(12) << engine.getNodesSearchedCount() * 1000 / duration << " nps\n\n";
}

void printLegalMoves(const std::string& fen) {
  Position pos(fen);
  std::cout << pos.legalMoves() << '\n';
}

void printBestMove(int argc, const char** argv) {
  SearchEngine engine(argv[1]);
  if (argc == 5 && (std::string(argv[3]) == "-d" || std::string(argv[3]) == "--depth")) {
    for (char c : std::string(argv[4]))
      if (!isdigit(c)) {
        std::cout << "Invalid depth: " << argv[4] << '\n';
        exit(-1);
      }
    std::cout << std::visit([](auto&& arg){ return arg.uci(); }, engine.search(std::stoi(argv[4]))) << '\n';
  } else if (argc == 3) {
    std::cout << std::visit([](auto&& arg){ return arg.uci(); }, engine.search()) << '\n';
  } else {
    std::cout << "Invalid flag for find-best\n";
    exit(-1);
  }
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
      std::cout << "Invalid flag: " << argv[i] << '\n';
      exit(-1);
    }
  }
  return args;
}

void printNewFen(const MakeMoveArgs& args) {
  SearchEngine engine(args.oldFen);
  engine.advance(uciToMove(args.uciMove, engine.getPosition()));
  std::cout << engine.getPosition().toFen() << '\n';
}

int main(int argc, const char* argv[]) {
  // print engine info and usage
  if (argc == 1) {
    printEngineInfo();
    printUsage(argv[0]);
    return 0;
  }

  if ((argc == 4 || argc == 6) && (std::string(argv[1]) == "--perft")) {
    printPerftResults(parsePerftArgs(argc, argv));
    return 0;
  }

  // help flag
  if (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
    printEngineInfo();
    printUsage(argv[0]);
    return 0;
  }

  // simulate self-play with variable number of moves and variable search depth from default position
  if (argc <= 10 && argc % 2 == 0 && std::string(argv[1]) == "--simulate") {
    simulateSelfPlay(parseSelfPlayArgs(argc, argv), argv[0]); // defaults to simulation of 100 moves at depth 6
    return 0;
  }

  // generate legal moves from fen
  if (argc == 3 && std::string(argv[1]) == "--legal-moves") {
    printLegalMoves(argv[2]);
    return 0;
  }

  // calculate best move from fen
  if ((argc == 3 || argc == 5) && std::string(argv[1]) == "--find-best") {
    printBestMove(argc, argv);
    return 0;
  }

  // compute new fen from uci move applied to fen
  if (std::string(argv[1]) == "--make-move") {
    if (argc == 6) { // with flags: --make-move --fen <fen> --move <move>
      printNewFen(parseMakeMoveArgs(argc, argv));
      return 0;
    } else if (argc == 4) { // without flags: --make-move <fen> <move>
      printNewFen({ argv[2], argv[3] });
      return 0;
    }
  }

  // bad arguments
  std::cout << "Invalid arguments passed in!\n\n";
  printUsage(argv[0]);
  return -1;
}
