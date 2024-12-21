#include "CachedMediaProxy.hpp"

#include <filesystem>
#include <future>

#include "Utils.hpp"

CachedMediaProxy::CachedMediaProxy(size_t maxMB)
    : _maxCacheSize(maxMB * 1024 * 1024)
{
}

std::shared_future<CachedImage> CachedMediaProxy::getImage(const std::string& path)
{
    assert(std::filesystem::exists(path));

    if (_cached_images.count(path))
    {
        return _cached_images.at(path);
    }
    else
    {
        std::lock_guard lock(_mutex);
        std::shared_future<CachedImage> future = std::async(std::launch::async, [this, path] {
            const auto now = std::chrono::system_clock::now().time_since_epoch().count();
            CachedImage cachedImage(path, now, QImage(QString::fromStdString(path)));

            _mutex.lock();
            if (_currentCacheSize + cachedImage.getMemorySize() > _maxCacheSize)
            {
                deleteOldest();
            }
            _currentCacheSize += cachedImage.getMemorySize();
            _mutex.unlock();

            return cachedImage;
        });
        _cached_images.emplace(path, future);
        return future;
    }
}

std::shared_ptr<QMovie> CachedMediaProxy::getAnimation(const std::string& path)
{
    return std::make_shared<QMovie>(QString::fromStdString(path));
}

void CachedMediaProxy::preCacheImage(const std::string& path)
{
    if (!isImage(path))
    {
        return;
    }

    if (_cached_images.count(path))
    {
        return;
    }

    std::lock_guard lock(_mutex);

    std::shared_future<CachedImage> future = std::async(std::launch::async, [&]() -> CachedImage {
        QImage image(QString::fromStdString(path));
        time_t now = std::chrono::system_clock::now().time_since_epoch().count();
        return CachedImage(path, now, std::move(image));
    });

    _cached_images.emplace(path, std::move(future));
}

void CachedMediaProxy::notifyBigJump()
{
    for (auto& [path, future] : _cached_images)
    {
        if (future.valid())
        {
            future.wait();
        }
    }
    _cached_images = decltype(_cached_images){};
}

void CachedMediaProxy::clear()
{
    for (auto& [path, future] : _cached_images)
    {
        if (future.valid())
        {
            future.wait();
        }
    }
    _cached_images.clear();
    _currentCacheSize = 0;
}

void CachedMediaProxy::deleteOldest()
{
    const CachedImage* oldest = nullptr;
    for (const auto& [path, future] : _cached_images)
    {
        const auto& image = future.get();
        if (oldest == nullptr)
        {
            oldest = &image;
            continue;
        }

        if (image.lastAccess() < oldest->lastAccess())
        {
            oldest = &image;
        }
    }
    assert(oldest);

    _currentCacheSize -= oldest->getMemorySize();
    _cached_images.erase(oldest->path());
}