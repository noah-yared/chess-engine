#pragma once

#include <atomic>
#include <cassert>
#include <thread>

#include "work_stealing_queue.h"

// Work-stealing thread pool with one Chase-Lev deque per worker.
//
// `num_workers` is total parallelism and includes the constructing thread
// as worker 0. That thread is attached in the constructor; helpers 1..N-1
// are spawned and each runs an idle loop (local pop, then steal).
//
// A pool worker may submit() onto its own deque (pushBottom) and help with
// tryRunOne() (popBottom, else steal). Tasks may themselves submit further
// work. Do not submit from a thread that is not a pool worker.
//
// On destruction, the constructing thread drains remaining work, signals
// shutdown, and joins the helpers.
//
// Deques follow the Chase-Lev work-stealing design:
// https://dl.acm.org/doi/epdf/10.1145/1073970.1073974
class ThreadPool
{
  public:
    // `num_workers` includes main worker (thread that runs constructor)
    explicit ThreadPool(int num_workers = 8)
        : workers_{}, worker_queues_(num_workers), shutdown_{false}, num_workers_{num_workers}
    {
        assert(num_workers >= 1);
        attach();
        setupWorkers();
    }

    ~ThreadPool() { shutdownAndJoin(); }

    [[nodiscard]] static int workerId() noexcept { return worker_id_; }
    [[nodiscard]] int numWorkers() const noexcept { return num_workers_; }

    // Submit a task to the thread pool by pushing it directly
    // onto the bottom of the respective thread's local task queue.
    void submit(const Task& task)
    {
        assert(worker_id_ < num_workers_ && worker_id_ >= 0);
        worker_queues_[worker_id_].pushBottom(task);
    }

    // Pops a task from bottom of the queue and executes
    // if task successfully popped, ie thread-local queue
    // was non-empty or stole a task from other thread's queue.
    bool tryRunOne()
    {
        // Attempt to pop and run task from local queue.
        Task task, *ptask = &task;
        if (worker_queues_[worker_id_].popBottom(&ptask); ptask != nullptr)
        {
            // Popped from local queue
            task();
            return true;
        }
        // Local task queue is empty, attempt to steal task from other worker.
        for (int i = 1; i < num_workers_; ++i)
        {
            int other_id = (worker_id_ + i) % num_workers_;
            auto& other_queue = worker_queues_[other_id];
            if (Task* ptask = &task; other_queue.steal(&ptask) && ptask != nullptr)
            {
                // Successfully stole a task
                task();
                return true;
            }
        }
        return false;
    }

  private:
    static thread_local inline int worker_id_{-1};

    std::vector<std::thread> workers_;
    std::vector<WorkStealingQueue> worker_queues_;
    std::atomic<bool> shutdown_;
    const int num_workers_;

    // Attach the main thread to worker_id = 0.
    static void attach() { worker_id_ = 0; }

    // Spawn workers that will execute the runWorker loop.
    void setupWorkers()
    {
        for (int id = 1; id < num_workers_; ++id)
        {
            workers_.emplace_back(std::bind_front(&ThreadPool::runWorker, this, id));
        }
    }

    // Execution loop for a worker thread.
    void runWorker(int id)
    {
        worker_id_ = id;
        // Loop while there are tasks to run and not shutting down.
        while (tryRunOne() || !shutdown_.load(std::memory_order_relaxed))
            ;
    }

    // Drain the remaining tasks in the queue and join the workers.
    // Must be run from the main thread (worker #0)
    void shutdownAndJoin()
    {
        assert(worker_id_ == 0);
        while (tryRunOne())
            ;
        shutdown_.store(true, std::memory_order_relaxed);
        for (auto& worker : workers_)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }
    }
};
