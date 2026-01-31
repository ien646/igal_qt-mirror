#pragma once

#include <QImage>
#include <QMovie>

#include <future>
#include <memory>
#include <unordered_map>
#include <utility>

class CachedImage
{
public:
    CachedImage(std::string  path, const time_t lastAccess, QImage&& data)
        : _path(std::move(path))
        , _lastAccess(lastAccess)
        , _data(std::make_shared<QImage>(std::move(data)))
    {
    }

    CachedImage(const CachedImage&) = delete;
    CachedImage(CachedImage&&) = default;

    void ping() { _lastAccess = std::chrono::system_clock::now().time_since_epoch().count(); }
    std::shared_ptr<QImage> image() const { return _data; }
    auto getMemorySize() const { return _data->sizeInBytes(); }
    time_t lastAccess() const { return _lastAccess; }
    const std::string& path() const { return _path; }

private:
    std::string _path;
    time_t _lastAccess;
    std::shared_ptr<QImage> _data;
};

class CachedMediaProxy
{
public:
    explicit CachedMediaProxy(size_t maxMB = 256);

    std::shared_future<CachedImage> getImage(const std::string& path);
    static std::shared_ptr<QMovie> getAnimation(const std::string& path);

    void preCacheImage(const std::string& path);
    void notifyBigJump();

    void clear();

private:
    size_t _maxCacheSize;
    size_t _currentCacheSize = 0;
    std::unordered_map<std::string, std::shared_future<CachedImage>> _cached_images;
    std::mutex _mutex;

    void deleteOldest();
};