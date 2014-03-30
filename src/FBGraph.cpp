#include "FBGraph.h"
#include "Browser.h"

#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>

static const std::string CLIENT_ID = "732872016745752";
static const std::string REDIRECT_URI = "https://www.facebook.com/connect/login_success.html";

bool FBGraph::is_logged_in() const {
    return logged_in;
}

void FBGraph::authenticate() {

}

void FBGraph::login() {
    if (is_logged_in()) {
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
