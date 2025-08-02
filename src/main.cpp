#include <bitset>
#include <cassert>
#include <chrono>
#include <iostream>

#include "engine.h"
#include "engine_config.h"
#include "move_utils.h"
#include "position.h"

void debug(Position& pos) {
  // implement if needed
}

int main(int argc, char* argv[]) {
  std::cout << '\n'
            << "Engine version: " << ENGINE_VERSION_MAJOR << '.' << ENGINE_VERSION_MINOR << '\n'
            << "Engine built: " << __DATE__ << " " << __TIME__ << '\n'
            << "Engine author: " << "Noah Yared" << "\n\n";
  // print usage
  if (argc == 1) {
    std::cout << "Usage:\n"
              << "   " << argv[0] << " --legal-moves | -lm <fen>\n"
              << "   " << argv[0] << " --find-best | -fb <fen>\n"
              << "   " << argv[0] << " --make-move | -mm <fen> <uci_move>\n";
    return 0;
  }
  // move generation -- written to stdout
  if (argc == 3 && (std::string(argv[1]) == "--legal-moves" || std::string(argv[1]) == "-lm")) {
    std::string fen = argv[2];
    Position pos(fen);
    std::cout << pos.legalMoves() << '\n';
    return 0;
  }
  // calculate best move
  if (argc == 3 && (std::string(argv[1]) == "--find-best" || std::string(argv[1]) == "-fb")) {
    std::string fen = argv[2];
    SearchEngine engine(fen);
    auto move = engine.search();
    std::cout << std::visit([](auto&& arg){ return arg.uci(); }, move) << '\n';
    return 0;
  }
  // update fen for uci move
  if (argc == 4 && (std::string(argv[1]) == "--make-move" || std::string(argv[1]) == "-mm")) {
    std::string oldFen = argv[2], uciMove = argv[3];
    SearchEngine engine(oldFen);
    engine.advance(uciToMove(uciMove, engine.getPosition()));
    std::cout << engine.getPosition().toFen() << '\n';
    return 0;
  }

  std::cout << "Invalid arguments passed in!\n\n";
  std::cout << "Usage:\n"
            << "   " << argv[0] << " --legal-moves | -lm <fen>\n"
            << "   " << argv[0] << " --find-best | -fb <fen>\n"
            << "   " << argv[0] << " --make-move | -mm <fen> <uci_move>\n";

  return -1;
}
