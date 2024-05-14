#include "CachedMediaProxy.hpp"

#include <filesystem>

#include "Utils.hpp"

CachedMediaProxy::CachedMediaProxy(size_t maxMB)
    : _maxCacheSize(maxMB * 1024 * 1024)
{
}

std::shared_ptr<QImage> CachedMediaProxy::getImage(const std::string& path)
{
    assert(std::filesystem::exists(path));

    if(_cached_images.count(path))
    {
        return _cached_images.at(path).get();
    }

    if(_future_images.count(path))
    {
        auto& future = _future_images.at(path);
        future.wait();
        CachedImage cachedImage = future.get();
        std::shared_ptr<QImage> result = cachedImage.get();
        _future_images.erase(cachedImage.path());

        if(_currentCacheSize + cachedImage.getMemorySize() > _maxCacheSize)
        {
            deleteOldest();
        }
        _currentCacheSize += cachedImage.getMemorySize();

        _cached_images.emplace(cachedImage.path(), std::move(cachedImage));
        return result;
    }

    const auto now = std::chrono::system_clock::now().time_since_epoch().count();
    CachedImage cachedImage(path, now, QImage(QString::fromStdString(path)));

    if(_currentCacheSize + cachedImage.getMemorySize() > _maxCacheSize)
    {
        deleteOldest();
    }
    _currentCacheSize += cachedImage.getMemorySize();

    auto result = cachedImage.get();
    _cached_images.emplace(path, std::move(cachedImage));
    return result;
}

std::shared_ptr<QMovie> CachedMediaProxy::getAnimation(const std::string& path)
{
    return std::make_shared<QMovie>(QString::fromStdString(path));
}

void CachedMediaProxy::preCacheImage(const std::string& path)
{
    if(!isImage(path))
    {
        return;
    }
    
    if(_cached_images.count(path) || _future_images.count(path))
    {
        return;
    }

    std::future<CachedImage> future = std::async(std::launch::async, [&] -> CachedImage {
        QImage image(QString::fromStdString(path));
        time_t now = std::chrono::system_clock::now().time_since_epoch().count();
        return CachedImage(path, now, std::move(image));
    });

    _future_images.emplace(path, std::move(future));
}

void CachedMediaProxy::notifyBigJump()
{
    std::thread clearThread([currentFutures = std::move(_future_images)]{
        for(auto& [path, future] : currentFutures)
        {
            future.wait();
        }
    });
    clearThread.detach();
    _future_images = decltype(_future_images){ };
}

void CachedMediaProxy::deleteOldest()
{
    const CachedImage* oldest = nullptr;
    for(const auto& [path, image] : _cached_images)
    {
        if(oldest == nullptr)
        {
            oldest = &image;
            continue;
        }

        if(image.lastAccess() < oldest->lastAccess())
        {
            oldest = &image;
        }
    }
    assert(oldest);

    _cached_images.erase(oldest->path());
}