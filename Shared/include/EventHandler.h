#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

template <typename... TArgs>
class EventHandler
{
public:
    using CallbackId = std::uint64_t;
    using EventCallback = std::function<void(TArgs...)>;
    using FunctionPointer = void (*)(TArgs...);

    std::optional<CallbackId> AddCallback(FunctionPointer callback)
    {
        if (callback == nullptr)
            return std::nullopt;

        return AddCallbackImpl(EventCallback{callback}, callback);
    }

    std::optional<CallbackId> AddCallback(EventCallback callback)
    {
        if (!callback)
            return std::nullopt;

        FunctionPointer functionPointer = nullptr;
        if (const auto target = callback.template target<FunctionPointer>())
            functionPointer = *target;

        return AddCallbackImpl(std::move(callback), functionPointer);
    }

    bool RemoveCallback(CallbackId id)
    {
        std::lock_guard<std::mutex> lock(mutex);

        for (auto callback = callbacks.begin(); callback != callbacks.end(); ++callback)
        {
            if (callback->id == id)
            {
                callbacks.erase(callback);
                return true;
            }
        }

        return false;
    }

    void Invoke(TArgs... args) const
    {
        std::vector<EventCallback> callbackSnapshot;
        {
            std::lock_guard<std::mutex> lock(mutex);
            callbackSnapshot.reserve(callbacks.size());
            for (const auto &callback : callbacks)
                callbackSnapshot.push_back(callback.callback);
        }

        // Run callbacks without holding the mutex so they can modify subscriptions during dispatch.
        for (const auto &callback : callbackSnapshot)
            callback(args...);
    }

private:
    struct CallbackEntry
    {
        CallbackId id;
        EventCallback callback;
        FunctionPointer functionPointer;
    };

    std::optional<CallbackId> AddCallbackImpl(EventCallback callback, FunctionPointer functionPointer)
    {
        std::lock_guard<std::mutex> lock(mutex);

        if (functionPointer != nullptr)
        {
            for (const auto &entry : callbacks)
            {
                if (entry.functionPointer == functionPointer)
                    return std::nullopt;
            }
        }

        if (nextCallbackId == 0)
            return std::nullopt;

        const CallbackId id = nextCallbackId++;
        callbacks.push_back({id, std::move(callback), functionPointer});
        return id;
    }

    mutable std::mutex mutex;
    std::vector<CallbackEntry> callbacks;
    CallbackId nextCallbackId = 1;
};
