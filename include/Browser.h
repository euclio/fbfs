#ifndef BROWSER_H
#define BROWSER_H

#include <memory>

#include <QtWidgets>
#include <QWebView>

class Browser {
    public:
        Browser();
        int open(std::string);
    private:
        std::unique_ptr<QApplication> app;
        std::unique_ptr<QWidget> window;
        std::unique_ptr<QWebView> browser;
};

#endif // BROWSER_H
