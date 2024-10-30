CXX = g++
CXXFLAGS = -std=c++17 -Wall -lcurl

all: minecraft_updater

minecraft_updater: main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o minecraft_updater

clean:
	rm -f minecraft_updater