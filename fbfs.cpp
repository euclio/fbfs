#define FUSE_USE_VERSION 26

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <system_error>

#include <fuse.h>

static const std::string hello_str = "Hello World!\n";
static const std::string hello_path = "/hello";

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

    if (path != "/") {
        result = std::errc::no_such_file_or_directory;
        return -result.value();
    }

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    filler(buf, hello_path.c_str() + 1, NULL, 0);

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

static struct fuse_operations fbfs_oper;

void initialize_operations(fuse_operations& operations) {
    operations.getattr = fbfs_getattr;
    operations.readdir = fbfs_readdir;
    operations.open    = fbfs_open;
    operations.read    = fbfs_read;
}

int main(int argc, char *argv[]) {
    umask(0);

    initialize_operations(fbfs_oper);

    return fuse_main(argc, argv, &fbfs_oper, NULL);
}
