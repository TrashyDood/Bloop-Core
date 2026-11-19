#pragma once

#include <atomic>
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

template <typename TValueType>
using ThreadTask = std::future<TValueType>;
using ThreadCount = std::uint8_t;

template <ThreadCount TThreadCount = 32, ThreadCount TDefaultThreadCount = 8>
class ThreadPool
{
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    bool isStopping = false;

public:
    explicit ThreadPool()
    {
        for (std::size_t i = 0; i < TThreadCount; i++)
            workers.emplace_back([this]() { WorkerLoop(); });
    }

    ~ThreadPool()
    {
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            isStopping = true;
        }
        condition.notify_all();
        for (auto &worker : workers)
            worker.join();
    }

    template <typename TFunc, typename... TArgs>
    [[nodiscard]]
    auto SubmitTask(TFunc &&f, TArgs &&...args)
    {
        using ReturnType = typename std::result_of<TFunc(TArgs...)>::type;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<TFunc>(f), std::forward<TArgs>(args)...));

        ThreadTask<ReturnType> result = task->get_future();

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (isStopping)
                throw std::runtime_error("SubmitTask called on stopped ThreadPool");
            tasks.emplace([task]() { (*task)(); });
        }

        condition.notify_one();
        return result;
    }

    template <typename TFunc>
    [[nodiscard]]
    auto For_Parallel(TFunc &&func, const std::size_t count, const ThreadCount threadCount = TDefaultThreadCount)
    {
        using ReturnType = std::invoke_result_t<TFunc &, uint16_t, std::size_t, std::size_t>;

        std::vector<ThreadTask<ReturnType>> tasks;
        if (count == 0 || threadCount == 0)
            return tasks;

        const std::size_t chunkSize = (count + threadCount - 1) / threadCount,
                          chunkCount = (count + chunkSize - 1) / chunkSize; // aka. actual thread count

        tasks.reserve(chunkCount);

        for (std::size_t chunk = 0; chunk < chunkCount; ++chunk)
        {
            const auto thread = static_cast<uint16_t>(chunk);
            const std::size_t chunkStart = chunk * chunkSize;
            const std::size_t chunkEnd = std::min(chunkStart + chunkSize, count);

            tasks.emplace_back(SubmitTask(
                [chunkStart, chunkEnd, thread, func]() mutable { return func(thread, chunkStart, chunkEnd); }));
        }

        return tasks;
    }

    template <typename TResult, typename TContainer, typename TFunc>
    [[nodiscard]]
    auto ForEach_Parallel(TFunc &&func, TContainer &values, const ThreadCount threadCount = TDefaultThreadCount)
    {
        auto chunkFunc = [&values, func = std::forward<TFunc>(func)](uint16_t, uint32_t start, uint32_t end) mutable
        {
            if constexpr (std::is_void_v<TResult>)
                for (std::size_t i = start; i < end; ++i)
                    func(values[i], i);
            else
            {
                TResult results{};
                for (std::size_t i = start; i < end; ++i)
                    func(values[i], i, results);
                return results;
            }
        };

        return For_Parallel(std::move(chunkFunc), values.size(), threadCount);
    }

private:
    void WorkerLoop()
    {
        while (true)
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                condition.wait(lock, [this]() { return isStopping || !tasks.empty(); });

                if (isStopping && tasks.empty())
                    return;

                task = std::move(tasks.front());
                tasks.pop();
            }

            task();
        }
    }
};

[[nodiscard]]
inline ThreadPool<32> &StaticThreadPool()
{
    static ThreadPool<32> instance;
    return instance;
}