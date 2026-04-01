--
-- RBQUAKE-3 build configuration script
-- 
solution "PanzerBFG"
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

-- Doom 3 BFG idlib
include "../rbdoom/idlib"

-- main engine code
project "PanzerBFG"
    targetname "PanzerBFG"
    language "C++"
    kind "WindowedApp"
    files {
        "../shared/**.c", "../shared/**.h",
        "../engine/client/*.c", "../engine/client/*.h",
        "../engine/server/*.c", "../engine/server/*.h",
        "../engine/sound/*.c", "../engine/sound/*.h",
        "../engine/qcommon/**.h", 
        "../engine/qcommon/cmd.c",
        "../engine/qcommon/common.c",
        "../engine/qcommon/cvar.c",
        "../engine/qcommon/files.c",
        "../engine/qcommon/huffman.c",
        "../engine/qcommon/md4.c",
        "../engine/qcommon/md5.c",
        "../engine/qcommon/msg.c",
        "../engine/qcommon/puff.c",
        "../engine/qcommon/vm.c",
        -- "../engine/qcommon/vm_x86.c",
        "../engine/qcommon/net_*.c",
        -- "../engine/qcommon/unzip.c",
		"../engine/sys/sys_local.h",
        "../engine/collision/*.c", "../engine/collision/*.h",
        "../engine/botlib/*.c", "../engine/botlib/*.h",
        "../engine/splines/*.cpp", "../engine/splines/*.c", "../engine/splines/*.h",
		"../rbdoom/rbdoom_precompiled.h",
		"../rbdoom/framework/*.cpp", "../rbdoom/framework/*.h",
		"../rbdoom/decls/*.cpp", "../rbdoom/decls/*.h",		
		
		"../libs/glew/src/glew.c",
		"../libs/glew/include/GL/glew.h",
        "../libs/jpeg/**.c", "../libs/jpeg/**.h",
        "../libs/png/**.c", "../libs/png/**.h",
        "../libs/zlib/**.cpp", "../libs/zlib/**.c", "../libs/zlib/**.h",
		"../libs/mikktspace/*.c", "../libs/mikktspace/*.h",
		"../libs/imgui/*.cpp", "../libs/imgui/*.h",
        "../libs/ogg/src/bitwise.c",
        "../libs/ogg/src/framing.c",
        "../libs/vorbis/lib/mdct.c",
        "../libs/vorbis/lib/smallft.c",
        "../libs/vorbis/lib/block.c",
        "../libs/vorbis/lib/envelope.c",
        "../libs/vorbis/lib/window.c",
        "../libs/vorbis/lib/lsp.c",
        "../libs/vorbis/lib/lpc.c",
        "../libs/vorbis/lib/analysis.c",
        "../libs/vorbis/lib/synthesis.c",
        "../libs/vorbis/lib/psy.c",
        "../libs/vorbis/lib/info.c",
        "../libs/vorbis/lib/floor1.c",
        "../libs/vorbis/lib/floor0.c",
        "../libs/vorbis/lib/res0.c",
        "../libs/vorbis/lib/mapping0.c",
        "../libs/vorbis/lib/registry.c",
        "../libs/vorbis/lib/codebook.c",
        "../libs/vorbis/lib/sharedbook.c",
        "../libs/vorbis/lib/lookup.c",
        "../libs/vorbis/lib/bitrate.c",
        "../libs/vorbis/lib/vorbisfile.c",
    }
    includedirs {
        "../shared",
		"../shared/botshared",
        "../libs/zlib",
        "../libs/gl3w/include",
        "../libs/ft2/include",
        "../libs/ogg/include",
        "../libs/vorbis/include",
        "../libs/sdl2/include",
		
		-- rbdoom specific
		"../libs/glew/include",
		"../libs/imgui",
		"../libs/mikktspace",
		"../libs/stb",
		"../rbdoom/idlib",
		"../rbdoom",
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
		"idlib"
	}
    
    --
    -- Platform Configurations
    --
        
    --
    -- Options Configurations
    --   
	-- Optional: Pre‑compiled header support (MSVC / Clang / GCC)
	if _OPTIONS["with-bfg"] then
		defines { "USE_BFG_RENDERER" }
		files { 
		"../rbdoom/renderer/**.c", "../rbdoom/renderer/**.cpp", "../rbdoom/renderer/**.h",
		"../engine/renderer/tr_public.h", 
		}
	else
		files { "../engine/renderer/**.c", "../engine/renderer/**.cpp", "../engine/renderer/**.h", }
    end
	
	if _OPTIONS["with-pch"] then
        pchheader "rbdoom_precompiled.h"
        pchsource "../rbdoom/framework/Engine_precompiled.cpp"

        -- Force‑include the PCH for compilers that need an explicit flag.
        filter { "action:gcc or clang" }
            forceincludes { "precompiled.h" }
        filter { "files:**.c or files:../engine/splines/*.cpp or files:../rbdoom/renderer/DXT/DXTDecoder.cpp or files:../libs/imgui/*.cpp" }
			flags { "NoPCH" }
    end
	
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
            "../engine/sys/sys_main.c",
            "../engine/sys/sys_win32.c",
            "../engine/sys/con_log.c",
            "../engine/sys/con_win32.c",
            "../engine/sys/sdl_gamma.c",
            "../engine/sys/sdl_glimp.c",
            "../engine/sys/sdl_input.c",
            "../engine/sys/sdl_snd.c",
            "../engine/sys/qe3.ico",
            "../engine/sys/win_resource.rc",
        }
        defines {
            "USE_OPENAL",
            "WIN32",
            "_CRT_SECURE_NO_WARNINGS",
        }
        includedirs {
            "../libs/sdl2/include",
            "../libs/openal/include",
        }
        links {
            "SDL2",
            "SDL2main",
            "winmm",
            "wsock32",
            "opengl32",
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