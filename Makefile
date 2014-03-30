CC := g++
CFLAGS := -g -std=c++11 -x c++ `pkg-config fuse --cflags --libs`

fbfs: fbfs.cpp
	$(CC) $(CFLAGS) fbfs.cpp -o fbfs
