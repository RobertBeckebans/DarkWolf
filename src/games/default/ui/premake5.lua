project "uidll"
	targetname  "ui"
	targetdir 	"../../../build/bin/%{cfg.system}/%{cfg.architecture}/%{cfg.buildcfg}"
	language    "C++"
	kind        "SharedLib"
	files
	{
		"../../../shared/*.cpp",
		"../../../shared/q_shared.h",
		"../../../shared/ui_public.h",
		"../../../shared/tr_types.h",
		"../../../shared/keycodes.h",
		"../../../shared/surfaceflags.h",

		"**.c", "**.cpp", "**.h",

		"../ui/**.c", "../ui/**.h",
		--"../game/bg_lib.c", "../game/bg_lib.h",
		"../game/bg_misc.cpp",
	}
	includedirs
	{
		"../../../shared",
	}
	defines
	{
		"UI",
		"UIDLL",
		"UI_EXPORTS"
	}

	filter "system:windows"
		linkoptions
		{
			'/DEF:"' .. path.join(_SCRIPT_DIR, "ui.def") .. '"',
		}
		postbuildcommands
		{
			'if not exist "$(SolutionDir)..\\..\\..\\main" mkdir "$(SolutionDir)..\\..\\..\\main"',
			'copy /Y "$(TargetPath)" "$(SolutionDir)..\\..\\..\\main\\"',
		}
		defines
		{
			"WIN32",
			"_CRT_SECURE_NO_WARNINGS",
		}

	filter { "system:windows", "platforms:x64" }
		targetname  "ui_sp_x64"

	filter { "system:linux", "platforms:x64" }
		targetname  "ui_sp_x86_64"
		targetprefix ""
