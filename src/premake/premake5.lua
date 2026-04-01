--
-- RBQUAKE-3 build configuration script
--
solution "DarkWolf"
    configurations { "Debug", "Profile", "Release" }
    platforms { "x64" }
    characterset ("ASCII")

    filter "configurations:Debug"
        defines { "_DEBUG" }
        symbols "On"
        vectorextensions "SSE"
        warnings "Extra"

    filter "configurations:Profile"
        defines { "NDEBUG" }
        symbols "On"
        vectorextensions "SSE"
        optimize "Speed"
        warnings "Extra"

    filter "configurations:Release"
        defines { "NDEBUG" }
        symbols "Off"
        vectorextensions "SSE"
        optimize "Speed"
        warnings "Extra"

    filter "system:windows"
        targetdir ".."
        flags {
            "NoManifest",
            "NoMinimalRebuild",
            "No64BitChecks",
        }
        exceptionhandling "Off"
        editandcontinue "Off"
        systemversion "latest"
        buildoptions {
            "/MP", -- multi processor support
            "/we4485", -- An accessor overrides warning
        }

    filter { "system:windows", "configurations:Debug" }
        buildoptions {
            -- turn off Smaller Type Check and Basic Runtime Checks (commented out in original)
            -- "/RTC-",
            -- "/RTC1-",
        }

    filter { "system:windows", "configurations:Profile" }
        buildoptions {
            "/Zi", -- Produces a program database (PDB)
            "/Oi", -- Enable Intrinsic Functions
            "/Ot", -- Favor fast code
            "/Oy", -- Omit Frame Pointers
        }
        linkoptions {
            "/DEBUG", -- Create .pdb file
        }

    filter { "system:windows", "configurations:Release" }
        buildoptions {
            "/Ob2", -- Inline Function Expansion: Any Suitable
            "/Ot", -- Favor fast code
            "/Oi", -- Enable Intrinsic Functions
            "/Oy", -- Omit Frame Pointers
        }

--
-- Options
--
newoption {
    trigger     = "with-pch",
    description = "Enable precompiled headers"
}

newoption {
    trigger     = "with-bfg",
    description = "Compile with RBDOOM-3-BFG renderer"
}

newoption {
    trigger = "with-freetype",
    description = "Compile with freetype support"
}

newoption {
    trigger = "with-openal",
    value = "TYPE",
    description = "Specify which OpenAL library",
    allowed = {
        { "none", "No support for OpenAL" },
        { "dlopen", "Dynamically load OpenAL library if available" },
        { "link", "Link the OpenAL library as normal" },
        { "openal-dlopen", "Dynamically load OpenAL library if available" },
        { "openal-link", "Link the OpenAL library as normal" }
    }
}

--
-- Platform specific defaults
--
-- Default to dlopen version of OpenAL
if not _OPTIONS["with-openal"] then
    _OPTIONS["with-openal"] = "dlopen"
end
if _OPTIONS["with-openal"] then
    _OPTIONS["with-openal"] = "openal-" .. _OPTIONS["with-openal"]
end

