set -xe

cc -ggdb -c -o main.o main.c -lGL -lGLU -lglfw -lm
cc -ggdb -o main main.o init.o stb_image.o -lGL -lGLU -lglfw -lm
./main
