#include "FBGraph.h"
#include "FBQuery.h"
#include "Browser.h"
#include "Util.h"

#include <boost/optional.hpp>
#include <CurlEasy.h>
#include <CurlHttpPost.h>
#include <CurlPair.h>
#include <fuse.h>

#include <cstdlib>
#include <memory>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

// Facebook URL parameters
static const std::string CLIENT_ID = "732872016745752";
static const std::string REDIRECT_URI = "https://www.facebook.com/connect/login_success.html";
static const std::string RESPONSE_TYPE = "token";
static const std::string FACEBOOK_GRAPH_URL = "https://graph.facebook.com";

// Strings
static const std::string NOT_LOGGED_IN = "You appear to be logged out of Facebook.";
static const std::string ASK_OPEN_BROWSER = "Would you like to open a browser and log in?";
static const std::string CANCELLED_LOGIN = "Facebook has denied the request for your profile. Reason: ";
static const std::string PROGRAM_TERMINATION = "The program will now terminate.";

FBGraph::FBGraph() : logged_in(false), request_cache() {};

bool FBGraph::is_logged_in() const {
    return logged_in;
}

void FBGraph::set_logged_in(const bool is_logged_in) noexcept {
    this->logged_in = is_logged_in;
}

void FBGraph::set_access_token(const std::string &token) noexcept {
    access_token = token;
}

static std::size_t write_callback(void *contents, std::size_t size,
                                  std::size_t nmemb, void *userdata) {
    (void)userdata;
    std::size_t real_size = size * nmemb;
    static_cast<std::string*>(userdata)->append(static_cast<char*>(contents), real_size);
    return real_size;
}

json_spirit::mObject FBGraph::get(const FBQuery &query,
                                  const bool should_clear_cache) {

    std::string node = query.get_node();
    std::string endpoint = query.get_endpoint();
    std::string edge = query.get_edge();
    auto parameters = query.get_parameters();

    // Cache the request
    auto request = std::make_tuple(node, endpoint, edge, parameters);
    if (should_clear_cache || !request_cache.count(request)) {
        std::string response = send_request("GET", query);
        request_cache[request] = parse_response(response).get_obj();
    }

    return request_cache.at(request);
}

json_spirit::mObject FBGraph::post(const FBQuery &query) {
    return parse_response(send_request("POST", query)).get_obj();
}

json_spirit::mValue FBGraph::del(const FBQuery &query) {
    return parse_response(send_request("DELETE", query));
}

std::string FBGraph::send_request(const std::string &type, const FBQuery &query) {
    curl::CurlEasy request;
    std::string response;

    // Construct the request URL
    std::ostringstream url_stream;
    url_stream << FACEBOOK_GRAPH_URL << "/" << query.get_node();
    if (!query.get_endpoint().empty()) {
        url_stream << "/" << query.get_endpoint();
    }

    if (!query.get_edge().empty()) {
        url_stream << "/" << query.get_edge();
    }

    url_stream << "?" << "access_token=" << access_token;
    if (!query.get_parameters().empty()) {
        for (auto parameter : query.get_parameters()) {
            std::string key = parameter.first;
            std::string value = parameter.second;
            request.escape(key);
            request.escape(value);

            url_stream << "&" << key << "=" << value;
        }
    }

    std::string url = url_stream.str();

    std::cout << url << std::endl;

    if (type == "POST") {
        CurlHttpPost post;
        request.addOption(CurlPair<CURLoption,CurlHttpPost>(CURLOPT_HTTPPOST, post));
    } else if (type == "DELETE") {
        request.addOption(CurlPair<CURLoption,std::string>(CURLOPT_CUSTOMREQUEST, "DELETE"));
    }

    request.addOption(CurlPair<CURLoption,string>(CURLOPT_URL, url));
    request.addOption(CurlPair<CURLoption,decltype(&write_callback)>(CURLOPT_WRITEFUNCTION, &write_callback));
    request.addOption(CurlPair<CURLoption,std::string*>(CURLOPT_WRITEDATA, &response));
    request.perform();

    std::cout << response << std::endl;

    return response;
}

json_spirit::mValue FBGraph::parse_response(const std::string &response) {
    json_spirit::mValue response_json;
    json_spirit::read(response, response_json);

    return response_json;
}

