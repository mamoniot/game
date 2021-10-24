glslc w:/code/shaders/shader.vert -o w:/env/shaders/vert.spv -O;
glslc w:/code/shaders/shader.frag -o w:/env/shaders/frag.spv -O;

pushd w:/dev &> $NULL
mv w:/env/game.exe previous.exe &> $NULL

cl -nologo -Ox -favor:blend -w w:/code/main.cc -I w:/include -link -incremental:no w:/lib/SDL2.lib w:/lib/SDL2main.lib w:/lib/vulkan-1.lib w:/lib/VkLayer_utils.lib

cp main.exe w:/env/game.exe &> $NULL
rm main.exe &> $NULL
popd &> $NULL
