project "cgamedll"
	targetname  "cgame"
	targetdir 	"../../../../main"
	language    "C++"
	kind        "SharedLib"
	files
	{
		"../../../shared/*.c",
		"../../../shared/q_shared.h",
		"../../../shared/cg_public.h",
		"../../../shared/tr_types.h",
		"../../../shared/keycodes.h",
		"../../../shared/surfaceflags.h",
		
		"**.c", "**.cpp", "**.h",

		"../game/bg_*.c",
		"../game/bg_*.cpp",
		"../game/bg_*.h",
		
		"../ui/ui_shared.h",
		"../ui/ui_shared.c",
	}
	excludes
	{
		--"cg_particles.c",
	}
	includedirs
	{
		"../../../shared",
	}
	defines
	{ 
		"CGAME",
		"CGAMEDLL",
	}
	
	filter "system:windows"
		linkoptions
		{
			"/DEF:cgame.def",
		}
		defines
		{
			--"WIN32",
			"_CRT_SECURE_NO_WARNINGS",
		}
		
	filter { "system:windows", "platforms:x64" }
		targetname  "cgame_sp_x64"
	
	filter { "system:linux", "platforms:x64" }
		targetname  "cgame_sp_x86_64"
		targetprefix ""
	
