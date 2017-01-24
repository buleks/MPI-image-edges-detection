CXX=mpic++
CPPFLAGS=-lfreeimage -lfreetype -I/usr/include/freetype2 -std=gnu++11 -g


edge: edges.cpp
	$(CXX) $(CPPFLAGS) $< -o $@ 




all: edge
.PHONY: edge all
clean:
	rm -f *.o	
