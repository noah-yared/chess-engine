#include "concurrency/thread_pool.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>
#include <vector>

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

TEST(ThreadPoolTest, TryRunOneOnEmptyPoolReturnsFalse)
{
    ThreadPool pool(1);
    EXPECT_FALSE(pool.tryRunOne());
}

TEST(ThreadPoolTest, SubmitIsLifoOnLocalDeque)
{
    ThreadPool pool(1);
    std::vector<int> observedOrder;

    pool.submit([&observedOrder] { observedOrder.push_back(1); });
    pool.submit([&observedOrder] { observedOrder.push_back(2); });
    pool.submit([&observedOrder] { observedOrder.push_back(3); });

    EXPECT_TRUE(pool.tryRunOne());
    EXPECT_TRUE(pool.tryRunOne());
    EXPECT_TRUE(pool.tryRunOne());
    EXPECT_FALSE(pool.tryRunOne());

    EXPECT_EQ(observedOrder, (std::vector<int>{3, 2, 1}));
}

TEST(ThreadPoolTest, HelpersStealAndRunEachTaskOnce)
{
    constexpr int kTaskCount = 256;
    ThreadPool pool(4);
    std::vector<std::atomic<int>> executionCounts(kTaskCount);
    std::atomic<int> completed{0};

    for (auto& executionCount : executionCounts)
        executionCount.store(0);

    for (int taskId = 0; taskId < kTaskCount; ++taskId)
    {
        pool.submit(
            [&executionCounts, &completed, taskId]
            {
                executionCounts[taskId].fetch_add(1);
                completed.fetch_add(1);
            });
    }

    ASSERT_TRUE(waitUntil([&completed] { return completed.load() >= kTaskCount; }))
        << "completed " << completed.load() << " of " << kTaskCount << " tasks";

    EXPECT_EQ(completed.load(), kTaskCount);
    for (std::size_t taskId = 0; taskId < executionCounts.size(); ++taskId)
        EXPECT_EQ(executionCounts[taskId].load(), 1) << "task " << taskId;
}

TEST(ThreadPoolTest, MainWorkerAndHelpersCooperate)
{
    constexpr int kTaskCount = 256;
    const auto mainThreadId = std::this_thread::get_id();
    std::atomic<bool> releaseHelpers{false};
    std::atomic<int> mainExecutions{0};
    std::atomic<int> helperExecutions{0};
    std::atomic<int> completed{0};
    ThreadPool pool(4);

    for (int i = 0; i < kTaskCount; ++i)
    {
        pool.submit(
            [&]
            {
                if (std::this_thread::get_id() == mainThreadId)
                    mainExecutions.fetch_add(1);
                else
                {
                    helperExecutions.fetch_add(1);
                    while (!releaseHelpers.load())
                        std::this_thread::sleep_for(1ms);
                }
                completed.fetch_add(1);
            });
    }

    EXPECT_TRUE(waitUntil([&helperExecutions] { return helperExecutions.load() > 0; }));

    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (mainExecutions.load() == 0 && std::chrono::steady_clock::now() < deadline)
        (void)pool.tryRunOne();

    EXPECT_GT(mainExecutions.load(), 0);
    releaseHelpers.store(true);

    // Finish any remaining local work on the main worker.
    while (pool.tryRunOne())
        ;

    EXPECT_TRUE(waitUntil([&completed] { return completed.load() >= kTaskCount; }));
    EXPECT_EQ(completed.load(), kTaskCount);
}

TEST(ThreadPoolTest, NestedSubmitFromRunningTask)
{
    constexpr int kChildTasks = 64;
    ThreadPool pool(4);
    std::atomic<int> completed{0};

    pool.submit(
        [&]
        {
            for (int i = 0; i < kChildTasks; ++i)
                pool.submit([&completed] { completed.fetch_add(1); });
        });

    while (pool.tryRunOne())
        ;

    EXPECT_TRUE(waitUntil([&completed] { return completed.load() >= kChildTasks; }));
    EXPECT_EQ(completed.load(), kChildTasks);
}

TEST(ThreadPoolTest, DestructorDrainsQueuedTasks)
{
    constexpr int kTaskCount = 256;
    std::atomic<int> completed{0};

    {
        ThreadPool pool(4);
        for (int i = 0; i < kTaskCount; ++i)
            pool.submit([&completed] { completed.fetch_add(1); });
    }

    EXPECT_EQ(completed.load(), kTaskCount);
}
