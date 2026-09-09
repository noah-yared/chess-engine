#include "app/search_thread.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "board/constants.h"
#include "search/search_types.h"
#include "test_utils.h"

namespace
{
using namespace std::chrono_literals;

template <typename Predicate>
bool waitUntil(Predicate&& predicate, std::chrono::milliseconds timeout = 2s)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (predicate())
            return true;
        std::this_thread::sleep_for(1ms);
    }
    return predicate();
}
} // namespace

TEST(SearchThreadTest, TryRunRejectsWhenBusy)
{
    SearchThread searchThread;
    std::atomic<bool> gate{false};
    std::atomic<bool> taskStarted{false};

    ASSERT_TRUE(searchThread.tryRun(
        [&](EngineController&, const std::atomic<bool>*)
        {
            taskStarted.store(true);
            while (!gate.load(std::memory_order_relaxed))
                std::this_thread::sleep_for(1ms);
        }));

    ASSERT_TRUE(waitUntil([&] { return taskStarted.load(); }));
    EXPECT_FALSE(searchThread.tryRun([](EngineController&, const std::atomic<bool>*) {}));

    gate.store(true, std::memory_order_relaxed);
    searchThread.waitIdle();
}

TEST(SearchThreadTest, WaitIdleDrainsPostedTask)
{
    SearchThread searchThread;
    std::atomic<bool> gate{false};
    std::atomic<bool> taskFinished{false};

    ASSERT_TRUE(searchThread.tryRun(
        [&](EngineController&, const std::atomic<bool>*)
        {
            while (!gate.load(std::memory_order_relaxed))
                std::this_thread::sleep_for(1ms);
            taskFinished.store(true);
        }));

    EXPECT_FALSE(taskFinished.load());
    gate.store(true, std::memory_order_relaxed);
    searchThread.waitIdle();
    EXPECT_TRUE(taskFinished.load());
}

TEST(SearchThreadTest, RunCancellingIfBusyReplacesInFlightTask)
{
    SearchThread searchThread;
    std::atomic<bool> gate{false};
    std::atomic<bool> firstTaskStarted{false};
    std::atomic<bool> secondTaskRan{false};

    searchThread.runCancellingIfBusy(
        [&](EngineController&, const std::atomic<bool>* stopFlag)
        {
            firstTaskStarted.store(true);
            while (!gate.load(std::memory_order_relaxed) &&
                   !(stopFlag != nullptr && stopFlag->load(std::memory_order_relaxed)))
                std::this_thread::sleep_for(1ms);
        });

    ASSERT_TRUE(waitUntil([&] { return firstTaskStarted.load(); }));

    searchThread.runCancellingIfBusy(
        [&](EngineController&, const std::atomic<bool>*)
        {
            secondTaskRan.store(true);
        });

    ASSERT_TRUE(waitUntil([&] { return secondTaskRan.load(); }));
    searchThread.waitIdle();
}

TEST(SearchThreadTest, CancelAbortsLongRunningSearch)
{
    SearchThread searchThread;
    SearchResult result{};
    std::atomic<bool> searchStarted{false};

    ASSERT_TRUE(searchThread.tryRun(
        [&](EngineController& engine, const std::atomic<bool>* stopFlag)
        {
            searchStarted.store(true);
            result = engine.search(SearchConfig::fixedDepth(MAX_SEARCH_DEPTH), stopFlag);
        }));

    ASSERT_TRUE(waitUntil([&] { return searchStarted.load(); }));
    std::this_thread::sleep_for(200ms);
    searchThread.cancel();
    searchThread.waitIdle();

    EXPECT_GT(result.stats.nodesSearched, 0ULL);
    EXPECT_TRUE(isLegalMove(Position::fromStartingPosition(), result.bestMove));
}

TEST(SearchThreadTest, DestructorJoinsUnderRunningSearch)
{
    std::atomic<bool> searchStarted{false};
    std::atomic<bool> searchFinished{false};

    {
        SearchThread searchThread;
        ASSERT_TRUE(searchThread.tryRun(
            [&](EngineController& engine, const std::atomic<bool>* stopFlag)
            {
                searchStarted.store(true);
                (void)engine.search(SearchConfig::fixedDepth(MAX_SEARCH_DEPTH), stopFlag);
                searchFinished.store(true);
            }));

        ASSERT_TRUE(waitUntil([&] { return searchStarted.load(); }));
    }

    EXPECT_TRUE(searchFinished.load());
}
