#ifndef FBGRAPH_H
#define FBGRAPH_H

#include "FBQuery.h"

#include <boost/optional.hpp>
#include "json_spirit.h"

#include <map>
#include <set>

typedef std::map<
    std::tuple<std::string, std::string, std::string,
        std::vector<std::pair<std::string, std::string>>>,
    json_spirit::mObject> request_cache_t;

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
        void login(std::vector<std::string>&, std::vector<std::string>&);
        json_spirit::mObject get(const FBQuery&, const bool = false);
        json_spirit::mObject post(const FBQuery&);
        std::string get_endpoint_for_permission(const std::string&) const;
        json_spirit::mObject fql_get(const std::string&);
        std::string get_uid_from_name(std::string name);
        std::set<std::string> get_friends();
    private:
        json_spirit::mObject parse_response(const std::string&);
        std::string send_request(const std::string&, const FBQuery&);
        bool logged_in;
        std::string access_token;
        request_cache_t request_cache;
};

#endif // FBGRAPH_H
