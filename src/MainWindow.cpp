#include "MainWindow.hpp"

#include <ien/fs_utils.hpp>
#include <ien/io_utils.hpp>
#include <ien/platform.hpp>
#include <ien/str_utils.hpp>

#include <filesystem>

#include "Utils.hpp"

MainWindow::MainWindow(const std::string& target_path)
{
    setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    setMinimumSize(600, 400);

    _mainWidget = new QWidget(this);
    setCentralWidget(_mainWidget);
    _mainWidget->setStyleSheet("QWidget{background-color:#000000;}");

    _mediaLayout = new QStackedLayout(this);
    _mainWidget->setLayout(_mediaLayout);

    _mediaWidget = new MediaWidget(this);
    _mediaWidget->setMinimumSize(600, 400);
    _mediaWidget->setMouseTracking(true);
    _mediaLayout->addWidget(_mediaWidget);

    std::string targetFile;
    if (std::filesystem::is_directory(target_path))
    {
        _targetDir = target_path;
        for (const auto& entry : std::filesystem::directory_iterator(target_path))
        {
            if (entry.is_regular_file())
            {
                const std::string file = entry.path().string();
                targetFile = file;
            }
        }
    }
    else if (std::filesystem::is_regular_file(target_path))
    {
        targetFile = target_path;
        _targetDir = ien::get_file_directory(targetFile);
    }
    else
    {
        throw std::logic_error(std::format("Attempt to load invalid path: {}", target_path));
    }

    loadFiles();
    const auto it = std::find_if(_fileList.cbegin(), _fileList.cend(), [&](const FileEntry& entry) {
        return entry.path == targetFile;
    });
    if (it == _fileList.cend())
    {
        _currentIndex = 0;
        _mediaWidget->setMedia("");
    }
    else
    {
        _currentIndex = it - _fileList.begin();
        _mediaWidget->setMedia(targetFile);
    }
    preCacheSurroundings();

    loadLinks();

    connect(this, &MainWindow::currentIndexChanged, this, [this] { updateCurrentFileInfo(); });
}

void MainWindow::processCopyToLinkKey(QKeyEvent* ev)
{
    for (const auto& [key, dir] : _links)
    {
        if (ev->key() == key)
        {
            auto copyResult = copyFileToLinkDir(_fileList[_currentIndex].path, dir);
            switch (copyResult)
            {
            case CopyFileToLinkDirResult::CreatedNew:
                _mediaWidget->showMessage("Copy succesful! (new)");
                break;
            case CopyFileToLinkDirResult::Overwriten:
                _mediaWidget->showMessage("Copy succesful! (overwrite)");
                break;
            case CopyFileToLinkDirResult::SourceFileNotFound:
                _mediaWidget->showMessage("Copy failed! (source not found)");
                break;
            case CopyFileToLinkDirResult::TargetDirNotFound:
                _mediaWidget->showMessage("Copy failed! (target dir not found)");
                break;
            case CopyFileToLinkDirResult::Error:
                _mediaWidget->showMessage("Copy failed! (error)");
                break;
            }
        }
    }
}

void MainWindow::preCacheSurroundings()
{
    // Precache surroundings
    constexpr int SURROUNDING_PRECACHE_WINDOW = 3;
    for(int i = 1; i <= SURROUNDING_PRECACHE_WINDOW; ++i)
    {
        const int64_t prevIndex = _currentIndex - i;
        const int64_t nextIndex = _currentIndex + i;

        if(prevIndex > 0)
        {
            _mediaWidget->cachedMediaProxy().preCacheImage(_fileList[prevIndex].path);
        }
        if(nextIndex < _fileList.size())
        {
            _mediaWidget->cachedMediaProxy().preCacheImage(_fileList[nextIndex].path);
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent* ev)
{
    const auto ctrl = ev->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier);
    const auto shift = ev->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier);
    const auto alt = ev->modifiers().testFlag(Qt::KeyboardModifier::AltModifier);

    switch (ev->key())
    {
    case Qt::Key_Left:
        prevEntry();
        break;
    case Qt::Key_Right:
        nextEntry();
        break;
    case Qt::Key_PageUp:
        _mediaWidget->cachedMediaProxy().notifyBigJump();
        prevEntry(10);
        break;
    case Qt::Key_PageDown:
        _mediaWidget->cachedMediaProxy().notifyBigJump();
        nextEntry(10);
        break;
    case Qt::Key_Home:
        _mediaWidget->cachedMediaProxy().notifyBigJump();
        firstEntry();
        break;
    case Qt::Key_End:
        _mediaWidget->cachedMediaProxy().notifyBigJump();
        lastEntry();
        break;
    case Qt::Key_F:
        toggleFullScreen();
        break;
    case Qt::Key_I:
        toggleCurrentFileInfo();
        break;
    case Qt::Key_M:
        _mediaWidget->toggleMute();
        break;
    }

    if (ctrl && shift && !_fileList.empty())
    {
        processCopyToLinkKey(ev);
    }
}

