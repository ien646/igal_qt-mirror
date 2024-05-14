#include <QApplication>
#include <QFontDatabase>
#include <QMessageBox>

#include <ien/fs_utils.hpp>

#include "MainWindow.hpp"

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    try
    {
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
        QFontDatabase::addApplicationFont(":/JohtoMono-Regular.otf");

        MainWindow window(path);
        window.show();

        app.exec();
        return 0;
    }
    catch(const std::exception& ex)
    {
        QMessageBox msgbox;
        msgbox.setStandardButtons(QMessageBox::StandardButton::Ok);
        msgbox.setWindowTitle("Error");
        msgbox.setText(ex.what());
        msgbox.exec();
    }
}