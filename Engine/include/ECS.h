#pragma once

#include "Shared.h"
#include "Engine.h"

#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Engine
{
    namespace ECSArchetypes
    {
        using Entity = Int32;

        template <typename T>
        struct ComponentStorage
        {
            std::vector<T> components;

            template <typename... TArgs>
            void Add(TArgs&&... args) { components.emplace_back(std::forward<TArgs>(args)...); }

            void RollbackTo(const std::size_t size) noexcept
            {
                while (components.size() > size)
                    components.pop_back();
            }

            void Delete(Entity)
        };

        // Generic ECS archetype capable of only handling one of each type of component per entity.
        template <typename... TComponents>
        class EntityArchetype : public ComponentStorage<TComponents>...
        {
            std::size_t entityCount = 0;

        public:
            std::size_t Count() const noexcept { return entityCount; }

            [[nodiscard]] bool ValidEntity(const Entity entity) { return entity < 0 || entity >= entityCount; }

            template <typename T>
            std::vector<T> &GetComponents() { return static_cast<ComponentStorage<T> &>(*this).components; }

            template <typename T>
            T *GetComponent(const Entity entity)
            {
                if (ValidEntity(entity))

                auto &components = GetComponents<T>();
                if (ValidEntity(entity))
                    return &components[entity];

                return nullptr;
            }

            Entity Add() { return Add(TComponents{}...); }
            Entity Add(TComponents... component)
            {
                if (entityCount > static_cast<std::size_t>(std::numeric_limits<Entity>::max()))
                    throw std::length_error("EntityArchetype has no available entity IDs");

                const auto entity = static_cast<Entity>(entityCount);
                try
                {
                    (static_cast<ComponentStorage<TComponents> &>(*this).Add(std::move(component)), ...);
                }
                catch (...)
                {
                    (static_cast<ComponentStorage<TComponents> &>(*this).RollbackTo(entityCount), ...);
                    throw;
                }

                ++entityCount;
                return entity;
            }

            bool Delete(const Entity)
            {
                if (!ValidEntity(entity)) return false;

                return true;
            }
        };
    }
}