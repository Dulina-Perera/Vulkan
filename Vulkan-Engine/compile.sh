#!/bin/bash

glslc_path=$(which glslc)

if [ -z "$glslc_path" ]; then
    echo "glslc not found. Please install it using your package manager."
    exit 1
fi

mkdir shaders/build

for file in shaders/src/*.vert shaders/src/*.frag; do
    $glslc_path $file -o shaders/build/$(basename $file).spv
done
