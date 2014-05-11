#define FUSE_USE_VERSION 26

#include "FBGraph.h"
#include "FBQuery.h"
#include "Util.h"

#include <boost/filesystem.hpp>
#include <fuse.h>
#include "json_spirit.h"

#include <cerrno>
#include <chrono>
#include <ctime>
#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <system_error>

static std::chrono::time_point<std::chrono::system_clock> mount_time;

static const std::string LOGIN_ERROR = "You are not logged in, so the program cannot fetch your profile. Terminating.";
static const std::string LOGIN_SUCCESS = "You are now logged into Facebook.";
static const std::string PERMISSION_CHECK_ERROR = "Could not determine app permissions.";
static const std::string POST_FILE_NAME = "post";

static inline FBGraph* get_fb_graph() {
    return static_cast<FBGraph*>(fuse_get_context()->private_data);
}

static inline std::string dirname(const std::string &path) {
    boost::filesystem::path p(path);
    return p.parent_path().string();
}

static inline std::string basename(const std::string &path) {
    return boost::filesystem::basename(path);
}

static inline std::set<std::string> get_endpoints() {
    return {
        "albums",
        "friends",
        "status",
    };
}

static inline std::error_condition handle_error(const json_spirit::mObject response) {
    json_spirit::mObject error = response.at("error").get_obj();
    std::cerr << error.at("message").get_str() << std::endl;
    if (error.at("type").get_str() == "OAuthException") {
        if (error.at("code").get_int() == 803) {
            return std::errc::no_such_file_or_directory;
        }

        return std::errc::permission_denied;
    }

    return std::errc::operation_not_permitted;
}

static inline std::string get_node_from_path(const std::string &path) {
    std::string p(path);
    std::string node;

    if (basename(p) == POST_FILE_NAME) {
        p = dirname(p);
    }

    if (dirname(p) == "/") {
        node = "me";
    } else {
        std::string friend_name = basename(dirname(p));
        node = get_fb_graph()->get_uid_from_name(friend_name);
    }

    return node;
}

