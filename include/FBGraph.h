#ifndef FBGRAPH_H
#define FBGRAPH_H

#include <boost/optional.hpp>
#include "json_spirit.h"

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
        void login(std::vector<std::string>&);
        json_spirit::mObject get(const std::string&, const std::string&,
                                 const std::string& = "");
        std::string get_endpoint_for_permission(const std::string&) const;
    private:
        json_spirit::mObject parse_response(const std::string&);
        std::string send_request(const std::string&, const std::string&,
                                 const std::string&);
        bool logged_in;
        std::string access_token;
};

#endif // FBGRAPH_H