void MainWindow::loadFiles()
{
    _fileList.clear();
    for (const auto& entry : std::filesystem::directory_iterator(_targetDir))
    {
        if (entry.is_regular_file() && hasMediaExtension(entry.path()))
        {
            const auto path = entry.path();
            const auto mtime = ien::get_file_mtime(path);
            _fileList.push_back(FileEntry{ path, mtime });
        }
    }

    std::sort(_fileList.begin(), _fileList.end(), [](const FileEntry& lhs, const FileEntry& rhs) {
        return lhs.mtime > rhs.mtime;
    });
}

void MainWindow::nextEntry(int times)
{
    if (_fileList.empty())
    {
        return;
    }

    _currentIndex += times;
    if (_currentIndex >= _fileList.size())
    {
        _currentIndex = _fileList.size() - 1;
    }
    _mediaWidget->setMedia(_fileList[_currentIndex].path);
    emit currentIndexChanged(_currentIndex);   

    preCacheSurroundings(); 
}

void MainWindow::prevEntry(int times)
{
    if (_fileList.empty())
    {
        return;
    }

    _currentIndex -= times;
    if (_currentIndex < 0)
    {
        _currentIndex = 0;
    }
    _mediaWidget->setMedia(_fileList[_currentIndex].path);
    emit currentIndexChanged(_currentIndex);

    preCacheSurroundings();
}

void MainWindow::firstEntry()
{
    if (_fileList.empty())
    {
        return;
    }
    _currentIndex = 0;
    _mediaWidget->setMedia(_fileList[_currentIndex].path);
    emit currentIndexChanged(_currentIndex);

    preCacheSurroundings();
}

void MainWindow::lastEntry()
{
    if (_fileList.empty())
    {
        return;
    }
    _currentIndex = _fileList.size() - 1;
    _mediaWidget->setMedia(_fileList[_currentIndex].path);
    emit currentIndexChanged(_currentIndex);

    preCacheSurroundings();
}

void MainWindow::toggleFullScreen()
{
    if (isFullScreen())
    {
        showNormal();
    }
    else
    {
        showFullScreen();
    }
}

void MainWindow::loadLinks()
{
    auto path = ien::get_current_user_homedir();
    if (!path.ends_with("/") & !path.ends_with("\\"))
    {
        path += std::filesystem::path::preferred_separator;
    }
    path += ".config/igal_qt/links.txt";
    _links = getLinksFromFile(path);
}

void MainWindow::toggleCurrentFileInfo()
{
    if (_mediaWidget->isInfoShown())
    {
        _mediaWidget->hideInfo();
    }
    else
    {
        std::string info = getFileInfoString(_fileList[_currentIndex].path, _mediaWidget->currentMediaSource());
        _mediaWidget->showInfo(QString::fromStdString(info));
    }
}

void MainWindow::updateCurrentFileInfo()
{
    if (_mediaWidget->isInfoShown())
    {
        std::string info = getFileInfoString(_fileList[_currentIndex].path, _mediaWidget->currentMediaSource());
        _mediaWidget->showInfo(QString::fromStdString(info));
    }
}