static int fbfs_getattr(const char* cpath, struct stat *stbuf) {
    std::string path(cpath);
    std::error_condition result;
    std::memset(stbuf, 0, sizeof(struct stat));

    timespec mount_timespec;
    mount_timespec.tv_sec = std::chrono::system_clock::to_time_t(mount_time);

    if (path == "/" || path == "." || path == "..") {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        stbuf->st_mtim = mount_timespec;
        return 0;
    }

    if (basename(dirname(path)) == "friends") {
        // This is a directory representing a friend
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    if (basename(path) == POST_FILE_NAME) {
        stbuf->st_mode = S_IFREG | 0200;
        stbuf->st_size = 0;
        stbuf->st_mtim = mount_timespec;
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
        if (basename(dirname(path)) == "status") {
            // Store the date in the file
            FBQuery query(basename(path));
            query.add_parameter("date_format", "U");
            query.add_parameter("fields", "message,updated_time");
            json_spirit::mObject status_response = get_fb_graph()->get(query);
            if (status_response.count("error")) {
                result = handle_error(status_response);
                return -result.value();
            }

            const time_t updated_time = status_response.at("updated_time").get_int();
            timespec time;
            time.tv_sec = updated_time;
            stbuf->st_mtim = time;
            stbuf->st_size = status_response.at("message").get_str().length();
        } else if (basename(dirname(path)) == "albums") {
            // This is an album
            stbuf->st_mode = S_IFDIR | 0755;
            return 0;
        }
    } else {
        result = std::errc::no_such_file_or_directory;
        return -result.value();
    }

    return 0;
}

static int fbfs_unlink(const char *cpath) {
    std::string path(cpath);
    std::error_condition result;

    if (basename(path) == POST_FILE_NAME) {
        result = std::errc::permission_denied;
        return -result.value();
    }

    std::set<std::string> endpoints = get_endpoints();
    if (endpoints.count(basename(dirname(path)))) {
        // This is a file in an endpoint
        if (dirname(path) == "/status") {
            // This is a user status, so we can delete it.
            // The Facebook API requires the status ID to be appended to the
            // user ID instead of just using the status ID. This is
            // undocumented.
            std::string node = get_fb_graph()->get_user() + "_" + basename(path);
            FBQuery query(node);
            json_spirit::mValue response = get_fb_graph()->del(query);
            if (response.type() == json_spirit::bool_type) {
                return 0;
            }

            std::cout << "IT AIN'T A BOOL" << std::endl;
            std::cout << response.type() << std::endl;

            result = handle_error(response.get_obj());
            return -result.value();
        }
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
    std::set<std::string> friends = get_fb_graph()->get_friends();

    if (path == "/" || friends.count(basename(path))) {
        for (auto endpoint : endpoints) {
            filler(buf, endpoint.c_str(), NULL, 0);
        }
    } else if (endpoints.count(basename(path))) {
        // Request the user's friends
        FBQuery query("me", "friends");
        json_spirit::mObject friend_response = get_fb_graph()->get(query);
        json_spirit::mArray friends_list = friend_response.at("data").get_array();

        std::string node = get_node_from_path(path);
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
            json_spirit::mObject status_response = get_fb_graph()->get(query, true);
            json_spirit::mArray statuses = status_response.at("data").get_array();

            if (dirname(path) == "/") {
                filler(buf, POST_FILE_NAME.c_str(), NULL, 0);
            }

            for (auto& status : statuses) {
                if (!status.get_obj().count("message")) {
                    // The status doesn't have a message
                    continue;
                }
                std::string id = status.get_obj().at("id").get_str();
                filler(buf, id.c_str(), NULL, 0);
            }
        } else if (basename(path) == "albums") {
            FBQuery query(node, "albums");
            json_spirit::mObject albums_response = get_fb_graph()->get(query);
            json_spirit::mArray albums = albums_response.at("data").get_array();

            for (auto& album : albums) {
                std::string album_name = album.get_obj().at("name").get_str();
                filler(buf, album_name.c_str(), NULL, 0);
            }
        }
    }

    return 0;
}

static int fbfs_open(const char *cpath, struct fuse_file_info *fi) {
    std::string path(cpath);
    std::error_condition result;

    std::set<std::string> endpoints = get_endpoints();

    std::string parent_folder = basename(dirname(path));

    if (fi->flags & O_RDONLY) {
        result = std::errc::permission_denied;
        return -result.value();
    }

    return 0;
}

static int fbfs_truncate(const char *cpath, off_t size) {
    (void)cpath;
    (void)size;
    return 0;
}

static int fbfs_write(const char *cpath, const char *buf, size_t size,
                      off_t offset, struct fuse_file_info *fi) {
    (void)fi;
    std::error_condition result;
    std::string path(cpath);
    std::set<std::string> endpoints = get_endpoints();

    if (endpoints.count(basename(dirname(path)))) {
        // We are in an endpoint directory, so we are able to write the file
        const char *start = buf + offset;
        std::string data(start, size);
        // Determine which endpoint we are in
        std::string endpoint = basename(dirname(path));

        if (endpoint == "status") {
            // TODO: Allow writes to friend's walls as well. Unfortunately, it
            // is not possible to post directly using the Facebook API.
            // Instead, we will have to open a feed dialog.
            // https://developers.facebook.com/docs/sharing/reference/feed-dialog
            FBQuery query("me", "feed");
            query.add_parameter("message", data);
            json_spirit::mObject response = get_fb_graph()->post(query);
            if (response.count("error")) {
                result = handle_error(response);
                return -result.value();
            }

            return data.size();
        }
    }

    return 0;
}

static int fbfs_read(const char *cpath, char *buf, size_t size, off_t offset,
                     struct fuse_file_info *fi) {
    (void)fi;
    std::string path(cpath);
    std::error_condition result;

    FBQuery query(basename(path));
    query.add_parameter("fields", "message");
    json_spirit::mObject status_response = get_fb_graph()->get(query, true);
    std::string message = status_response.at("message").get_str();

    if (static_cast<unsigned>(offset) < message.length()) {
        if (offset + size > message.length()) {
            size = message.length() - offset;
        }

        std::memcpy(buf, message.c_str() + offset, size);
    } else {
        size = 0;
    }

    return size;
}

static void* fbfs_init(struct fuse_conn_info *ci) {
    (void)ci;

    FBGraph *fb_graph = new FBGraph();

    // We will ask for both user and friend variants of these permissions.
    // Refer to https://developers.facebook.com/docs/facebook-login/permissions
    // to see what each permission requests.
    std::vector<std::string> permissions = {
        "status",
    };

    std::vector<std::string> extended_permissions = {
        "publish_actions",
    };

    fb_graph->login(permissions, extended_permissions);

    if (!fb_graph->is_logged_in()) {
        std::cout << LOGIN_ERROR << std::endl;
        std::exit(EXIT_SUCCESS);
    }

    std::cout << LOGIN_SUCCESS << std::endl;

    // Store the time that the filesystem was mounted
    std::chrono::system_clock clock;
    mount_time = clock.now();

    return fb_graph;
}

static void fbfs_destroy(void *private_data) {
    delete static_cast<FBGraph*>(private_data);
}

static struct fuse_operations fbfs_oper;

void initialize_operations(fuse_operations& operations) {
    std::memset(static_cast<void*>(&operations), 0, sizeof(operations));

    operations.getattr  = fbfs_getattr;
    operations.unlink   = fbfs_unlink;
    operations.readdir  = fbfs_readdir;
    operations.open     = fbfs_open;
    operations.read     = fbfs_read;
    operations.init     = fbfs_init;
    operations.destroy  = fbfs_destroy;
    operations.truncate = fbfs_truncate;
    operations.write    = fbfs_write;
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
