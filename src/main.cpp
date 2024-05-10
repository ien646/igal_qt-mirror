#include <QApplication>
#include <QFontDatabase>

#include <ien/fs_utils.hpp>

#include "MainWindow.hpp"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    std::string path;
    if (argc > 1)
    {
        path = argv[1];
    }
    else
    {
        path = ien::get_current_user_homedir();
    }

    QFontDatabase::addApplicationFont(":/Pokemon Classic.ttf");

    MainWindow window(path);
    window.show();

    app.exec();
    return 0;
}