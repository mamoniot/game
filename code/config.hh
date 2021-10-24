#include "basic.h"



#define WINDOW_NAME "game"
#define MVK_ENGINE_NAME "No Engine"


#ifdef DEBUG
	#define MVK_DEBUG_LAYERS_SIZE 1
	static char* MVK_DEBUG_LAYERS[MVK_DEBUG_LAYERS_SIZE] = {"VK_LAYER_KHRONOS_validation"};
#else
	#define MVK_DEBUG_LAYERS_SIZE 0
	static char** MVK_DEBUG_LAYERS = 0;
#endif
#define MVK_DEVICE_EXTENSIONS_SIZE 1
static char* MVK_DEVICE_EXTENSIONS[MVK_DEVICE_EXTENSIONS_SIZE] = {"VK_KHR_swapchain"};
#define MVK_SHADER_FRAG "frag.spv"
#define MVK_SHADER_VERT "vert.spv"
#define MVK_FRAMES_IN_FLIGHT 2

#define DEFAULT_SCREEN_WIDTH 1200
#define DEFAULT_SCREEN_HEIGHT 800

#ifdef DEBUG
	#define PEDAL_TO_THE_METAL 0
	#define FPS_PRINTOUT_FREQUENCY 1024
#else
	#define PEDAL_TO_THE_METAL 0
	#define FPS_PRINTOUT_FREQUENCY 0
#endif

const int VERTEX_BUFFER_SIZE = MEGABYTE;
const int INDEX_BUFFER_SIZE = MEGABYTE;

const inta TEMP_STACK_SIZE = MEGABYTE;
const inta GAME_STACK_SIZE = MEGABYTE;
const float MAX_UPDATE_DELTA = .5;
const float DELAY_RESOLUTION = 0.0005;
const float DEFAULT_FPS = (1.0f/60.0f);


// VK_KHR_surface
// VK_KHR_win32_surface
// VK_KHR_get_physical_device_properties2
// VK_KHR_get_surface_capabilities2
// VK_KHR_external_memory_capabilities
// VK_KHR_device_group_creation
// VK_KHR_external_semaphore_capabilities
// VK_KHR_external_fence_capabilities
// VK_EXT_debug_report
// VK_EXT_debug_utils
// VK_EXT_swapchain_colorspace
