LIB_PATH = "third_party/"

function useVulkan()
    includedirs { LIB_PATH .. "Vulkan-Headers/include" }

    local sdk_path = os.getenv("VULKAN_SDK")
    if sdk_path == "" then
        print("Could not find Vulkan Sdk, please make sure the Vulkan SDK is installed and that the environment variable VULKAN_SDK is defined.")
        return
    end
    libdirs { sdk_path .. "/Lib" }
        
    filter "system:windows"
        links "vulkan-1"
    
    filter "system:linux"
        links "vulkan"
    filter {}
end

function useVMA()
    includedirs {  LIB_PATH .. "vma" }
end

function useGLFW()
    includedirs {  LIB_PATH .. "glfw/include" }
    filter "system:windows"
        libdirs { LIB_PATH .. "glfw/lib" }
        links "glfw3"

    filter "system:linux"
        links "glfw"
    filter {}
end

function useGLM()
    includedirs {  LIB_PATH .. "glm" }
end

function useImGUI()
    includedirs { LIB_PATH .. "imgui"}
    files { LIB_PATH .. "imgui/*"}
end

function useSpirVReflect()
    includedirs { LIB_PATH .. "spirv-reflect"}
    files { LIB_PATH .. "spirv-reflect/**.cpp"}
end

function userHeaderOnlyLibs()
    includedirs { 
        LIB_PATH .. "tinygltfloader",
        LIB_PATH .. "tinyobjloader",
        LIB_PATH .. "stbimage"
    }
end

function compileShaders()
    filter "system:windows"
	    buildcommands {
		"{ECHO} \"Compiling Shaders\"",
		".\\shaders\\compile.bat"
	    }

    filter "system:linux"
	    buildcommands {
		"{ECHO} \"Compiling Shaders\"",
		"./shaders/compile.sh"
	    }
end

workspace "Pyrrhasterized"
    configurations { "Debug", "Release" }
    
    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG" }
        optimize "On"
        
project "Pyrrhasterized"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    location "generated"
    architecture "x86_64"
    targetdir ("build/bin/%{prj.name}/%{cfg.longname}")
    objdir ("build/obj/%{prj.name}/%{cfg.longname}")
    debugdir ""
    
    libdirs{}

    useVulkan()
    useVMA()
    useGLFW()
    useGLM()
    useImGUI()
    userHeaderOnlyLibs()
    useSpirVReflect()

    files { "src/**.h", "src/**.hpp", "src/**.cpp"}

    postbuildcommands {
    	"{RMDIR} %{cfg.targetdir}/assets",
        "{RMDIR} %{cfg.targetdir}/shaders",
     	"{COPYDIR} ../assets %{cfg.targetdir}/assets",
   	"{COPYDIR} ../shaders %{cfg.targetdir}/shaders"
    }



