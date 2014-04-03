#ifndef BROWSER_H
#define BROWSER_H

#include "FBGraph.h"

#include <memory>

#include <QObject>
#include <QtWidgets>
#include <QWebView>

class Browser : public QObject {
    Q_OBJECT

    public:
        Browser(FBGraph& parent);
        int open(std::string);
    private:
        std::unique_ptr<QApplication> app;
        std::unique_ptr<QWidget> window;
        std::unique_ptr<QWebView> browser;
        FBGraph &parent;
    private slots:
        void url_changed(const QUrl&);
        void ssl_error_handler(QNetworkReply*, const QList<QSslError> &errors);
};

#endif // BROWSER_H
