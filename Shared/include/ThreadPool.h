#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

class ThreadPool;

template <typename TResult>
class ThreadTask
{
    std::future<TResult> future;
    ThreadPool *threadPool;

    ThreadTask(std::future<TResult> &&future, ThreadPool &threadPool)
        : future(std::move(future)), threadPool(&threadPool) {}

    friend class ThreadPool;

public:
    [[nodiscard]] std::future_status Status() const
    {
        return future.wait_for(std::chrono::milliseconds(0));
    }

    ThreadTask(const ThreadTask &) = delete;
    ThreadTask &operator=(const ThreadTask &) = delete;
    ThreadTask(ThreadTask &&) = default;
    ThreadTask &operator=(ThreadTask &&) = default;

    void Wait();
    decltype(auto) Get()
    {
        Wait();
        return future.get();
    }
};

class ThreadPool
{
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    bool isStopping = false;

    // Identifies which pool owns the current worker thread.
    static ThreadPool *&CurrentPool();
    // Lets a waiting worker make progress on work queued behind its blocked task.
    bool TryRunOneTask();
    void WorkerLoop();

    template <typename TResult>
    void Wait(std::future<TResult> &task)
    {
        // Only workers from this pool help; other callers wait normally.
        if (CurrentPool() != this)
        {
            task.wait();
            return;
        }

        while (true)
        {
            const auto status = task.wait_for(std::chrono::milliseconds(0));
            if (status == std::future_status::ready)
                return;
            if (status == std::future_status::deferred)
            {
                task.wait();
                return;
            }

            if (!TryRunOneTask())
                task.wait_for(std::chrono::milliseconds(1));
        }
    }

    template <typename>
    friend class ThreadTask;

public:
    [[nodiscard]] std::uint8_t WorkerCount() const { return static_cast<std::uint8_t>(workers.size()); }

    explicit ThreadPool(const std::uint8_t workerCount = 32);
    ~ThreadPool();

    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ThreadPool(ThreadPool &&) = delete;
    ThreadPool &operator=(ThreadPool &&) = delete;

    template <typename TFunc, typename... TArgs>
    [[nodiscard]]
    auto SubmitTask(TFunc &&func, TArgs &&...args)
    {
        using ReturnType = typename std::result_of<TFunc(TArgs...)>::type;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<TFunc>(func), std::forward<TArgs>(args)...));

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (isStopping)
                throw std::runtime_error("SubmitTask called on stopped ThreadPool");
            tasks.emplace([task]() { (*task)(); });
        }

        condition.notify_one();
        return ThreadTask<ReturnType>(task->get_future(), *this);
    }

    template <typename TFunc>
    [[nodiscard]]
    auto For_Parallel(const std::size_t workItemCount, TFunc &&func)
    {
        return For_Parallel(
            workItemCount, static_cast<std::uint8_t>(workers.size()), std::forward<TFunc>(func));
    }

    template <typename TFunc>
    [[nodiscard]]
    auto For_Parallel(const std::size_t workItemCount, const std::uint8_t desiredWorkerCount, TFunc &&func)
    {
        using ReturnType = std::invoke_result_t<TFunc &, uint16_t, std::size_t, std::size_t>;

        std::vector<ThreadTask<ReturnType>> tasks;
        if (workItemCount < 1 || desiredWorkerCount < 1)
            return tasks;

        const std::size_t chunkSize = (workItemCount + desiredWorkerCount - 1) / desiredWorkerCount;
        const std::size_t chunkCount = (workItemCount + chunkSize - 1) / chunkSize;

        tasks.reserve(chunkCount);

        for (std::size_t chunk = 0; chunk < chunkCount; ++chunk)
        {
            const auto thread = static_cast<uint16_t>(chunk);
            const std::size_t chunkStart = chunk * chunkSize; // Inclusive
            const std::size_t chunkEnd = std::min(chunkStart + chunkSize, workItemCount); // Exclusive

            tasks.emplace_back(SubmitTask(
                [chunkStart, chunkEnd, thread, func]() mutable
                {
                    return func(thread, chunkStart, chunkEnd);
                }));
        }

        return tasks;
    }

};

template <typename TResult>
void ThreadTask<TResult>::Wait()
{
    if (Status() != std::future_status::ready)
        threadPool->Wait(future);
}

[[nodiscard]]
ThreadPool &StaticThreadPool();