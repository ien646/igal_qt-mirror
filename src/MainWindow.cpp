#include "MainWindow.hpp"

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>

#include <ien/container_utils.hpp>
#include <ien/fs_utils.hpp>
#include <ien/io_utils.hpp>
#include <ien/platform.hpp>
#include <ien/str_utils.hpp>

#include <algorithm>
#include <filesystem>

#include "Utils.hpp"

MainWindow::MainWindow(const std::string& target_path)
{
    setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    setMinimumSize(600, 400);

    _mainWidget = new QWidget(this);
    _mediaLayout = new QStackedLayout(this);
    _mediaWidget = new MediaWidget(this);
    _upscaleSelectWidget = new ListSelectWidget(getUpscaleModels(), this);
    _navigateSelectWidget = new ListSelectWidget({}, this);

    setCentralWidget(_mainWidget);
    _mainWidget->setStyleSheet("QWidget{background-color:#000000;}");
    _mainWidget->setLayout(_mediaLayout);
    _mediaWidget->setMinimumSize(600, 400);
    _mediaWidget->setMouseTracking(true);

    _mediaLayout->setStackingMode(QStackedLayout::StackAll);
    _mediaLayout->addWidget(_mediaWidget);
    _mediaLayout->addWidget(_upscaleSelectWidget);
    _mediaLayout->addWidget(_navigateSelectWidget);

    _mediaLayout->setCurrentWidget(_upscaleSelectWidget);
    _upscaleSelectWidget->hide();
    _navigateSelectWidget->hide();

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
    std::vector<std::string> linksList;
    for (const auto& [key, dir] : _links)
    {
        linksList.push_back(dir);
    }
    _navigateSelectWidget->setItems(linksList);

    connect(this, &MainWindow::currentIndexChanged, this, [this] { updateCurrentFileInfo(); });

    connect(_upscaleSelectWidget, &ListSelectWidget::itemSelected, this, [this](const std::string& selected) {
        upscaleImage(_fileList[_currentIndex].path, selected);
        _upscaleSelectWidget->hide();
    });

    connect(_upscaleSelectWidget, &ListSelectWidget::cancelled, this, [this] { _upscaleSelectWidget->hide(); });

    connect(_navigateSelectWidget, &ListSelectWidget::itemSelected, this, [this](const std::string& selected) {
        const auto newDir = _targetDir + "/" + selected;
        if (std::filesystem::exists(newDir))
        {
            navigateDir(newDir);
        }
        else
        {
            _mediaWidget->showMessage(QString::fromStdString("Target dir not found: " + newDir));
        }
        _navigateSelectWidget->hide();
    });

    connect(_navigateSelectWidget, &ListSelectWidget::cancelled, this, [this] { _navigateSelectWidget->hide(); });
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
    for (int i = 1; i <= SURROUNDING_PRECACHE_WINDOW; ++i)
    {
        const int64_t prevIndex = _currentIndex - i;
        const int64_t nextIndex = _currentIndex + i;

        if (prevIndex > 0)
        {
            _mediaWidget->cachedMediaProxy().preCacheImage(_fileList[prevIndex].path);
        }
        if (nextIndex < _fileList.size())
        {
            _mediaWidget->cachedMediaProxy().preCacheImage(_fileList[nextIndex].path);
        }
    }
}

void MainWindow::upscaleImage(const std::string& path, const std::string& model)
{
    _controls_disabled = true;

    std::thread thread([=, this] {
        const std::string targetpath = std::format("{}/up2_{}", ien::get_file_directory(path), ien::get_file_name(path));
        const std::string command = "realesrgan-ncnn-vulkan";
        const std::vector<std::string> args = { "-i", path, "-o", targetpath, "-n", model, "-f", "jpg" };
        runCommand(command, args, [this](std::string text) {
            QMetaObject::invokeMethod(this, [=, this] { _mediaWidget->showMessage(QString::fromStdString(text)); });
        });
        
        QImage img(QString::fromStdString(targetpath));
        QImage scaledImg = img.scaledToWidth(img.width() / 2, Qt::TransformationMode::SmoothTransformation);
        scaledImg.save(QString::fromStdString(targetpath), "JPG", 95);
        const auto mtime = ien::get_file_mtime(path);
        QFile::moveToTrash(QString::fromStdString(path));
        const auto finalPath = path.ends_with(".jpg") ? path : path + ".jpg";

        std::filesystem::rename(targetpath, finalPath);
        ien::set_file_mtime(finalPath, mtime);

        QMetaObject::invokeMethod(this, [=, this] {     
            _mediaWidget->showMessage("Finished!");
            _controls_disabled = false;
            loadFiles();
            updateCurrentFileInfo();
        });
    });
    thread.detach();
}

void MainWindow::navigateDir(const std::string& path)
{
    _targetDir = path;
    loadFiles();
    _currentIndex = 0;
    _mediaWidget->cachedMediaProxy().clear();
    _mediaWidget->setMedia(_fileList[_currentIndex].path);
}

