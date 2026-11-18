#pragma once

#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <future>
#include <atomic>
#include <type_traits>
#include <utility>

template<typename TValueType>
using ThreadTask = std::future<TValueType>;

template<unsigned char TThreadCount>
class ThreadPool
{
	std::vector<std::thread> workers;
	std::queue<std::function<void()>> tasks;

	std::mutex queueMutex;
	std::condition_variable condition;
	bool stopping = false;

public:
	static constexpr unsigned char ThreadCount = TThreadCount;

	explicit ThreadPool()
	{
		for (size_t i = 0; i < TThreadCount; i++)
			workers.emplace_back([this]() { WorkerLoop(); });
	}

	~ThreadPool()
	{
		{
			std::lock_guard<std::mutex> lock(queueMutex);
			stopping = true;
		}
		condition.notify_all();
		for (auto& worker : workers)
			worker.join();
	}

	template<typename TFunc, typename... Args>
	auto SubmitTask(TFunc&& f, Args&&... args) -> ThreadTask<typename std::result_of<TFunc(Args...)>::type>
	{
		using ReturnType = typename std::result_of<TFunc(Args...)>::type;

		auto task = std::make_shared<std::packaged_task<ReturnType()>>(
			std::bind(std::forward<TFunc>(f), std::forward<Args>(args)...));

		ThreadTask<ReturnType> result = task->get_future();

		{
			std::lock_guard<std::mutex> lock(queueMutex);
			if (stopping)
				throw std::runtime_error("SubmitTask called on stopped ThreadPool");
			tasks.emplace([task]() { (*task)(); });
		}

		condition.notify_one();
		return result;
	}
	
	template<int TParallelForThreadCount, typename TFunc>
	auto For_Parallel(TFunc&& func, size_t count)
	{
		using ReturnType = typename std::result_of<TFunc(uint16_t, uint32_t, uint32_t)>::type;

		size_t chunkSize = (count + TParallelForThreadCount - 1) / TParallelForThreadCount;
		std::vector<ThreadTask<ReturnType>> tasks;

		if (chunkSize < 1) return tasks;

		tasks.reserve(TParallelForThreadCount);

		for (uint16_t thread = 0; thread < TParallelForThreadCount; thread++)
		{
			uint32_t chunkStart = thread * chunkSize;
			if (chunkStart >= count)
				break;

			uint32_t chunkEnd = std::min(chunkStart + chunkSize, count);

			auto chunkTask = [chunkStart, chunkEnd, thread, func]() mutable {
				return func(thread, chunkStart, chunkEnd); };

			tasks.emplace_back(SubmitTask(std::move(chunkTask)));
		}

		return tasks;
	}

	template<int TParallelForThreadCount, typename TResult, typename TContainer, typename TFunc>
	auto ForEach_Parallel(TContainer& values, TFunc&& func)
	{
		auto chunkFunc = [&values, func = std::forward<TFunc>(func)](
			uint16_t, uint32_t start, uint32_t end) mutable
		{
			if constexpr (std::is_void_v<TResult>)
			{
				for (std::size_t i = start; i < end; ++i)
					func(values[i], i);
			}
			else
			{
				TResult results{};

				for (std::size_t i = start; i < end; ++i)
					func(values[i], i, results);
				return results;
			}
		};

		return For_Parallel<TParallelForThreadCount>(std::move(chunkFunc), values.size());
	}

private:
	void WorkerLoop()
	{
		while (true)
		{
			std::function<void()> task;
			{
				std::unique_lock<std::mutex> lock(queueMutex);
				condition.wait(lock, [this]() { return stopping || !tasks.empty(); });

				if (stopping && tasks.empty())
					return;

				task = std::move(tasks.front());
				tasks.pop();
			}

			task();
		}
	}
};

inline ThreadPool<32>& StaticThreadPool() { static ThreadPool<32> instance; return instance; }