#pragma once

class IResourceOwner
{
protected:
    IResourceOwner() = default;
    IResourceOwner(IResourceOwner &&) = default;
    IResourceOwner &operator=(IResourceOwner &&) = default;

public:
    IResourceOwner(const IResourceOwner &) = delete;
    IResourceOwner &operator=(const IResourceOwner &) = delete;

    virtual ~IResourceOwner() = default;
    virtual void ReleaseResources() noexcept = 0;
    virtual bool InitializeResources(bool reinitialize = false) = 0;
};