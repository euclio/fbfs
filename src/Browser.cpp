#include "Browser.h"
#include "FBGraph.h"

#include <iostream>

#include <QNetworkReply>
#include <QObject>
#include <QSslConfiguration>
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

    // The browser refuses to load any content that involves SSL errors.
    // Facebook's CSS and JavaScript cause some SSL errors upon load, so
    // logging in through the browser is impossible. To fix this, we load a new
    // SSL configuration containing the certificates on the computer.
    QSslConfiguration ssl_config = QSslConfiguration::defaultConfiguration();
    QList<QSslCertificate> ca_list = ssl_config.caCertificates();
    QList<QSslCertificate> new_ca = QSslCertificate::fromData("CaCertificates");
    ca_list += new_ca;
    ssl_config.setCaCertificates(ca_list);
    ssl_config.setProtocol(QSsl::AnyProtocol);
    QSslConfiguration::setDefaultConfiguration(ssl_config);

    // Now, we register an error handler that simply ignores all SSL errors.
    QObject::connect(browser->page()->networkAccessManager(),
                     SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> &)),
                     this,
                     SLOT(ssl_error_handler(QNetworkReply*, const QList<QSslError> &)));
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

void Browser::ssl_error_handler(QNetworkReply* qnr,
                                const QList<QSslError> &errlist) {
    (void)errlist;
    qnr->ignoreSslErrors();
}
