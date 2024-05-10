#include "MainWindow.hpp"

#include <ien/fs_utils.hpp>

#include <filesystem>

#include "Utils.hpp"

MainWindow::MainWindow(const std::string& target_path)
{
    setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    setMinimumSize(600, 400);

    _main_widget = new QWidget(this);
    setCentralWidget(_main_widget);
    _main_widget->setStyleSheet("QWidget{background-color:#000000;}");

    _media_layout = new QStackedLayout(this);
    _main_widget->setLayout(_media_layout);

    _media_widget = new MediaWidget(this);
    _media_widget->setMinimumSize(600, 400);
    _media_widget->setMouseTracking(true);
    _media_layout->addWidget(_media_widget);

    std::string target_file;
    if (std::filesystem::is_directory(target_path))
    {
        _target_dir = target_path;
        for (const auto& entry : std::filesystem::directory_iterator(target_path))
        {
            if (entry.is_regular_file())
            {
                const std::string file = entry.path().string();
                target_file = file;
            }
        }
    }
    else if (std::filesystem::is_regular_file(target_path))
    {
        target_file = target_path;
        _target_dir = ien::get_file_directory(target_file);
    }
    else
    {
        throw std::logic_error(std::format("Attempt to load invalid path: {}", target_path));
    }

    loadFiles();
    const auto it = std::find_if(_file_list.cbegin(), _file_list.cend(), [&](const FileEntry& entry) {
        return entry.path == target_file;
    });
    if (it == _file_list.cend())
    {
        _current_index = 0;
        _media_widget->setMedia("");
    }
    else
    {
        _current_index = it - _file_list.begin();
        _media_widget->setMedia(target_file);
    }
}

void MainWindow::keyPressEvent(QKeyEvent* ev)
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
        prevEntry(10);
        break;
    case Qt::Key_PageDown:
        nextEntry(10);
        break;
    case Qt::Key_Home:
        firstEntry();
        break;
    case Qt::Key_End:
        lastEntry();
        break;
    case Qt::Key_F:
        toggleFullScreen();
        break;
    }
}

void MainWindow::loadFiles()
{
    _file_list.clear();
    for (const auto& entry : std::filesystem::directory_iterator(_target_dir))
    {
        if (entry.is_regular_file() && hasMediaExtension(entry.path()))
        {
            const auto path = entry.path();
            const auto mtime = ien::get_file_mtime(path);
            _file_list.push_back(FileEntry{ path, mtime });
        }
    }

    std::sort(_file_list.begin(), _file_list.end(), [](const FileEntry& lhs, const FileEntry& rhs) {
        return lhs.mtime > rhs.mtime;
    });
}

void MainWindow::nextEntry(int times)
{
    if (_file_list.empty())
    {
        return;
    }

    _current_index += times;
    if (_current_index >= _file_list.size())
    {
        _current_index = _file_list.size() - 1;
    }
    _media_widget->setMedia(_file_list[_current_index].path);
}

void MainWindow::prevEntry(int times)
{
    if (_file_list.empty())
    {
        return;
    }

    _current_index -= times;
    if (_current_index < 0)
    {
        _current_index = 0;
    }
    _media_widget->setMedia(_file_list[_current_index].path);
}

void MainWindow::firstEntry()
{
    if (_file_list.empty())
    {
        return;
    }
    _current_index = 0;
    _media_widget->setMedia(_file_list[_current_index].path);
}

void MainWindow::lastEntry()
{
    if (_file_list.empty())
    {
        return;
    }
    _current_index = _file_list.size() - 1;
    _media_widget->setMedia(_file_list[_current_index].path);
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
