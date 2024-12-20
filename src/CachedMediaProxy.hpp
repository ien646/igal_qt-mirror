#pragma once

#include <QImage>
#include <QMovie>

#include <future>
#include <memory>
#include <unordered_map>

class CachedImage
{
public:
    CachedImage(const std::string& path, time_t lastAccess, QImage&& data)
        : _path(path)
        , _lastAccess(lastAccess)
        , _data(std::make_shared<QImage>(std::move(data)))
    {
    }

    CachedImage(const CachedImage&) = delete;
    CachedImage(CachedImage&&) = default;

    inline void ping() { _lastAccess = std::chrono::system_clock::now().time_since_epoch().count(); }
    inline std::shared_ptr<QImage> image() const { return _data; }
    inline auto getMemorySize() const { return _data->sizeInBytes(); }
    inline time_t lastAccess() const { return _lastAccess; }
    inline const std::string& path() const { return _path; }

private:
    std::string _path;
    time_t _lastAccess;
    std::shared_ptr<QImage> _data;
};

class CachedMediaProxy
{
public:
    CachedMediaProxy(size_t maxMB = 256);

    std::shared_future<CachedImage> getImage(const std::string& path);
    std::shared_ptr<QMovie> getAnimation(const std::string& path);

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