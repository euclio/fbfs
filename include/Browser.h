#ifndef BROWSER_H
#define BROWSER_H

#include <memory>

#include <QObject>
#include <QtWidgets>
#include <QWebView>

class Browser : public QObject {
    Q_OBJECT

    public:
        Browser();
        int open(std::string);
    private:
        std::unique_ptr<QApplication> app;
        std::unique_ptr<QWidget> window;
        std::unique_ptr<QWebView> browser;
    private slots:
        void url_changed(QUrl);
};

#endif // BROWSER_H
