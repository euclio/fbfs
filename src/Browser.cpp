#include "Browser.h"
#include "FBGraph.h"

#include <iostream>

#include <QObject>
#include <QtGui>
#include <QUrlQuery>
#include <QWebView>

Browser::Browser(FBGraph &parent) : parent(parent) {
    int argc = 0;
    char** argv = nullptr;

    app = std::unique_ptr<QApplication>(new QApplication(argc, argv));
    window = std::unique_ptr<QWidget>(new QWidget);
    browser = std::unique_ptr<QWebView>(new QWebView);
    QObject::connect(browser.get(), SIGNAL(urlChanged(QUrl)),
                     this,          SLOT(url_changed(const QUrl&)));
}

int Browser::open(std::string url) {
    QWebSettings *settings = QWebSettings::globalSettings();
    settings->setAttribute(QWebSettings::JavascriptEnabled, true);

    browser->setWindowTitle(QApplication::translate("toplevel", "FBFS Login"));
    browser->show();
    browser->load(QUrl(url.c_str()));
    return app->exec();
}

void Browser::url_changed(const QUrl &url) {
    std::string scheme = url.scheme().toStdString();
    std::string host = url.host().toStdString();
    std::string path = url.path().toStdString();

    // Convert the URL parameters into a map of strings
    QUrlQuery query(url);
    std::map<std::string, std::string> parameters;
    for (auto item : query.queryItems()) {
        std::string key = item.first.toStdString();
        std::string value = item.second.toStdString();
        parameters[key] = value;
    }

    std::string fragment = url.fragment().toStdString();

    parent.parse_login_response(scheme, host, path, parameters, fragment);
}
