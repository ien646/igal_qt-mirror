#pragma once

#include <QMainWindow>
#include <QStackedLayout>

#include "MediaWidget.hpp"

#include <unordered_map>
#include <vector>

struct FileEntry
{
    std::string path;
    time_t mtime;
};

class MainWindow : public QMainWindow 
{
    Q_OBJECT
public:
    MainWindow(const std::string& target_path);
    
    void keyPressEvent(QKeyEvent* ev) override;

signals:
    void currentIndexChanged(int64_t index);

private:
    std::string _targetDir;
    std::vector<FileEntry> _fileList;
    std::unordered_map<int, std::string> _links;
    int64_t _currentIndex = 0;
    QWidget* _mainWidget = nullptr;
    QStackedLayout* _mediaLayout = nullptr;
    MediaWidget* _mediaWidget = nullptr;

    void loadFiles();
    void nextEntry(int times = 1);
    void prevEntry(int times = 1);
    void firstEntry();
    void lastEntry();
    void toggleFullScreen();
    void loadLinks();
    void toggleCurrentFileInfo();
    void updateCurrentFileInfo();
    void processCopyToLinkKey(QKeyEvent* ev);
};