find_package(Vulkan REQUIRED)

add_library(vulkanHeaders INTERFACE)
target_include_directories(vulkanHeaders INTERFACE Vulkan-Headers/include)

if(WIN32)
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE glm/glm)
endif()

add_library(vma INTERFACE)
target_include_directories(vma INTERFACE vulkan-headers vma)

add_library(tinyobjloader INTERFACE)
target_include_directories(tinyobjloader INTERFACE tinyobjloader)

add_library(tinygltfloader INTERFACE)
target_include_directories(tinygltfloader INTERFACE tinygltfloader)

add_library(stbimage INTERFACE)
target_include_directories(stbimage INTERFACE stbimage)

add_library(spirv-reflect "spirv-reflect/spirv_reflect.c" "spirv-reflect/spirv_reflect.cpp" "spirv-reflect/include/spirv/unified1/spirv.h" "spirv-reflect/spirv_reflect.h")
target_include_directories(spirv-reflect PUBLIC spirv-reflect)



set(IMGUI_HEADERS
	"imgui/imconfig.h"
	"imgui/imgui.h"
	"imgui/imgui_impl_glfw.h"
	"imgui/imgui_impl_vulkan.h"
	"imgui/imgui_internal.h"
	"imgui/imstb_rectpack.h"
	"imgui/imstb_textedit.h"
	"imgui/imstb_truetype.h")
	
set(IMGUI_SOURCES
	"imgui/imgui.cpp"
	"imgui/imgui_demo.cpp"
	"imgui/imgui_draw.cpp"
	"imgui/imgui_impl_glfw.cpp"
	"imgui/imgui_impl_vulkan.cpp"
	"imgui/imgui_tables.cpp"
	"imgui/imgui_widgets.cpp")
	
add_library(imgui ${IMGUI_SOURCES} ${IMGUI_HEADERS})
target_include_directories(imgui PUBLIC Vulkan-Headers/include imgui glfw/include)
