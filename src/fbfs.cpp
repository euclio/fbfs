#define FUSE_USE_VERSION 26

#include "FBGraph.h"
#include "Util.h"

#include <fuse.h>
#include "json_spirit.h"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <memory>
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

static int fbfs_getattr(const char* cpath, struct stat *stbuf) {
    std::string path(cpath);
    std::error_condition result;
    std::memset(stbuf, 0, sizeof(struct stat));

    if (path == "/") {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if (path == hello_path) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = hello_str.length();
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

    if (path == "/") {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);

        json_spirit::mObject permissions_response = get_fb_graph()->get("me", "permissions");
        if (!permissions_response.count("data")) {
            std::cerr << PERMISSION_CHECK_ERROR << std::endl;
            result = std::errc::network_unreachable;
            return -result.value();
        }

        // Check
        // https://developers.facebook.com/docs/facebook-login/permissions
        // for JSON format
        json_spirit::mObject permissions = permissions_response.at("data").get_array()[0].get_obj();
        json_spirit::write(permissions, std::cout);
        for (auto permission : permissions) {
            if (permission.second.get_int()) {
                filler(buf, permission.first.c_str(), NULL, 0);
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

    fb_graph->login();

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
