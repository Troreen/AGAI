include "../../Premake/extensions.lua"
include "../../Premake/common.lua"

examples = os.matchdirs("*")

newoption {
    trigger = "defaultproject",
    value = "Platformer_with_Editor",
    default = "Platformer_with_Editor",
    description = "Select default project to build and run."
}

workspace "Examples"
	architecture "x64"
	location "../../"
	
	startproject (_OPTIONS["defaultproject"])

	configurations {
		"Debug",
		"Release",
		"Retail"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group ("Engine")
include (dirs.external)
include (dirs.application)
include (dirs.core_tests)
include (dirs.graphics)
include (dirs.core)
include (dirs.scene_script_core)

group ("Editor")
include (dirs.editor)
include (dirs.editor_default_graphics)

group ("Examples")

for _, v in pairs(examples) do
	group("Examples/" .. v)
	if os.isfile(path.join(v, "premake5.lua")) then
		include (v)
	end
	group()
end
