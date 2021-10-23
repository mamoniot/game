

const uint GAME_MEMDESC_TEMP = 0b1;//marks that an allocation does not need to be preserved on save/load
const uint GAME_MEMDESC_INTERNAL = 0b10;//marks that an allocation is internal to its parent allocation and thus does not require separate memory management
typedef struct GameMemDesc {
	void* mem;
	inta alloc_size;// size in bytes of the memory at mem
	int children_total;// number of child GameMemDesc allocations at the beginning of the memory
	uint flags;
} GameMemDesc;

typedef struct Game {
	union {
		MamStack* stack;
		GameMemDesc stack_desc;
	};
	union {
		MamStack* temp_stack;
		GameMemDesc temp_stack_desc;
	};

	int32 grid_w;
	int32 grid_h;
	int32 grid[4*4];

	PCG rng;
	double lifetime;
	bool do_draw;

	bool input_left_down;
	bool input_right_down;
	bool input_up_down;
	bool input_down_down;

	bool input_left_just_down;
	bool input_right_just_down;
	bool input_up_just_down;
	bool input_down_just_down;
} Game;

typedef struct Output {
	bool game_quit;
	bool window_resize;
	bool do_draw;
} Output;



typedef struct MvkData {
	MamStack* stack;
	VkDevice device;
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPresentModeKHR present_mode;
	VkSurfaceFormatKHR surface_format;
	VkSurfaceCapabilitiesKHR capabilities;
	VkSwapchainKHR swap_chain;
	VkExtent2D swap_chain_image_extent;
	VkImageView* swap_chain_image_views;
	VkPipelineShaderStageCreateInfo* shader_stages;
	VkDescriptorSetLayout descriptor_set_layout;
	VkPipelineLayout pipeline_layout;
	VkFramebuffer* frame_buffers;
	VkRenderPass render_pass;
	VkPipeline pipeline;
	VkCommandPool command_pool;
	VkSemaphore* image_available_sems;
	VkSemaphore* render_finished_sems;
	VkFence* in_flight_fences;
	VkFence* images_in_flight_fences;
	VkQueue draw_queue;
	VkCommandBuffer* command_buffers;
	VkQueue present_queue;
	VkPhysicalDevice physical_device;
	uint32 vertex_buffer_size;
	VkBuffer vertex_buffer;
	VkDeviceMemory vertex_buffer_memory;
	uint32 index_buffer_size;
	VkBuffer index_buffer;
	VkDeviceMemory index_buffer_memory;
	VkBuffer uniform_buffer;
	VkDeviceMemory uniform_buffer_memory;
	VkDescriptorPool descriptor_pool;
	VkDescriptorSet* descriptor_sets;
	uint32 draw_queue_i;
	uint32 present_queue_i;
	uint32 shader_stages_size;
	uint32 swap_chain_size;
	uinta swap_chain_mem_start;
	bool device_does_vsync;
} MvkData;

const int TRASH_PTRS_SIZE = 4;
typedef struct MainTrash {
	bool sdl_isinit;
	MvkData* mvk;
	SDL_Window* window;
	GameMemDesc game_desc;
	void* ptrs[TRASH_PTRS_SIZE];
} MainTrash;


typedef struct Vertex {
    gbVec2 pos;
    gbVec3 color;
} Vertex;

typedef struct UniformBufferObject {
    gbMat4 model;
    gbMat4 view;
    gbMat4 proj;
} UniformBufferObject;
