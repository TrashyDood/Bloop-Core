#include "AssetResourceManager.h"

void AssetResourceManager::PollPendingResources()
{
    LOG_SCOPE;

    auto &shaderRegistry = GetAssetRegistry<Graphics::ShaderResource>();
    Graphics::PollResourceLoadQueue(shaderRegistry.loadQueue, shaderRegistry.resources,
        shaderRegistry.names, shaderRegistry.indexMap, "shader");

    auto &meshRegistry = GetAssetRegistry<Graphics::MeshResource>();
    Graphics::PollResourceLoadQueue(meshRegistry.loadQueue, meshRegistry.resources,
        meshRegistry.names, meshRegistry.indexMap, "mesh");

    auto &textureRegistry = GetAssetRegistry<Graphics::Texture2DResource>();
    Graphics::PollResourceLoadQueue(textureRegistry.loadQueue, textureRegistry.resources,
        textureRegistry.names, textureRegistry.indexMap, "texture");
}
