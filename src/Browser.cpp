#include "Browser.h"

#include <iostream>

#include <QObject>
#include <QtGui>
#include <QWebView>

Browser::Browser() {
    int argc = 0;
    char** argv = nullptr;

    app = std::unique_ptr<QApplication>(new QApplication(argc, argv));
    window = std::unique_ptr<QWidget>(new QWidget);
    browser = std::unique_ptr<QWebView>(new QWebView);
    QObject::connect(browser.get(), SIGNAL(urlChanged(QUrl)),
                     this,          SLOT(url_changed(QUrl)));
}

int Browser::open(std::string url) {
    QWebSettings *settings = QWebSettings::globalSettings();
    settings->setAttribute(QWebSettings::JavascriptEnabled, true);

    browser->setWindowTitle(QApplication::translate("toplevel", "FBFS Login"));
    browser->show();
    browser->load(QUrl(url.c_str()));
    return app->exec();
}

void Browser::url_changed(QUrl url) {
    std::cout << "URL_CHANGED TO " << url.toString().toStdString() << std::endl;
}