-- main engine code
project "DarkWolf"
    targetname "DarkWolf"
    language "C++"
    kind "WindowedApp"
    files {
        "../shared/**.cpp", "../shared/**.h",
        "../engine/client/*.cpp", "../engine/client/*.h",
        "../engine/server/*.cpp", "../engine/server/*.h",
        "../engine/sound/*.cpp", "../engine/sound/*.h",
        "../engine/qcommon/**.h",
        "../engine/qcommon/cmd.cpp",
        "../engine/qcommon/common.cpp",
        "../engine/qcommon/cvar.cpp",
        "../engine/qcommon/files.cpp",
        "../engine/qcommon/huffman.cpp",
        "../engine/qcommon/md4.cpp",
        --"../engine/qcommon/md5.cpp",
        "../engine/qcommon/msg.cpp",
        --"../engine/qcommon/puff.cpp",
        --"../engine/qcommon/vm.cpp",
        -- "../engine/qcommon/vm_x86.c",
        "../engine/qcommon/net_*.cpp",
        "../engine/qcommon/unzip.cpp",
		"../engine/sys/sys_local.h",
        "../engine/collision/*.cpp", "../engine/collision/*.h",
        "../engine/botlib/*.cpp", "../engine/botlib/*.h",
        "../engine/splines/*.cpp", "../engine/splines/*.h",

		"../libs/jpeg-6/**.c", "../libs/jpeg/**.h",
        --"../libs/zlib/**.cpp", "../libs/zlib/**.c", "../libs/zlib/**.h",
        "../libs/opengl/**.cpp", "../libs/opengl/**.c", "../libs/opengl/**.h",
    }
    includedirs {
        "../shared",
		"../shared/botshared",
        "../libs/zlib",
        "../libs/ft2/include",
    }
    defines {
        "REF_HARD_LINKED",
        "GLEW_STATIC",
        --"BUILD_FREETYPE",
        "FT2_BUILD_LIBRARY",
        "USE_CODEC_VORBIS",
        "USE_ALLOCA",
        "FLOATING_POINT",
        "BOTLIB",
		"USE_OPENGLES",
        --"USE_LOCAL_HEADERS",
    }
    excludes {
        "../engine/server/sv_rankings.c",
        "../engine/renderer/tr_animation_mdm.c",
        "../engine/renderer/tr_model_mdm.c",
    }
	links {
		--"idlib"
	}

    --
    -- Platform Configurations
    --

    --
    -- Options Configurations
    --
	-- Optional: Pre‑compiled header support (MSVC / Clang / GCC)
		files { "../engine/renderer/**.c", "../engine/renderer/**.cpp", "../engine/renderer/**.h", }

    filter "options:with-freetype"
        links { "freetype" }
        buildoptions { "`pkg-config --cflags freetype2`" }
        defines { "BUILD_FREETYPE" }
        files {
            "../libs/ft2/**.c", "../libs/ft2/**.h",
        }

    filter "options:openal-dlopen"
        defines {
            "USE_OPENAL",
            "USE_OPENAL_DLOPEN",
            "USE_OPENAL_LOCAL_HEADERS"
        }

    filter "options:openal-link"
        links { "openal" }
        defines { "USE_OPENAL" }

    --
    -- Project Configurations
    --
    filter "system:windows"
        files {
            "../engine/win32/*.h",
            "../engine/win32/*.cpp",
            "../engine/win32/qe3.ico",
            "../engine/win32/winquake.rc",
        }
        defines {
            "USE_OPENAL",
            "WIN32",
            "_CRT_SECURE_NO_WARNINGS",
        }
        includedirs {
            "../libs/sdl2/include",
            "../libs/openal/include",
            "../libs/opengl/include",
        }
        links {
            --"SDL2",
            --"SDL2main",
            "winmm",
            "wsock32",
            --"opengl32",
            "user32",
            "advapi32",
            "ws2_32",
            "Psapi",
			"Shlwapi",
        }
        buildoptions {
            -- "/MT"
        }
        linkoptions {
            -- "/LARGEADDRESSAWARE",
        }

    filter { "system:windows", "platforms:x64" }
        targetdir "../.."
        libdirs {
            "../libs/sdl2/lib/x64",
            "../libs/openal/libs/win64",
        }
        links {
            "OpenAL32",
        }

    filter { "system:linux", "action:gmake" }
        buildoptions {
            "`pkg-config --cflags sdl2`",
            "`pkg-config --cflags libcurl`",
        }
        linkoptions {
            "`pkg-config --libs sdl2`",
            "`pkg-config --libs libcurl`",
        }
        links {
            "openal",
        }

    filter { "system:linux", "platforms:x64" }
        targetdir "../bin/linux-x86_64"

    filter { "system:linux", "platforms:native" }
        targetdir "../bin/linux-native"

    filter "system:linux"
        targetname "PanzerBFG"
        files {
            "../engine/sys/sys_main.c",
            "../engine/sys/sys_unix.c",
            "../engine/sys/con_log.c",
            "../engine/sys/con_passive.c",
            "../engine/sys/sdl_gamma.c",
            "../engine/sys/sdl_glimp.c",
            "../engine/sys/sdl_input.c",
            "../engine/sys/sdl_snd.c",
        }
        links {
            "GL",
        }
        defines {
            "PNG_NO_ASSEMBLER_CODE",
        }

-- RTCW SP game
include "../games/default/game"
include "../games/default/cgame"
include "../games/default/ui"
