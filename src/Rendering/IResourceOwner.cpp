#include "Rendering/IResourceOwner.h"
#include "Shared/Shared.h"

bool Rendering::InitializeResources(IResourceOwner *owners[], std::size_t count)
{
    if (count != 0 && owners == nullptr)
    {
        LOG_ERROR("Invalid resource owner array.\n");
        return false;
    }

    for (std::size_t i = 0; i < count; ++i)
        if (owners[i] == nullptr || !owners[i]->InitializeResources())
        {
            LOG_ERROR("Failed to initialize resources for resource owner at index %zu.\n", i);
            return false;
        }

    return true;
}
