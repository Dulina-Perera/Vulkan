include .env

CFLAGS = -std=c++17 -O2 -Wall -Wextra -Werror
LDFLAGS = -ldl -lglfw -lpthread -lXi -lXrandr -lXxf86vm -lX11 -lvulkan

VulkanEngine: *.cpp
	g++ $(CFLAGS) -o VulkanEngine *.cpp $(LDFLAGS)

.PHONY: test clean

test: VulkanEngine
	./VulkanEngine

clean:
	rm -f VulkanEngine
