#include "ResourceOwner.h"
#include "LogHandler.h"
#include "Shared.h"

bool Graphics::ResourceOwner::InitializeResources()
{
    LOG_SCOPE;
    if (IsInitialized())
        LOG_INFO("Reinitializing resources.\n");

    ReleaseResources();
    return true;
}
