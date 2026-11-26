#pragma once

#include "Shared.h"
#include "Engine.h"

#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace Engine
{
    template <typename... TComponents>
    using Entity = Int32;

    namespace ECSArchetypes
    {
        template <typename T>
        using ComponentList = std::vector<T>;

        template <typename T>
        struct ComponentStorage
        {
            static constexpr std::size_t NoComponent = std::numeric_limits<std::size_t>::max();

            std::vector<T> components; // Dense set
            std::vector<std::size_t> entityHandles; // Sparse set
            
            void AddEntity(const std::size_t compIndex = NoComponent) { entityHandles.push_back(compIndex); }

            template <typename... TArgs>
            std::size_t AddInstance(TArgs&&... args)
            {
                components.emplace_back(std::forward<TArgs>(args)...);
                return components.size() - 1;
            }
        };

        template <typename... TComponents>
        class LooseEntityArchetype : public ComponentStorage<TComponents>...
        {
            using ArchEntity = Entity<TComponents...>;
            std::size_t entityCount = 0;

            template <typename T, typename... TComps>
            void AddEntityToStorage()
            {
                auto &storage = static_cast<ComponentStorage<T> &>(*this);
                if constexpr ((std::is_same_v<T, TComps> || ...))
                    storage.AddEntity(storage.components.size() - 1);
                else
                    storage.AddEntity();
            }

        public:
            static constexpr std::size_t ComponentTypeCount = sizeof...(TComponents);

            [[nodiscard]]
            bool ValidEntity(const ArchEntity entity) const noexcept { return entity >= 0 && static_cast<std::size_t>(entity) < entityCount; }
            template <typename T>
            [[nodiscard]]
            const ComponentList<T> &GetComponents() const { return static_cast<const ComponentStorage<T> &>(*this).components; }
            [[nodiscard]]
            std::size_t Count() const noexcept { return entityCount; }

            template <typename T>
            [[nodiscard]] T *Get(const ArchEntity entity)
            {
                if (!ValidEntity(entity))
                    return nullptr;

                auto &storage = static_cast<ComponentStorage<T> &>(*this);
                const std::size_t entityIndex = static_cast<std::size_t>(entity);
                if (entityIndex >= storage.entityHandles.size())
                    return nullptr;

                const std::size_t componentIndex = storage.entityHandles[entityIndex];
                if (componentIndex == ComponentStorage<T>::NoComponent ||
                    componentIndex >= storage.components.size())
                    return nullptr;

                return &storage.components[componentIndex];
            }

            template <typename... TComps>
            bool Get(const ArchEntity entity, TComps *&... outComponents)
            {
                ((outComponents = nullptr), ...);

                if (!ValidEntity(entity))
                    return false;

                ((outComponents = Get<TComps>(entity)), ...);

                return true;
            }

            ArchEntity Create() { return Create<TComponents...>(TComponents{}...); }
            ArchEntity Create(TComponents... component) { return Create<TComponents...>(std::move(component)...); }

            template <typename... TComps>
            Entity<TComps...> Create() { return Create<TComps...>(TComps{}...); }

            template <typename... TComps>
            Entity<TComps...> Create(TComps... component)
            {
                if (entityCount > static_cast<std::size_t>(std::numeric_limits<ArchEntity>::max()))
                    throw std::length_error("EntityArchetype has no available entity IDs");

                const auto entity = static_cast<ArchEntity>(entityCount);
            
                (static_cast<ComponentStorage<TComps> &>(*this).AddInstance(std::move(component)), ...);
                (AddEntityToStorage<TComponents, TComps...>(), ...);

                ++entityCount;                
                return entity;
            }
        };
    }
}
