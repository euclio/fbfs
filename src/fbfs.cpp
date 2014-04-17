#define FUSE_USE_VERSION 26

#include "FBGraph.h"
#include "Util.h"

#include <boost/filesystem.hpp>
#include <fuse.h>
#include "json_spirit.h"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <system_error>

static const std::string LOGIN_ERROR = "You are not logged in, so the program cannot fetch your profile. Terminating.";
static const std::string LOGIN_SUCCESS = "You are now logged into Facebook.";
static const std::string PERMISSION_CHECK_ERROR = "Could not determine app permissions.";

static const std::string hello_str = "Hello World!\n";
static const std::string hello_path = "/hello";

static inline FBGraph* get_fb_graph() {
    return static_cast<FBGraph*>(fuse_get_context()->private_data);
}

static inline std::string dirname(const std::string path) {
    boost::filesystem::path p(path);
    return p.parent_path().string();
}

static inline std::string basename(const std::string path) {
    return boost::filesystem::basename(path);
}

static inline std::set<std::string> get_endpoints() {
    std::set<std::string> endpoints;

    json_spirit::mObject permissions_response = get_fb_graph()->get("me", "permissions");
    if (!permissions_response.count("data")) {
        std::cerr << PERMISSION_CHECK_ERROR << std::endl;
        std::error_condition err = std::errc::network_unreachable;
        errno = err.value();
        return endpoints;
    }

    // Check
    // https://developers.facebook.com/docs/facebook-login/permissions
    // for JSON format
    json_spirit::mObject permissions = (
            permissions_response.at("data").get_array()[0].get_obj());
    for (auto permission : permissions) {
        if (permission.first == "installed") {
            // Skip this permission, we know the app is installed
            continue;
        }

        if (permission.second.get_int()) {
            std::string endpoint = (
                get_fb_graph()->get_endpoint_for_permission(permission.first));
            endpoints.insert(endpoint);
        }
    }

    return endpoints;
}

static int fbfs_getattr(const char* cpath, struct stat *stbuf) {
    std::string path(cpath);
    std::error_condition result;
    std::memset(stbuf, 0, sizeof(struct stat));

    if (path == "/") {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    std::set<std::string> endpoints = get_endpoints();
    if (endpoints.count(basename(path))) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else {
        result = std::errc::no_such_file_or_directory;
        return -result.value();
    }

    return 0;
}

static int fbfs_readdir(const char *cpath, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi) {
    (void)offset;
    (void)fi;

    std::string path(cpath);
    std::error_condition result;

    std::set<std::string> endpoints = get_endpoints();

    if (path == "/") {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);

        for (auto endpoint : endpoints) {
            filler(buf, endpoint.c_str(), NULL, 0);
        }
    } else if (endpoints.count(basename(path))) {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);

        if (basename(path) == "friends") {
            if (dirname(path) == "/") {
                json_spirit::mObject friend_response = get_fb_graph()->get("me", "friends");
                json_spirit::mArray friends = friend_response.at("data").get_array();
                for (auto friend_obj : friends) {
                    std::string name = friend_obj.get_obj().at("name").get_str();
                    filler(buf, name.c_str(), NULL, 0);
                }
            } else {
                // We are in a friends directory. We should either make the
                // folder read-only, or a symlink to the user's friends.
                // TODO: Implement
            }
        }

    }

    return 0;
}

static int fbfs_open(const char *cpath, struct fuse_file_info *fi) {
    std::string path(cpath);
    std::error_condition result;

    if (path != hello_path) {
        result = std::errc::no_such_file_or_directory;
        return -result.value();
    }
    if ((fi->flags & 3) != O_RDONLY) {
        result = std::errc::permission_denied;
        return -result.value();
    }

    return 0;
}

static int fbfs_read(const char *cpath, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi) {
    (void)fi;
    std::string path(cpath);
    std::error_condition result;

    if (path != hello_path) {
        result = std::errc::no_such_file_or_directory;
        return -result.value();
    }

    if (static_cast<size_t>(offset) < hello_str.length()) {
        if (offset + size > hello_str.length()) {
            size = hello_str.length() - offset;
        }

        std::memcpy(buf, hello_str.c_str() + offset, size);
    } else {
        size = 0;
    }

    return size;
}

void* fbfs_init(struct fuse_conn_info *ci) {
    (void)ci;

    FBGraph *fb_graph = new FBGraph();

    // We will ask for both user and friend variants of these permissions.
    // Refer to https://developers.facebook.com/docs/facebook-login/permissions
    // to see what each permission requests.
    std::vector<std::string> permissions = {
        "events",
        "likes",
        "photos",
        "status",
    };

    fb_graph->login(permissions);

    if (!fb_graph->is_logged_in()) {
        std::cout << LOGIN_ERROR << std::endl;
        std::exit(EXIT_SUCCESS);
    }

    std::cout << LOGIN_SUCCESS << std::endl;
    return fb_graph;
}

void fbfs_destroy(void *private_data) {
    delete static_cast<FBGraph*>(private_data);
}

static struct fuse_operations fbfs_oper;

void initialize_operations(fuse_operations& operations) {
    operations.getattr = fbfs_getattr;
    operations.readdir = fbfs_readdir;
    operations.open    = fbfs_open;
    operations.read    = fbfs_read;
    operations.init    = fbfs_init;
    operations.destroy = fbfs_destroy;
}

void call_fusermount() {
    std::system("fusermount -u testdir");
}

int main(int argc, char *argv[]) {
    umask(0);

    std::atexit(call_fusermount);

    initialize_operations(fbfs_oper);
    return fuse_main(argc, argv, &fbfs_oper, NULL);
}
