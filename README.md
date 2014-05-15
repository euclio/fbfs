# FBFS: Facebook Filesystem

Final project for CS137: Filesystems.

## Installation

Requirements:

* C++ compiler with C++11 support
* CMake
* FUSE 2.6
* Boost system and filesystem libraries
* Qt5 WebKit module

Once the requirements are met, install FBFS by entering the following commands
into a terminal:

```bash
cmake .
make
```

The binary produced will be called `fbfs`.

## Running FBFS

Open a terminal and run FBFS with the following arguments:

```bash
mkdir -p testdir
./fbfs -s -d -f testdir
```

Open a second terminal and enter:

```bash
ls testdir
```



## Paper
If you'd like, you can read the paper that I wrote describing the filesystem
[here].

[here]: paper/final_paper.pdf?raw=true
