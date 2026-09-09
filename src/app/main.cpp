#ifdef ENGINE_UCI_INTERFACE
#include "app/search_thread.h"
#include "app/uci.h"

#include <iostream>
#include <sstream>
#include <string>

namespace
{

std::string joinArgs(int argc, const char* argv[])
{
    std::ostringstream oss;
    for (int i = 1; i < argc; ++i)
    {
        if (i > 1)
            oss << ' ';
        oss << argv[i];
    }
    return oss.str();
}

} // namespace

int main(int argc, const char* argv[])
{
    SearchThread searchThread;
    UciEngine engine(searchThread);

    if (argc > 1)
    {
        engine.execute(joinArgs(argc, argv));
        searchThread.waitIdle();
        return 0;
    }

    std::string line;
    while (std::getline(std::cin, line))
    {
        if (!engine.execute(line))
            break;
    }

    searchThread.waitIdle();
    return 0;
}
#else
#include "app/cli.h"

int main(int argc, const char* argv[]) { return cli::runCli(argc, argv); }
#endif
