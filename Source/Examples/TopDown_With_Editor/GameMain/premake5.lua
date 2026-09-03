include "../../Premake/common.lua"

local projectname = "TopDownGameMain"
project (projectname)
	location (dirs.projectfiles)
	dependson { "External", "Application", "TopDownGame" }
		
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"

	debugdir "%{dirs.bin}"
	targetdir ("%{dirs.bin}")
	targetname("%{prj.name}_%{cfg.buildcfg}")
	objdir ("%{dirs.temp}/%{prj.name}/%{cfg.buildcfg}")

	links {"External", "Application", "TopDownGame"}

	includedirs { 
		dirs.external, 
		dirs.external .. "ffmpeg-2.0/",
		dirs.application, 
		"../Game/source"
	}

	files {
		"source/**.h",
		"source/**.cpp",
	}

	libdirs { dirs.lib, dirs.dependencies }
	
	filter "configurations:Debug"
		defines {"_DEBUG"}
		runtime "Debug"
		symbols "on"
		files {"tools/**"}
		includedirs {"tools/"}
	filter "configurations:Release"
		defines "_RELEASE"
		runtime "Release"
		optimize "on"
		files {"tools/**"}
		includedirs {"tools/"}
	filter "configurations:Retail"
		defines "_RETAIL"
		runtime "Release"
		optimize "on"

	filter "system:windows"
		staticruntime "off"
		symbols "On"		
		systemversion "latest"
		warnings "Extra"
		flags { 
			"FatalCompileWarnings",
			"MultiProcessorCompile"
		}
		
		defines {
			"WIN32",
			"_LIB", 
			"TGE_SYSTEM_WINDOWS" 
		}
    
	filter { "system:windows", "not configurations:Retail" }
		editandcontinue "Off"
		buildoptions { "/Gm-" }
		buildoptions { "/Gy" }
		buildoptions { "/Gw" }
		linkoptions { "/FUNCTIONPADMIN" }
		linkoptions { "/OPT:NOREF" }
		linkoptions { "/OPT:NOICF" }
		linkoptions { "/DEBUG:FULL" }
