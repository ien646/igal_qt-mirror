#pragma once

#include <QMainWindow>
#include <QStackedLayout>

#include "ListSelectWidget.hpp"
#include "MediaWidget.hpp"

#include <unordered_map>
#include <vector>

struct FileEntry
{
    std::string path;
    time_t mtime;

    bool operator==(const FileEntry& rhs) const { return path == rhs.path && mtime && rhs.mtime; }
};

class HelpOverlay;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(const std::string& target_path);

protected:
    void keyPressEvent(QKeyEvent* ev) override;
    void resizeEvent(QResizeEvent* ev) override;

signals:
    void currentIndexChanged(int64_t index);
    void resized(QSize size);
    void downPressed();
    void upPressed();

private:
    std::string _targetDir;
    std::vector<FileEntry> _fileList;
    std::unordered_map<int, std::string> _links;
    int64_t _currentIndex = 0;
    QWidget* _mainWidget = nullptr;
    QStackedLayout* _mediaLayout = nullptr;
    MediaWidget* _mediaWidget = nullptr;
    ListSelectWidget* _upscaleSelectWidget = nullptr;
    ListSelectWidget* _navigateSelectWidget = nullptr;
    HelpOverlay* _helpOverlay = nullptr;
    bool _controls_disabled = false;
    float _currentZoom = 1.0f;
    QPointF _currentTranslation = { 0.0f, 0.0f };
    bool _multiMode = false;

    void loadFiles();
    void loadFilesMulti(const std::vector<std::string>& abs_directories);
    void nextEntry(int times = 1);
    void prevEntry(int times = 1);
    void firstEntry();
    void lastEntry();
    void randomEntry();
    void toggleFullScreen();
    void loadLinks();
    void toggleCurrentFileInfo();
    void updateCurrentFileInfo();
    void processCopyToLinkKey(QKeyEvent* ev);
    void preCacheSurroundings();
    void upscaleImage(const std::string& path, const std::string& model);
    void navigateDir(const std::string& path);
    void deleteFile(const std::string& path);
    void openDir();
    void openNavigationDialog();
    void handleNumpadInput(int key);
    void handleStandardInput(int key);
};