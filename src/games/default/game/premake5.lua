project "gamedll"
	targetname  "game"
	targetdir 	"../../../../main"
	language    "C++"
	kind        "SharedLib"
	files
	{
		"../../../shared/*.cpp",
		"../../../shared/q_shared.h",
		"../../../shared/g_public.h",
		"../../../shared/surfaceflags.h",

		"*.c", "*.cpp", "*.h",
	}
	includedirs
	{
		"../../../shared",
	}
	defines
	{
		"QAGAME",
		"GAMEDLL",
		--"BOTLIB",
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
