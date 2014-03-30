#include "FBGraph.h"
#include "Browser.h"
#include "Util.h"

#include <cstdlib>
#include <memory>
#include <iostream>
#include <sstream>
#include <string>

static const std::string CLIENT_ID = "732872016745752";
static const std::string REDIRECT_URI = "https://www.facebook.com/connect/login_success.html";

// Strings
static const std::string NOT_LOGGED_IN = "You appear to be logged out of Facebook.";
static const std::string ASK_OPEN_BROWSER = "Would you like to open a browser and log in?";

bool FBGraph::is_logged_in() const {
    return logged_in;
}

void FBGraph::authenticate() {

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
           "&redirect_uri=" << REDIRECT_URI;

    Browser browser;

    browser.open(fb_connect_url.str());
}
