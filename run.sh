#!/bin/sh

set -xe

gcc -ggdb -c -o main.o main.c -lGL -lGLU -lglfw -lm
gcc -ggdb -o main main.o stb_image.o -lGL -lGLU -lglfw -lm
./main
