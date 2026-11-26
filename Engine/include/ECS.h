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

        public:
            static constexpr std::size_t ComponentTypeCount = sizeof...(TComponents);

            [[nodiscard]]
            bool ValidEntity(const EntityId entity) const noexcept { return entity >= 0 && static_cast<std::size_t>(entity) < entityCount; }
            template <typename T>
            [[nodiscard]]
            ComponentList<T> &GetComponents() { return static_cast<ComponentStorage<T> &>(*this).instances; }
            [[nodiscard]]
            std::size_t Count() const noexcept { return entityCount; }

            template <typename T>
            [[nodiscard]] T *Get(const EntityId entity)
            {
                if (!ValidEntity(entity))
                    return nullptr;

                const auto index = static_cast<std::size_t>(entity);
                return &static_cast<ComponentStorage<T> &>(*this).instances[index];
            }

            template <typename... TComps>
            bool Get(const EntityId entity, TComps *&... outComponent)
            {
                if (!ValidEntity(entity))
                    return false;

                ((outComponent = Get<TComps>(entity)), ...);
                return true;
            }

            EntityId Create() { return Create(TComponents{}...); }
            EntityId Create(const TComponents &... component)
            {
                if (entityCount > static_cast<std::size_t>(std::numeric_limits<EntityId>::max()))
                    throw std::length_error("EntityArchetype has no available entity IDs");

                const auto entity = static_cast<EntityId>(entityCount);
                (static_cast<ComponentStorage<TComponents> &>(*this).AddInstance(std::move(component)), ...);

                ++entityCount;                
                return entity;
            }

            struct Entity
            {
                const EntityId id;
                Archetype &manager;

                using CreationCallback = std::function<void(TComponents &...)>;
                using EntityCreationCallback = std::function<void(Entity &)>;

                static Entity Create(Archetype &manager, const TComponents &... component) { return Entity{manager.Create(component...), manager}; }
                static Entity Create(Archetype &manager) { return Entity{manager.Create(), manager}; }

                static Entity Create(Archetype &manager, const CreationCallback &callback)
                {
                    auto entity = Entity{manager.Create(), manager};
                    callback(*manager.Get<TComponents>(entity.id)...);

                    return entity;
                }

                static Entity Create(Archetype &manager, const EntityCreationCallback &callback)
                {
                    auto entity = Entity{manager.Create(), manager};
                    callback(entity);

                    return entity;
                }

                template <typename T>
                [[nodiscard]] T *GetComponent() { return manager.Get<T>(id); }
                template <typename... TComps>
                void GetComponents(TComps *&... outComponent) { manager.Get(id, outComponent...); }
            };
        };
    }
}

#define ECS_ARCHETYPE(ARCHETYPE_NAME, ENTITY_NAME, ...) \
    using ARCHETYPE_NAME = Engine::ECSArchetypes::EntityArchetype<__VA_ARGS__>; \
    using ENTITY_NAME = ARCHETYPE_NAME::Entity
