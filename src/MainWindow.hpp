#pragma once

#include <QMainWindow>
#include <QStackedLayout>

#include "MediaWidget.hpp"

struct FileEntry
{
    std::string path;
    time_t mtime;
};

class MainWindow : public QMainWindow 
{
public:
    MainWindow(const std::string& target_path);

    void keyPressEvent(QKeyEvent* ev) override;

private:
    std::string _target_dir;
    std::vector<FileEntry> _file_list;
    int64_t _current_index = 0;
    QWidget* _main_widget = nullptr;
    QStackedLayout* _media_layout = nullptr;
    MediaWidget* _media_widget = nullptr;

    void loadFiles();
    void nextEntry(int times = 1);
    void prevEntry(int times = 1);
    void firstEntry();
    void lastEntry();
    void toggleFullScreen();
};