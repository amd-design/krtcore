-- KRT project workspace
workspace 'krt'
    configurations { 'Debug', 'Release' }

    flags { 'Symbols', 'Unicode' }

    includedirs { 'common/include/' }

    architecture 'x64'

    location 'build/windows/'

    filter 'system:windows'
        includedirs { 'dependencies/include/wtl/' }

    filter 'configurations:Debug'
        defines { '_DEBUG', 'KRT_DEBUG' }
        targetdir 'bin/debug/'

    filter 'configurations:Release'
        optimize 'On'
        defines { 'NDEBUG', 'KRT_RELEASE' }
        targetdir 'bin/release/'

    project 'krt'
        targetname 'KRT.%{cfg.buildcfg}'
        language 'C++'
        kind 'ConsoleApp'

        includedirs
        {
            'core/include',
            'streaming/include',
            'game/include'
        }

        files
        {
            'common/**.h',
            'common/**.cpp',
            'core/**.h',
            'core/**.cpp',
            'streaming/**.h',
            'streaming/**.cpp',
            'game/**.h',
            'game/**.cpp'
        }

        filter 'system:windows'
            files { 'common/**.rc' }

        pchsource 'common/src/StdInc.cpp'
        pchheader 'StdInc.h'
