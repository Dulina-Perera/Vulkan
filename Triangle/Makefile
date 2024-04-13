CFLAGS = -std=c++17 -O2 -Wall -Wextra -Werror -Wno-missing-field-initializers -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-value -Wno-unused-result -Wno-unused-local-typedefs
LDFLAGS = -ldl -lglfw -lpthread -lXi -lXrandr -lXxf86vm -lX11 -lvulkan

Triangle: main.cpp
	g++ $(CFLAGS) -o Triangle main.cpp $(LDFLAGS)

.PHONY: test clean

test: Triangle
	./Triangle

clean:
	rm -f Triangle
