#pragma once

#include <atomic>
#include <functional>

using Task = std::function<void()>;

// Implementation is mostly based on the Chase-Lev Work-Stealing Deque.
// Link: https://dl.acm.org/doi/epdf/10.1145/1073970.1073974
class WorkStealingQueue
{
  public:
    // `log_size` is the log2 of the initial circular capacity.
    explicit WorkStealingQueue(int log_size = 8)
        : active_buffer_(std::make_shared<CircularBuffer>(log_size))
    {
    }

    // Not copyable/movable.
    WorkStealingQueue(const WorkStealingQueue&) = delete;
    WorkStealingQueue& operator=(const WorkStealingQueue&) = delete;

    // Pushes the task onto the task queue.
    // Grows the queue if capacity is reached.
    void pushBottom(Task task)
    {
        auto bottom = bottom_.load();
        auto top = top_.load();
        // No synchronization since this only runs on owner.
        auto tasks = active_buffer_;
        if (bottom - top >= tasks->size() - 1)
        {
            // Grow the circular array. Store atomically to prevent races
            // with thiefs.
            std::atomic_store(&active_buffer_, tasks->grow(bottom, top));
            tasks = active_buffer_;
        }
        (*tasks)[bottom] = std::move(task);
        bottom_.store(bottom + 1);
    }

    // Returns false if the steal operation was aborted
    // due to a concurrent pop/steal.
    // Returns true and sets *task to nullptr if there
    // were no tasks to steal, ie deque was empty.
    // Returns true and writes the stolen task into **task
    // if the steal operation was successful.
    [[nodiscard]] bool steal(Task** task)
    {
        auto top = top_.load();
        auto bottom = bottom_.load();
        // Atomically load shared_ptr to active buffer to ensure no
        // use-after-free when growing buffer.
        auto tasks = std::atomic_load(&active_buffer_);
        if (bottom - top <= 0)
        {
            *task = nullptr;
            return true;
        }
        **task = (*tasks)[top];
        return top_.compare_exchange_strong(top, top + 1);
    }

    // Sets task to nullptr if task queue is empty.
    // Else, writes the task at the bottom of the
    // queue into the output parameter task.
    void popBottom(Task** task)
    {
        auto bottom = bottom_.load();
        // No synchronization since this only runs on owner.
        auto tasks = active_buffer_;
        bottom = bottom - 1;
        bottom_.store(bottom);
        auto top = top_.load();
        auto size = bottom - top;
        if (size < 0)
        {
            bottom_.store(top);
            *task = nullptr;
            return;
        }
        **task = (*tasks)[bottom];
        if (size > 0)
        {
            return;
        }
        // Last element: compete with steal. compare_exchange_strong writes the
        // observed top into `top` on failure, so bottom must be restored from
        // the pre-CAS index. Using the updated `top` skips a slot and later
        // steal invokes an empty std::function (SIGABRT).
        const auto taken = top;
        if (!top_.compare_exchange_strong(top, taken + 1))
            *task = nullptr;
        bottom_.store(taken + 1);
    }

  private:
    // Index of the next available slot in the array,
    // where the next new element should be pushed.
    std::atomic<int64_t> bottom_{0};

    // Index of the topmost element in the array
    // (if there is any), incremented on each steal.
    std::atomic<int64_t> top_{0};

    // Holds a grow-able dynamically allocated circular
    // buffer with power-of-two size. Uses std::shared_ptr
    // so that old references held by thiefs are automatically
    // deallocated once no more references remain.
    class CircularBuffer
    {
      public:
        explicit CircularBuffer(int log_size) : log_size_(log_size), data_(new Task[1 << log_size])
        {
        }

        // Disable copying/moving since this class manages resource handle.
        CircularBuffer(const CircularBuffer&) = delete;
        CircularBuffer& operator=(const CircularBuffer&) = delete;

        ~CircularBuffer() { delete[] data_; }

        [[nodiscard]] size_t size() const { return 1UL << log_size_; }

        [[nodiscard]] Task& operator[](size_t index) { return data_[index & (size() - 1)]; }
        [[nodiscard]] const Task& operator[](size_t index) const
        {
            return data_[index & (size() - 1)];
        }

        [[nodiscard]] std::shared_ptr<CircularBuffer> grow(int64_t b, int64_t t)
        {
            auto new_buffer = std::make_shared<CircularBuffer>(log_size_ + 1);
            for (auto i = t; i < b; ++i)
            {
                (*new_buffer)[i] = (*this)[i];
            }
            return new_buffer;
        }

      private:
        int log_size_;
        Task* data_;
    };
    std::shared_ptr<CircularBuffer> active_buffer_;
};
