#pragma once

#include <vector>
#include <queue>
#include <utility>

template <typename T>
class Registry
{
    std::vector<T> elements{};
    std::queue<std::size_t> vacantIndices{};

public:
    std::size_t Add(T &&element)
    {
        std::size_t index;
        if (!vacantIndices.empty())
        {
            index = vacantIndices.front();
            vacantIndices.pop();
            elements[index] = element;
            return index;
        }

        index = elements.size();
        elements.push_back(element);
        return index;
    }
};