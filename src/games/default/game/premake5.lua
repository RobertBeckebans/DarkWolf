project "gamedll"
	targetname  "game"
	targetdir 	"../../../../main"
	language    "C++"
	kind        "SharedLib"
	files
	{
		"../../../shared/*.c",
		"../../../shared/q_shared.h",
		"../../../shared/g_public.h",
		"../../../shared/surfaceflags.h",
		
		"*.c", "*.cpp", "*.h",
		"steam/*.c", "steam/*.h",
		"steamshim/*.c", "steamshim/*.h",
		"../../../libs/pthread-win32/pthread-JMP.c",
	}
	includedirs
	{
		"../../../shared",
		"../../../libs/pthread-win32",
	}
	defines
	{ 
		"QAGAME",
		"GAMEDLL",
		--"BOTLIB",
		"PTW32_ARCHX64",
		"HAVE_CONFIG_H",
	}
				
	filter "system:windows"
		linkoptions
		{
			"/DEF:game.def",
		}
		defines
		{
			--"WIN32",
			"_CRT_SECURE_NO_WARNINGS",
		}
	
	filter { "system:windows", "platforms:x64" }
		targetname  "qagame_sp_x64"
	
	filter { "system:linux", "platforms:x64" }
		targetname  "qagame_sp_x86_64"
		targetprefix ""

	