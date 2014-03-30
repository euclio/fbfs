#include "Browser.h"

#include <QtGui>
#include <QWebView>

Browser::Browser() {
    int argc = 0;
    char** argv = nullptr;

    app = std::unique_ptr<QApplication>(new QApplication(argc, argv));
    window = std::unique_ptr<QWidget>(new QWidget);
    browser = std::unique_ptr<QWebView>(new QWebView);
}

int Browser::open(std::string url) {
    QWebSettings *settings = QWebSettings::globalSettings();
    settings->setAttribute(QWebSettings::JavascriptEnabled, true);

    browser->setWindowTitle(QApplication::translate("toplevel", "Top-level Widget"));
    browser->show();
    browser->load(QUrl(url.c_str()));
    return app->exec();
}
