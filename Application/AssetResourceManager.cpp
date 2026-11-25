#include "AssetResourceManager.h"

void AssetResourceManager::PollPendingResources()
{
    LOG_SCOPE;

    auto &shaderRegistry = GetAssetRegistry<Graphics::Shader>();
    Graphics::PollResourceLoadQueue(shaderRegistry.loadQueue, shaderRegistry.resources,
        shaderRegistry.names, shaderRegistry.indexMap, "shader");

    auto &meshRegistry = GetAssetRegistry<Graphics::Mesh>();
    Graphics::PollResourceLoadQueue(meshRegistry.loadQueue, meshRegistry.resources,
        meshRegistry.names, meshRegistry.indexMap, "mesh");

    auto &textureRegistry = GetAssetRegistry<Graphics::Texture2D>();
    Graphics::PollResourceLoadQueue(textureRegistry.loadQueue, textureRegistry.resources,
        textureRegistry.names, textureRegistry.indexMap, "texture");
}
