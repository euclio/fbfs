#include "FBGraph.h"
#include "Browser.h"
#include "Util.h"

#include <boost/optional.hpp>
#include <curl_easy.h>
#include <curl_pair.h>
#include <fuse.h>
#include <JsonBox/Value.h>

#include <cstdlib>
#include <memory>
#include <iostream>
#include <sstream>
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

FBGraph::FBGraph() : logged_in(false) {};

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

JsonBox::Value FBGraph::get(const std::string &endpoint, const std::string &edge) {
    std::string response = send_request(endpoint, edge);
    JsonBox::Value json_reponse = parse_reponse(response);
    return response;
}

std::string FBGraph::send_request(const std::string& endpoint, const std::string &edge) {
    curl::curl_easy request;
    std::string response;

    std::ostringstream url_stream;
    url_stream << FACEBOOK_GRAPH_URL << "/" << endpoint << "/" << edge <<
        "?access_token=" << access_token;

    std::cout << url_stream.str() << std::endl;

    request.add_option(curl::curl_pair<CURLoption,string>(CURLOPT_URL, url_stream.str()));
    request.add_option(curl::curl_pair<CURLoption,decltype(&write_callback)>(CURLOPT_WRITEFUNCTION, &write_callback));
    request.add_option(curl::curl_pair<CURLoption,std::string*>(CURLOPT_WRITEDATA, &response));
    request.perform();

    return response;
}

JsonBox::Value FBGraph::parse_reponse(const std::string &response) {
    return JsonBox::Value(response);
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

void FBGraph::login() {
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
           "&response_type=" << RESPONSE_TYPE;

    Browser browser(*this);

    browser.open(fb_connect_url.str());
}
