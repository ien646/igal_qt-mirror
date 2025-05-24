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
#include <ranges>

#include <omp.h>

#include "HelpOverlay.hpp"
#include "PreviewStrip.hpp"
#include "Utils.hpp"

MainWindow::MainWindow(const std::string& target_path)
{
    setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    setMinimumSize(600, 400);

    _mainWidget = new QWidget(this);
    _mediaLayout = new QStackedLayout(this);
    _mediaWidget = new MediaWidget(this);
    _imageUpscaleSelectWidget = new ListSelectWidget(getImageUpscaleModels(), this);
    _videoUpscaleSelectWidget = new ListSelectWidget(getVideoUpscaleModels(), this);
    _navigateSelectWidget = new ListSelectWidget({}, this, true);

    setCentralWidget(_mainWidget);
    _mainWidget->setStyleSheet("QWidget{background-color:#000000;}");
    _mainWidget->setLayout(_mediaLayout);
    _mediaWidget->setMinimumSize(600, 400);
    _mediaWidget->setMouseTracking(true);

    _mediaLayout->setStackingMode(QStackedLayout::StackAll);
    _mediaLayout->addWidget(_mediaWidget);
    _mediaLayout->addWidget(_imageUpscaleSelectWidget);
    _mediaLayout->addWidget(_videoUpscaleSelectWidget);
    _mediaLayout->addWidget(_navigateSelectWidget);

    _mediaLayout->setCurrentWidget(_imageUpscaleSelectWidget);
    _imageUpscaleSelectWidget->hide();
    _videoUpscaleSelectWidget->hide();
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

    connect(_imageUpscaleSelectWidget, &ListSelectWidget::itemsSelected, this, [this](const auto& selected) {
        upscaleImage(_fileList[_currentIndex].path, selected[0]);
        _imageUpscaleSelectWidget->hide();
    });

    connect(_imageUpscaleSelectWidget, &ListSelectWidget::cancelled, this, [this] {
        _imageUpscaleSelectWidget->hide();
    });

    connect(_videoUpscaleSelectWidget, &ListSelectWidget::itemsSelected, this, [this](const auto& selected) {
        upscaleVideo(_fileList[_currentIndex].path, selected[0]);
        _videoUpscaleSelectWidget->hide();
    });

    connect(_videoUpscaleSelectWidget, &ListSelectWidget::cancelled, this, [this] {
        _videoUpscaleSelectWidget->hide();
    });

    connect(_navigateSelectWidget, &ListSelectWidget::itemsSelected, this, [this](const std::vector<std::string>& selected) {
        if (selected.empty())
        {
            return;
        }
        else if (selected.size() == 1)
        {
            const auto newDir = _targetDir + "/" + selected[0];
            if (std::filesystem::exists(newDir))
            {
                navigateDir(newDir);
            }
            else
            {
                _mediaWidget->showMessage(QString::fromStdString("Target dir not found: " + newDir));
            }
            _navigateSelectWidget->hide();
        }
        else
        {
            std::vector<std::string> abs_paths;
            for (const auto& item : selected)
            {
                const auto path = _targetDir + "/" + item;
                if (std::filesystem::exists(path))
                {
                    abs_paths.push_back(path);
                }
            }

            loadFilesMulti(abs_paths);

            _currentIndex = 0;
            _mediaWidget->cachedMediaProxy().clear();
            _mediaWidget->setMedia(_fileList[_currentIndex].path);

            _navigateSelectWidget->hide();
        }
    });

    connect(_navigateSelectWidget, &ListSelectWidget::cancelled, this, [this] { _navigateSelectWidget->hide(); });

    emit currentIndexChanged(_currentIndex);

    setFocus(Qt::FocusReason::MouseFocusReason);
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

    std::thread thread([path, model, this] {
        const std::string targetpath = std::format("{}/up2_{}", ien::get_file_directory(path), ien::get_file_name(path));
        const std::string command = "realesrgan-ncnn-vulkan";
        if (!ien::exists_in_envpath(command))
        {
            _controls_disabled = false;
            return;
        }

        const auto extension = ien::str_tolower(ien::get_file_extension(path)).substr(1);

        const std::vector<std::string> args = { "-i", path, "-o", targetpath, "-n", model, "-f", extension };
        runCommand(command, args, [this](std::string text) {
            QMetaObject::invokeMethod(this, [=, this] { _mediaWidget->showMessage(QString::fromStdString(text)); });
        });

        if (!std::filesystem::exists(targetpath))
        {
            _mediaWidget->showMessage("Upscale command failed!");
            _controls_disabled = false;
            return;
        }

        QImage img(QString::fromStdString(targetpath));
        QImage scaledImg = img.scaledToWidth(img.width() / 2, Qt::TransformationMode::SmoothTransformation);
        scaledImg.save(QString::fromStdString(targetpath), extension.c_str(), 95);
        const auto mtime = ien::get_file_mtime(path);
        QFile::moveToTrash(QString::fromStdString(path));

        std::filesystem::rename(targetpath, path);
        ien::set_file_mtime(path, mtime);

        QMetaObject::invokeMethod(this, [=, this] {
            _mediaWidget->showMessage("Finished!");
            _controls_disabled = false;
            loadFiles();
            updateCurrentFileInfo();
            if (_fileList.size() > _currentIndex)
            {
                _mediaWidget->setMedia(_fileList[_currentIndex].path);
            }
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
    if (_fileList.empty())
    {
        _mediaWidget->setMedia("");
    }
    else
    {
        _mediaWidget->setMedia(_fileList[_currentIndex].path);
    }
    emit currentIndexChanged(_currentIndex);
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

    if (diag.result() == QDialog::Rejected)
    {
        return;
    }

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
    emit currentIndexChanged(_currentIndex);
}

void MainWindow::openNavigationDialog()
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

void MainWindow::handleNumpadInput(int key)
{
    switch (key)
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
    case Qt::Key_Period:
        toggleMarkCurrentFile();
        break;
    }
}

void MainWindow::handleCtrlInput(int key)
{
    switch (key)
    {
    case Qt::Key_Period:
        if (_currentMode != GalleryMode::MARKED)
        {
            filterMarkedFiles();
        }
        else
        {
            loadFiles();
        }
        break;

    case Qt::Key_Up:
        _mediaWidget->increaseVideoVolume(0.05F);
        break;

    case Qt::Key_Down:
        _mediaWidget->increaseVideoVolume(-0.05F);
        break;
    }
}

void MainWindow::handleStandardInput(int key)
{
    switch (key)
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
    case Qt::Key_Space:
        _mediaWidget->togglePlayPauseVideo();
        break;
    case Qt::Key_H:
        if (_helpOverlay)
        {
            _helpOverlay->setVisible(!_helpOverlay->isVisible());
        }
        else
        {
            _helpOverlay = new HelpOverlay(this);
            _helpOverlay->resize(size());
            connect(this, &MainWindow::resized, _helpOverlay, [this](QSize sz) { _helpOverlay->resize(sz); });
            connect(this, &MainWindow::upPressed, _helpOverlay, [this] { _helpOverlay->moveUp(); });
            connect(this, &MainWindow::downPressed, _helpOverlay, [this] { _helpOverlay->moveDown(); });
            _helpOverlay->show();
        }
        break;
    case Qt::Key_Up:
        emit upPressed();
        break;
    case Qt::Key_Down:
        emit downPressed();
        break;

    case Qt::Key_P:
        if (_previewStrip)
        {
            _previewStrip->setVisible(!_previewStrip->isVisible());
        }
        else
        {
            _previewStrip = new PreviewStrip(_mediaWidget->cachedMediaProxy(), this);
            _previewStrip->resize(size());
            connect(this, &MainWindow::resized, _previewStrip, [this](QSize sz) { _previewStrip->resize(sz); });
            connect(this, &MainWindow::currentIndexChanged, _previewStrip, [this](int64_t index) {
                std::vector<std::string> paths;
                for (int i = -2; i <= 2; ++i)
                {
                    if ((i + index) >= 0 && (i + index) < _fileList.size())
                    {
                        paths.emplace_back(_fileList[i + index].path);
                    }
                    else
                    {
                        paths.emplace_back();
                    }
                }
                _previewStrip->loadImages(paths);
            });
            _previewStrip->show();

            emit currentIndexChanged(_currentIndex);
        }
        break;
    case Qt::Key_Period:
        toggleMarkCurrentFile();
        break;
    }
}

void MainWindow::filterVideos()
{
    std::vector<FileEntry> resultList;
    size_t total = _fileList.size();
    std::atomic_size_t current = 0;

    bool task_finished = false;

    std::thread task_thread([&] {

#pragma omp parallel
        {
            std::vector<FileEntry> threadResult;
#pragma omp for
            for (long i = 0; i < _fileList.size(); ++i)
            {
                const auto& entry = _fileList[i];
                if (omp_get_thread_num() == 0)
                {
                    QMetaObject::invokeMethod(this, [&] {
                        _mediaWidget->showMessage(QString::fromStdString(std::format("{}/{}", current.load(), total)));
                    });
                }

                if (isVideo(entry.path) || isAnimation(entry.path))
                {
                    threadResult.push_back(std::move(entry));
                }
                ++current;
            }
#pragma omp critical
            std::ranges::move(threadResult, std::back_inserter(resultList));
        }
        task_finished = true;
    });

    while (!task_finished)
    {
        QGuiApplication::processEvents();
    }
    task_thread.join();

    _mediaWidget->showMessage(QString::fromStdString(std::format("Filtered {} video files", resultList.size())));

    _fileList = std::move(resultList);
    _currentIndex = 0;
    _mediaWidget->setMedia(_fileList[_currentIndex].path);
}

void MainWindow::upscaleVideo(const std::string& path, const std::string& modelStr)
{
    _controls_disabled = true;

    std::thread thread([path, modelStr, this] {
        std::string out_path = path;
        std::string target_path = std::format("{}/up2_{}", ien::get_file_directory(path), ien::get_file_name(path));
        const std::string command = "video2x";
        if (!ien::exists_in_envpath(command))
        {
            _controls_disabled = false;
            return;
        }

        const auto extension = ien::str_tolower(ien::get_file_extension(path)).substr(1);
        if (extension != ".mp4")
        {
            target_path += ".mp4";
            out_path += ".mp4";
        }

        const auto [actual_model, upscale_factor] = videoUpscaleModelToStringAndFactor(modelStr);

        const std::vector<std::string> args = {
            "--input",
            path,
            "--output",
            target_path,
            "-p",
            "realesrgan",
            "--realesrgan-model",
            actual_model,
            "-s",
            std::to_string(upscale_factor)
        };
        runCommand(command, args, [this](std::string text) {
            QMetaObject::invokeMethod(this, [=, this] { _mediaWidget->showMessage(QString::fromStdString(text)); });
        });

        if (!std::filesystem::exists(target_path) || std::filesystem::file_size(target_path) < 1024)
        {
            _mediaWidget->showMessage("Upscale command failed!");
            _controls_disabled = false;
            return;
        }

        const auto mtime = ien::get_file_mtime(path);
        QFile::moveToTrash(QString::fromStdString(path));

        std::filesystem::rename(target_path, out_path);
        ien::set_file_mtime(out_path, mtime);

        QMetaObject::invokeMethod(this, [=, this] {
            _mediaWidget->showMessage("Finished!");
            _controls_disabled = false;
            loadFiles();
            updateCurrentFileInfo();
            if (_fileList.size() > _currentIndex)
            {
                _mediaWidget->cachedMediaProxy().clear();
                _fileList[_currentIndex].path = out_path;
                _mediaWidget->setMedia(out_path);
            }
        });
    });
    thread.detach();
}

void MainWindow::toggleMarkCurrentFile()
{
    if (auto found_it = std::ranges::find(_markedFiles, _currentIndex); found_it != _markedFiles.end())
    {
        _markedFiles.erase(found_it);
        _mediaWidget->showMessage(QString::fromStdString(std::format("Unmarked file: {}", _currentIndex)));
    }
    else
    {
        _markedFiles.emplace(_currentIndex);
        _mediaWidget->showMessage(QString::fromStdString(std::format("Marked file: {}", _currentIndex)));
    }
}

void MainWindow::filterMarkedFiles()
{
    if (_markedFiles.empty())
    {
        _mediaWidget->showMessage("No marked files");
        return;
    }

    std::vector<FileEntry> markedEntries;
    for (const auto& index : _markedFiles)
    {
        markedEntries.push_back(std::move(_fileList[index]));
    }
    std::ranges::sort(markedEntries, [](const FileEntry& lhs, const FileEntry& rhs) { return lhs.mtime > rhs.mtime; });

    _fileList = std::move(markedEntries);
    _currentIndex = 0;

    _mediaWidget->cachedMediaProxy().clear();
    _mediaWidget->setMedia(_fileList[_currentIndex].path);
    _mediaWidget->showMessage("Switched to marked-mode");

    _currentMode = GalleryMode::MARKED;
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
    const int key = ev->key();

    if (ctrl && shift && !_fileList.empty())
    {
        if (key == Qt::Key_Plus)
        {
            if (_currentMode == GalleryMode::MULTI)
            {
                _mediaWidget->showMessage("Upscaling not available in multi-mode");
                return;
            }
            if (_currentMode == GalleryMode::MARKED)
            {
                _mediaWidget->showMessage("Upscaling not available in marked-mode");
                return;
            }

            if (_mediaWidget->currentMediaType() == CurrentMediaType::Image)
            {
                _imageUpscaleSelectWidget->show();
                _imageUpscaleSelectWidget->setFocus(Qt::FocusReason::MouseFocusReason);
                _mediaLayout->setCurrentWidget(_imageUpscaleSelectWidget);
            }
            else
            {
                _videoUpscaleSelectWidget->show();
                _videoUpscaleSelectWidget->setFocus(Qt::FocusReason::MouseFocusReason);
                _mediaLayout->setCurrentWidget(_videoUpscaleSelectWidget);
            }
        }
        else if (key == Qt::Key_Minus)
        {
            if (!_videoFilter)
            {
                filterVideos();
            }
            else
            {
                loadFiles();
            }
            _videoFilter = !_videoFilter;
        }
        else
        {
            processCopyToLinkKey(ev);
        }
    }
    else if (shift && key == Qt::Key_Return)
    {
        openNavigationDialog();
    }
    else if (shift)
    {
        if (key == Qt::Key_Up)
        {
            _mediaWidget->increaseVideoSpeed(0.05f);
        }
        if (key == Qt::Key_Down)
        {
            _mediaWidget->increaseVideoSpeed(-0.05f);
        }
    }
    else if (ctrl)
    {
        handleCtrlInput(key);
    }
    else if (numpad)
    {
        handleNumpadInput(key);
    }
    else
    {
        handleStandardInput(key);
    }
}

void MainWindow::resizeEvent(QResizeEvent* ev)
{
    emit resized(ev->size());
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

    _currentMode = GalleryMode::STANDARD;

    _mediaWidget->showMessage(QString::fromStdString(std::format("Loaded {} files", _fileList.size())));
}

void MainWindow::loadFilesMulti(const std::vector<std::string>& abs_directories)
{
    _mediaWidget->cachedMediaProxy().clear();
    _fileList.clear();

    const auto listToMap = [](const std::vector<FileEntry>& list) {
        std::unordered_map<std::string, FileEntry> result;
        for (const auto& item : list)
        {
            result.emplace(ien::get_file_name(item.path), item);
        }
        return result;
    };

    std::vector<std::vector<FileEntry>> dir_entry_lists;
    for (const auto& dir : abs_directories)
    {
        std::vector<FileEntry>& current_list = dir_entry_lists.emplace_back();
        for (const auto& entry :
             std::filesystem::directory_iterator(dir) | std::views::filter([](const auto& e) {
                 return e.is_regular_file() && hasMediaExtension(e.path().string());
             }))
        {
            const auto path = entry.path().string();
            const auto mtime = ien::get_file_mtime(path);
            current_list.push_back(FileEntry{ .path = path, .mtime = mtime });
        }
    }

    for (size_t i = 0; i < dir_entry_lists.size() - 1; ++i)
    {
        const std::unordered_map<std::string, FileEntry> current_list = listToMap(dir_entry_lists[i]);

        for (size_t j = i + 1; j < dir_entry_lists.size(); ++j)
        {
            const auto& other_list = listToMap(dir_entry_lists[j]);
            for (const auto& [current_name, current_entry] : current_list)
            {
                if (std::find_if(other_list.begin(), other_list.end(), [&](const auto& pair) {
                        return current_name == pair.first;
                    }) != other_list.end())
                {
                    _fileList.push_back(current_entry);
                }
            }
        }
    }

    std::sort(_fileList.begin(), _fileList.end(), [](const FileEntry& lhs, const FileEntry& rhs) {
        return lhs.mtime > rhs.mtime;
    });

    _fileList.erase(std::unique(_fileList.begin(), _fileList.end()), _fileList.end());

    _mediaWidget->showMessage("Entering multi-mode");
    _currentMode = GalleryMode::MULTI;

    _currentIndex = 0;
    emit currentIndexChanged(_currentIndex);
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
        if (_fileList.empty())
        {
            return;
        }
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