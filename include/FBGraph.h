#ifndef FBGRAPH_H
#define FBGRAPH_H

#include <QWebView>

class FBGraph {
    public:
        bool is_logged_in() const;
        void parse_login_response(std::string, std::string, std::string,
                std::map<std::string, std::string>, std::string);
        void login();
    private:
        bool logged_in;
};

#endif // FBGRAPH_H
