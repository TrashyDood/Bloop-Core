#include "ThreadPool.h"

ThreadPool::ThreadPool(const std::uint8_t workerCount)
{
    workers.reserve(workerCount);
    for (std::size_t i = 0; i < workerCount; ++i)
        workers.emplace_back([this]() { WorkerLoop(); });
}

ThreadPool::~ThreadPool()
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        isStopping = true;
    }
    condition.notify_all();
    for (auto &worker : workers)
        worker.join();
}

ThreadPool *&ThreadPool::CurrentPool()
{
    static thread_local ThreadPool *pool = nullptr;
    return pool;
}

bool ThreadPool::TryRunOneTask()
{
    std::function<void()> task;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (tasks.empty())
            return false;

        task = std::move(tasks.front());
        tasks.pop();
    }

    // Run outside the lock so the task can submit more work.
    task();
    return true;
}

void ThreadPool::WorkerLoop()
{
    CurrentPool() = this; // Sets the active pool for this thread

    while (true)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this]() { return isStopping || !tasks.empty(); });

            if (isStopping && tasks.empty())
            {
                CurrentPool() = nullptr;
                return;
            }

            task = std::move(tasks.front());
            tasks.pop();
        }

        task();
    }
}

ThreadPool &StaticThreadPool()
{
    static ThreadPool instance(32);
    return instance;
}
