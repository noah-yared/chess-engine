#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>

#include "search/engine_controller.h"

class SearchThread
{
  public:
    using Task = std::function<void(EngineController&, const std::atomic<bool>*)>;

    SearchThread()
        : mu_{},
          task_cv_{},
          idle_cv_{},
          task_{},
          shutdown_{false},
          busy_{false},
          cancelled_{false},
          thread_{std::bind_front(&SearchThread::searchThreadLoop, this)}
    {
    }

    ~SearchThread()
    {
        shutdown();
        join();
    }

    SearchThread(const SearchThread&) = delete;
    SearchThread& operator=(const SearchThread&) = delete;

    bool tryRun(Task&& task)
    {
        std::lock_guard lock(mu_);
        if (busy_.load(std::memory_order_acquire))
            return false;

        task_ = std::move(task);
        cancelled_.store(false, std::memory_order_relaxed);
        busy_.store(true, std::memory_order_release);
        task_cv_.notify_one();
        return true;
    }

    void cancel() { cancelled_.store(true, std::memory_order_relaxed); }

    void waitIdle()
    {
        std::unique_lock lock(mu_);
        idle_cv_.wait(lock, [this]() { return !busy_.load(std::memory_order_acquire); });
    }

  private:
    std::mutex mu_;
    std::condition_variable task_cv_;
    std::condition_variable idle_cv_;
    std::optional<Task> task_;
    std::atomic<bool> shutdown_;
    std::atomic<bool> busy_;
    std::atomic<bool> cancelled_;
    std::thread thread_;

    void shutdown()
    {
        cancelled_.store(true, std::memory_order_relaxed);
        std::lock_guard lock(mu_);
        shutdown_.store(true, std::memory_order_release);
        task_cv_.notify_one();
    }

    void join() { thread_.join(); }

    void searchThreadLoop()
    {
        EngineController engine_controller;

        while (!shutdown_.load(std::memory_order_acquire))
        {
            Task task;
            {
                std::unique_lock lock(mu_);
                task_cv_.wait(lock, [this]() {
                    return task_ || shutdown_.load(std::memory_order_acquire);
                });
                if (shutdown_.load(std::memory_order_acquire))
                    break;

                task = *std::exchange(task_, std::nullopt);
            }
            task(engine_controller, &cancelled_);
            {
                std::lock_guard lock(mu_);
                busy_.store(false, std::memory_order_release);
                idle_cv_.notify_one();
            }
        }
    }
};
