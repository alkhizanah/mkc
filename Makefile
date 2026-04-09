all:
	mkdir -p build
	$(CXX) -o build/mkc src/file_watch.cc src/main.cc