boost::optional<std::string> get_fragment_value(const std::string &fragment,
                                                const std::string &key) {
    std::size_t key_index = fragment.find(key);

    if (key_index == std::string::npos) {
        // The parameter wasn't found
        return boost::optional<std::string>();
    }

    // Add 1 to skip the equals sign
    std::size_t value_index = key_index + key.length() + 1;

    // The next parameter marks the end of the value
    std::size_t value_end_index = fragment.find("&");

    if (value_end_index == std::string::npos) {
        // It's ok that we didn't find the next parameter. Just use the end of
        // the string.
        value_end_index = fragment.length();
    }

    std::string result = fragment.substr(value_index, value_end_index - value_index);
    return boost::optional<std::string>(result);
}

boost::optional<std::string>
FBGraph::parse_login_response(const std::string scheme, const std::string host,
        const std::string path, const std::map<std::string, std::string> parameters,
        const std::string fragment) const noexcept {

    if (host != "www.facebook.com") {
        return boost::optional<std::string>();
    }

    // Ensure that Facebook followed the redirect URL
    if (scheme + "://" + host + path == REDIRECT_URI) {
        if (parameters.count("error_reason")) {
            std::string reason = parameters.at("error_reason");
            std::cout << CANCELLED_LOGIN << reason << std::endl;
            std::cout << PROGRAM_TERMINATION << std::endl;
            std::exit(EXIT_SUCCESS);
        }
    }

    // Parse the fragment to get the access token.
    // Ensure that the access token was found
    // TODO: Should verify whether the access token is valid or not
    return get_fragment_value(fragment, "access_token");
}

std::string FBGraph::get_endpoint_for_permission(const std::string &permission) const {
    if (permission == "basic_info") {
        // Keep this endpoint name the same because it isn't really an endpoint
        return permission;
    }

    std::size_t begin = permission.find("_") + 1;
    return permission.substr(begin, permission.length() - begin);
}

std::string FBGraph::get_uid_from_name(std::string name) {
    std::string fql = "SELECT id FROM profile WHERE id IN "
                      "(SELECT uid2 FROM friend WHERE uid1 = me()) "
                      "AND name = \"" + name + "\"";
    json_spirit::mObject response = fql_get(fql);

    // FIXME: Code assumes unique names for now
    return response.at("data").get_array()[0].get_obj().at("id").get_str();
}

std::set<std::string> FBGraph::get_friends() {
    std::string fql = "SELECT name FROM profile WHERE id IN "
                      "(SELECT uid2 FROM friend WHERE uid1 = me())";
    json_spirit::mObject response = fql_get(fql);
    json_spirit::mArray friends_list = response.at("data").get_array();
    std::set<std::string> friends;
    for (auto &friend_name : friends_list) {
        friends.insert(friend_name.get_obj().at("name").get_str());
    }

    return friends;
}


json_spirit::mObject FBGraph::fql_get(const std::string &fql_query,
                                      bool should_clear_cache) {
    if (!should_clear_cache && fql_cache.count(fql_query)) {
        return fql_cache.at(fql_query);
    }

    FBQuery query("fql");
    query.add_parameter("q", fql_query);
    json_spirit::mObject response = parse_response(send_request("GET", query)).get_obj();
    fql_cache[fql_query] = response;
    return response;
}

std::string FBGraph::get_user() {
    FBQuery query("me");
    query.add_parameter("fields", "id");
    return get(query).at("id").get_str();
}

void FBGraph::login(std::vector<std::string> &permissions,
                    std::vector<std::string> &extended_permissions) {
    if (is_logged_in()) {
        return;
    }

    std::cout << NOT_LOGGED_IN << std::endl;

    bool should_open_browser = confirm_yes(ASK_OPEN_BROWSER, true);

    if (!should_open_browser) {
        return;
    }

    // Create the Facebook Connect URL
    std::ostringstream fb_connect_url;
    fb_connect_url << "https://www.facebook.com/dialog/oauth?" <<
           "client_id=" << CLIENT_ID <<
           "&redirect_uri=" << REDIRECT_URI <<
           "&response_type=" << RESPONSE_TYPE <<
           "&scope=" << "basic_info,";
    for (auto permission : permissions) {
        fb_connect_url <<
            "user_" << permission << "," <<
            "friends_" << permission << ",";
    }

    for (auto permission : extended_permissions) {
        fb_connect_url << permission << ",";
    }

    Browser browser(*this);

    browser.open(fb_connect_url.str());
}
