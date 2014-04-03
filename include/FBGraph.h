#ifndef FBGRAPH_H
#define FBGRAPH_H

#include <boost/optional.hpp>

#include <map>

class FBGraph {
    public:
        FBGraph();
        bool is_logged_in() const;
        void set_logged_in(const bool) noexcept;
        void set_access_token(const std::string&) noexcept;
        boost::optional<std::string>
            parse_login_response(const std::string, const std::string,
                const std::string, const std::map<std::string, std::string>,
                const std::string) const noexcept;
        void login();
    private:
        bool logged_in;
        std::string access_token;
};

#endif // FBGRAPH_H
