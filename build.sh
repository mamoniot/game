glslc w:/code/shaders/shader.vert -o w:/env/shaders/vert.spv -O;
glslc w:/code/shaders/shader.frag -o w:/env/shaders/frag.spv -O;
g++ -O3 w:/code/main.cc -o/w:/env/game.exe -I/w:/include -L/w:/lib -lSDL2 -lSDL2main -lvulkan-1 -lVkLayer_utils -Wno-write-strings
