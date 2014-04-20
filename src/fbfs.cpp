#define FUSE_USE_VERSION 26

#include "FBGraph.h"
#include "FBQuery.h"
#include "Util.h"

#include <boost/filesystem.hpp>
#include <fuse.h>
#include "json_spirit.h"

#include <cerrno>
#include <ctime>
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

static const std::string TIME_FILENAME_FORMAT = "%F-%H%M%S";

static const size_t MAX_FILENAME_SIZE = 80;

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

    FBQuery query("me", "permissions");
    json_spirit::mObject permissions_response = get_fb_graph()->get(query);
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

    if (basename(dirname(path)) == "friends") {
        // This is a directory representing a friend
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    std::set<std::string> endpoints = get_endpoints();
    if (endpoints.count(basename(path))) {
        // This is an endpoint
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if (endpoints.count(basename(dirname(path)))) {
        // This is a file inside an endpoint
        stbuf->st_mode = S_IFREG | 0400;
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

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    std::set<std::string> endpoints = get_endpoints();

    if (path == "/") {
        for (auto endpoint : endpoints) {
            filler(buf, endpoint.c_str(), NULL, 0);
        }
    } else if (endpoints.count(basename(path))) {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);

        // Request the user's friends
        FBQuery query("me", "friends");
        json_spirit::mObject friend_response = get_fb_graph()->get(query);
        json_spirit::mArray friends_list = friend_response.at("data").get_array();

        std::string node;
        if (dirname(path) == "/") {
            node = "me";
        } else {
            // Determine the friends's ID by searching their name
            for (auto friend_obj : friends_list) {
                // FIXME: Friends may have the same name
                std::string friend_name = basename(dirname(path));
                if (friend_name == friend_obj.get_obj().at("name").get_str()) {
                    node = friend_obj.get_obj().at("id").get_str();
                    break;
                }
            }

            // Couldn't find a friend with a matching name
            if (node.empty()) {
                result = std::errc::no_such_file_or_directory;
                return -result.value();
            }
        }

        if (basename(path) == "friends") {
            if (dirname(path) == "/") {
                for (auto friend_obj : friends_list) {
                    std::string name = friend_obj.get_obj().at("name").get_str();
                    filler(buf, name.c_str(), NULL, 0);
                }
            } else {
                // We are in a friends directory. We should either make the
                // folder read-only, or a symlink to the user's friends.
                // TODO: Implement
            }
        } else if (basename(path) == "status") {
            FBQuery query(node, "statuses");
            query.add_parameter("date_format", "U");
            query.add_parameter("fields", "updated_time,message,id");
            json_spirit::mObject status_response = get_fb_graph()->get(query);
            json_spirit::mArray statuses = status_response.at("data").get_array();

            for (auto& status : statuses) {
                if (!status.get_obj().count("message")) {
                    // The status doesn't have a message
                    continue;
                }
                time_t timestamp = status.get_obj().at("updated_time").get_int();
                char time_string [MAX_FILENAME_SIZE];
                std::strftime(time_string, 80, TIME_FILENAME_FORMAT.c_str(),
                              std::localtime(&timestamp));
                filler(buf, time_string, NULL, 0);
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
