# Chess Engine

This is a basic chess engine that can generate legal moves and perform simple search to decide on moves. The project is in early stages, so while the core functionality works, there’s still plenty of room for improvements like better move evaluation, advanced search techniques, and UI integration.

## Features so far
- Legal move generation for all pieces, including special moves (castling, en passant, promotion)
- Alphabeta pruning search algorithm to explore possible moves and pick the best one
- Board representation and move application

## Future plans
- Add evaluation heuristics for stronger play
- Improve move ordering and quiescence search
- Optimize move generation (possibly use magic bitboards or other strategies)
- Build a user interface or integrate with existing chess GUIs

## Build & Run
To clone the repo run:
```bash
cd some/path/to/folder
git clone "https://github.com/noah-yared/chess-engine.git"
cd chess-engine
```
To build all the src/include files and run the code in main.cpp ensure that both ```#define DEBUG``` and ```#define TEST``` are commented (at top of ```./src/main.cpp```) as follows and execute the command ```make run```.
```cpp
// Uncomment for debugging mode
// #define DEBUG

// Uncomment for testing
// #define TEST
```

To run the tests in ```./tests/tests.cpp``` ensure that ```#define DEBUG```  and ```#define TEST``` are respectively commented and uncommented (at top of ```./src/main.cpp```) as follows and execute the command ```make tests```.
```cpp
// Uncomment for debugging mode
// #define DEBUG

// Uncomment for testing
#define TEST
``` 

To remove previously built files before recompiling execute the command ```make clean```. 