void MainWindow::deleteFile(const std::string& path)
{
    using StandardButton = QMessageBox::StandardButton;

    QMessageBox msgbox;
    msgbox.setWindowTitle("Confirm");
    msgbox.setText("Send file to trash?");
    msgbox.setStandardButtons(StandardButton::Yes | StandardButton::No);
    msgbox.setDefaultButton(StandardButton::No);
    msgbox.setIcon(QMessageBox::Icon::Question);

    const auto button = static_cast<StandardButton>(msgbox.exec());
    if (button == StandardButton::Yes)
    {
        QFile(QString::fromStdString(_fileList[_currentIndex].path)).moveToTrash();
        _fileList.erase(_fileList.begin() + _currentIndex);
        if (_fileList.empty())
        {
            _mediaWidget->setMedia("");
        }
        _currentIndex = ien::clamp_index_to_vector_bounds(_fileList, _currentIndex);
        _mediaWidget->setMedia(_fileList[_currentIndex].path);
        emit currentIndexChanged(_currentIndex);
        preCacheSurroundings();
    }
}

void MainWindow::openDir()
{
    QFileDialog diag(this, "Open directory");
    diag.setFileMode(QFileDialog::FileMode::Directory);
    diag.setAcceptMode(QFileDialog::AcceptMode::AcceptOpen);
    diag.setViewMode(QFileDialog::ViewMode::Detail);

    diag.exec();

    const auto dir = diag.directory();
    if (std::filesystem::exists(dir.filesystemPath()))
    {
        _targetDir = dir.filesystemPath().string();
        loadFiles();
        _currentIndex = 0;
        if (_fileList.empty())
        {
            _mediaWidget->setMedia("");
        }
        else
        {
            _mediaWidget->setMedia(_fileList[_currentIndex].path);
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent* ev)
{
    if (_controls_disabled)
    {
        return;
    }

    const auto ctrl = ev->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier);
    const auto shift = ev->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier);
    const auto alt = ev->modifiers().testFlag(Qt::KeyboardModifier::AltModifier);
    const auto numpad = ev->modifiers().testFlag(Qt::KeyboardModifier::KeypadModifier);
    
    if (ctrl && shift && !_fileList.empty())
    {
        if (ev->key() == Qt::Key_Plus)
        {
            _upscaleSelectWidget->show();
            _upscaleSelectWidget->setFocus(Qt::FocusReason::MouseFocusReason);
            _mediaLayout->setCurrentWidget(_upscaleSelectWidget);
        }
        else
        {
            processCopyToLinkKey(ev);
        }
    }
    else if (shift && ev->key() == Qt::Key_Return)
    {
        std::vector<std::string> items;
        for (const auto& [key, dir] : _links)
        {
            const auto linkDir = _targetDir + "/" + dir;
            if (std::filesystem::exists(linkDir))
            {
                items.push_back(ien::str_split(linkDir, "/").back());
            }
        }
        if (items.empty())
        {
            items.push_back("..");
        }

        std::sort(items.begin(), items.end());
        _navigateSelectWidget->setItems(items);
        _navigateSelectWidget->show();
        _navigateSelectWidget->setFocus(Qt::FocusReason::MouseFocusReason);
        _mediaLayout->setCurrentWidget(_navigateSelectWidget);
    }
    else if (numpad)
    {
        switch (ev->key())
        {
        case Qt::Key_4:
            _mediaWidget->translateLeft(0.2f);
            break;
        case Qt::Key_6:
            _mediaWidget->translateRight(0.2f);
            break;
        case Qt::Key_2:
            _mediaWidget->translateUp(0.2f);
            break;
        case Qt::Key_8:
            _mediaWidget->translateDown(0.2f);
            break;
        case Qt::Key_Plus:
            _mediaWidget->zoomIn(0.25f);
            break;
        case Qt::Key_Minus:
            _mediaWidget->zoomOut(0.25f);
            break;
        case Qt::Key_0:
            _mediaWidget->resetTransform();
            break;
        }
    }
    else
    {
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
        case Qt::Key_R:
            _mediaWidget->cachedMediaProxy().notifyBigJump();
            randomEntry();
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
        case Qt::Key_Delete:
            deleteFile(_fileList[_currentIndex].path);
            break;
        case Qt::Key_O:
            openDir();
            break;
        }
    }
}

void MainWindow::loadFiles()
{
    _mediaWidget->cachedMediaProxy().clear();
    _fileList.clear();
    for (const auto& entry : std::filesystem::directory_iterator(_targetDir))
    {
        if (entry.is_regular_file() && hasMediaExtension(entry.path().string()))
        {
            const auto path = entry.path().string();
            const auto mtime = ien::get_file_mtime(path);
            _fileList.push_back(FileEntry{ .path = path, .mtime = mtime });
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

void MainWindow::randomEntry()
{
    if (_fileList.empty())
    {
        return;
    }

    _currentIndex = rand() % _fileList.size();
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