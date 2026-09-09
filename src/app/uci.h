#pragma once

#include <mutex>
#include <string>

#include "app/search_thread.h"
#include "board/position.h"

class UciEngine
{
  public:
    explicit UciEngine(SearchThread& searchThread);

    // Returns false when the engine should exit (quit).
    bool execute(const std::string& line);

  private:
    SearchThread& searchThread_;
    Position position_;
    std::mutex outputMutex_;

    int threads_{1};
    int hashMB_{16};
    int moveOverheadMS_{10};

    void writeLine(const std::string& line);
    void handleUci();
    void handleSetOption(const std::string& line);
    void handlePosition(const std::string& line);
    void handleGo(const std::string& line);
    void handleStop();
    void handlePerft(int depth);

    void printFen();
    void printLegalMoves();
    void printBoard();
    void printEval();
    void printHash();

    [[nodiscard]] bool hasLegalMoves() const;
};
