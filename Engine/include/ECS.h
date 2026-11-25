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
        template <typename T>
        using ComponentList = std::vector<T>;

        template <typename... TComps>
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
        };

        // Generic ECS archetype capable of only handling one of each type of component per entity.
        template <typename... TComponents>
        class EntityArchetype : public ComponentStorage<TComponents>...
        {
            std::size_t entityCount = 0;

        public:
            using ArchEntity = Entity<TComponents...>;
            static constexpr std::size_t ComponentTypeCount = sizeof...(TComponents);

            template <typename T>
            [[nodiscard]]
            ComponentList<T> &GetComponents() { return static_cast<ComponentStorage<T> &>(*this).components; }
            template <typename T>
            [[nodiscard]]
            const ComponentList<T> &GetComponents() const { return static_cast<const ComponentStorage<T> &>(*this).components; }
            [[nodiscard]]
            std::size_t Count() const noexcept { return entityCount; }
            [[nodiscard]]
            bool ValidEntity(const ArchEntity entity) const noexcept { return entity >= 0 && static_cast<std::size_t>(entity) < entityCount; }

            template <typename T>
            [[nodiscard]] T *Get(const ArchEntity entity)
            {
                if (!ValidEntity(entity))
                    return nullptr;

                auto &components = GetComponents<T>();
                return &components[static_cast<std::size_t>(entity)];
            }

            bool Get(const ArchEntity entity, TComponents *&... outComponents)
            {
                ((outComponents = nullptr), ...);
                if (!ValidEntity(entity))
                    return false;

                const auto index = static_cast<std::size_t>(entity);
                ((outComponents = &static_cast<ComponentStorage<TComponents> &>(*this).components[index]), ...);

                return true;
            }

            ArchEntity Add() { return Add<TComponents...>(TComponents{}...); }
            ArchEntity Add(TComponents... component) { return Add<TComponents...>(std::move(component)...); }

            template <typename... TComps>
            Entity<TComps...> Add() { return Add<TComps...>(TComps{}...); }

            template <typename... TComps>
            Entity<TComps...> Add(TComps... component)
            {
                if (entityCount > static_cast<std::size_t>(std::numeric_limits<ArchEntity>::max()))
                    throw std::length_error("EntityArchetype has no available entity IDs");

                const auto entity = static_cast<ArchEntity>(entityCount);
                
                try
                {
                    (static_cast<ComponentStorage<TComps> &>(*this).Add(std::move(component)), ...);
                }
                catch (...)
                {
                    (static_cast<ComponentStorage<TComps> &>(*this).RollbackTo(entityCount), ...);
                    throw;
                }

                ++entityCount;                
                return entity;
            }
        };
    }
}