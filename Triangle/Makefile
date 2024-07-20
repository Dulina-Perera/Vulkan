DEBUG_CFLAGS = -std=c++17 -O0 -g -Wall -Wextra -Werror -Wno-missing-field-initializers -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-value -Wno-unused-result -Wno-unused-local-typedefs
RELEASE_CFLAGS = -std=c++17 -O2 -Wall -Wextra -Werror -Wno-missing-field-initializers -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-value -Wno-unused-result -Wno-unused-local-typedefs

LDFLAGS = -ldl -lglfw -lpthread -lXi -lXrandr -lXxf86vm -lX11 -lvulkan

ifdef DEBUG
CFLAGS = $(DEBUG_CFLAGS)
else
CFLAGS = $(RELEASE_CFLAGS)
endif

Triangle: main.cc
	g++ $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: test clean

test: Triangle
	./Triangle

clean:
	rm -f Triangle
