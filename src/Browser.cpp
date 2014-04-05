#include "Browser.h"
#include "FBGraph.h"

#include <iostream>

#include <QFile>
#include <QNetworkReply>
#include <QObject>
#include <QSslConfiguration>
#include <QTextStream>
#include <QUrlQuery>
#include <QWebView>
#include <QtDebug>
#include <QtGui>

void customMessageHandler(QtMsgType type, const QMessageLogContext &context,
                          const QString &msg) {
    (void)context;
    QString txt;
    switch (type) {
        case QtDebugMsg:
            txt = QString("Debug: %1").arg(msg);
            break;
        case QtWarningMsg:
            txt = QString("Warning: %1").arg(msg);
            break;
        case QtCriticalMsg:
            txt = QString("Critical: %1").arg(msg);
            break;
        case QtFatalMsg:
            txt = QString("Fatal %1").arg(msg);
            break;
    }

    QFile out_file("QtDebugLog.txt");
    out_file.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&out_file);
    ts << txt << endl;
}


Browser::Browser(FBGraph &parent) : parent(parent) {
    int argc = 0;
    char** argv = nullptr;

    qInstallMessageHandler(customMessageHandler);
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

    boost::optional<std::string> access_token = (
        parent.parse_login_response(scheme, host, path, parameters, fragment));

    if (access_token.is_initialized()) {
        // We have a valid access token!
        parent.set_access_token(access_token.get());
        parent.set_logged_in(true);
        app->exit();
    }
}

void Browser::ssl_error_handler(QNetworkReply* qnr,
                                const QList<QSslError> &errlist) {
    (void)errlist;
    qnr->ignoreSslErrors();
}
