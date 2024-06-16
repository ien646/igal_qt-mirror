#include <QApplication>
#include <QFontDatabase>
#include <QMessageBox>

#include <ien/fs_utils.hpp>
#include <ien/platform.hpp>

#include <iostream>

#include "MainWindow.hpp"

#ifdef IGAL_QT_VERSION
constexpr const char* APP_VERSION = IGAL_QT_VERSION;
#else
constexpr const char* APP_VERSION = "0.0.0";
#endif

#ifdef IEN_OS_WIN
    #include <ien/win32/windows.h>
#endif

int main(int argc, char** argv)
{
#ifdef IEN_OS_WIN
    const std::vector<std::wstring> wargs = ien::get_cmdline_wargs();
    const std::string path = wargs.size() > 1 ? ien::wstr_to_str(wargs[1]) : ien::get_current_user_homedir();
#else
    const std::string path = argc > 1 ? argv[1] : ien::get_current_user_homedir();
#endif
    if (path == "--version")
    {
        std::cout << "Version: " << APP_VERSION << std::endl;
        return 0;
    }

    QApplication app(argc, argv);

    const QIcon icon(":/igal_qt.png");
    app.setWindowIcon(icon);

    QGuiApplication::setWindowIcon(icon);
    try
    {
        QFontDatabase::addApplicationFont(":/JohtoMono-Regular.otf");

        MainWindow window(path);
        window.setWindowIcon(icon);
        
#ifdef IGAL_QT_VERSION
        window.setWindowTitle(QString::fromStdString(std::format("IGAL-QT <{}>", IGAL_QT_VERSION)));
#else
        window.setWindowTitle("IGAL-QT");
#endif

        window.resize(1000, 800);
        window.show();

        app.exec();
        return 0;
    }
    catch (const std::exception& ex)
    {
        QMessageBox msgbox;
        msgbox.setStandardButtons(QMessageBox::StandardButton::Ok);
        msgbox.setWindowTitle("Error");
        msgbox.setText(ex.what());
        msgbox.exec();
    }
}