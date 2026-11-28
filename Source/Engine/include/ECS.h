#pragma once

#include "Shared.h"
#include "Engine.h"

#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>
#include <functional>

namespace Engine
{
    using EntityId = Int32;

    namespace ECSArchetypes
    {
        template <typename T>
        using List = std::vector<T>;
        template <typename T>
        using ComponentList = List<T>;

        template <typename T>
        struct ComponentStorage
        {
            ComponentList<T> instances;

            template <typename... TArgs>
            void AddInstance(TArgs&&... args) { instances.emplace_back(std::forward<TArgs>(args)...); }
        };

        template <typename... TComponents>
        class EntityArchetype : public ComponentStorage<TComponents>...
        {
            using Archetype = EntityArchetype<TComponents...>;
            std::size_t entityCount = 0;

            template <typename... TComps>
            void GetUnchecked(const EntityId entity, TComps *&...outComponent)
            {
                const auto index = static_cast<std::size_t>(entity);
                ((outComponent = &static_cast<ComponentStorage<TComps> &>(*this).instances[index]), ...);
            }

        public:
            static constexpr std::size_t ComponentTypeCount = sizeof...(TComponents);

            [[nodiscard]]
            bool ValidEntity(const EntityId entity) const noexcept { return entity >= 0 && static_cast<std::size_t>(entity) < entityCount; }
            template <typename T>
            [[nodiscard]]
            ComponentList<T> &GetComponentList() { return static_cast<ComponentStorage<T> &>(*this).instances; }
            [[nodiscard]]
            std::size_t Count() const noexcept { return entityCount; }

            template <typename... TComps>
            bool Get(const EntityId entity, TComps *&...outComponent)
            {
                if (!ValidEntity(entity))
                    return false;

                GetUnchecked(entity, outComponent...);
                return true;
            }

            EntityId Create() { return Create(TComponents{}...); }
            EntityId Create(const TComponents &... component)
            {
                if (entityCount >= static_cast<std::size_t>(std::numeric_limits<EntityId>::max()))
                    throw std::length_error("EntityArchetype has no available entity IDs");

                const auto entity = static_cast<EntityId>(entityCount);
                (static_cast<ComponentStorage<TComponents> &>(*this).AddInstance(std::move(component)), ...);

                ++entityCount;                
                return entity;
            }

            EntityId Create(const std::function<void(Archetype &archetype, const EntityId)> &callback)
            {
                const EntityId entity = Create();
                callback(*this, entity);
                return entity;
            }
        };
    }
}
