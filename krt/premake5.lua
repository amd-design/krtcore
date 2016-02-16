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
            'game/include',
            'framework/librw',
            'framework/librwgta'
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
        
        links
        {
            'librw',
            'librwgta'
        }

        filter 'system:windows'
            files { 'common/**.rc' }

        pchsource 'common/src/StdInc.cpp'
        pchheader 'StdInc.h'

    project 'librw'
        targetname 'librw.%{cfg.buildcfg}'
        language 'C++'
        kind 'StaticLib'
        
        includedirs
        {
            'framework/librw/src'
        }
        
        files
        {
            'framework/librw/src/**.h',
            'framework/librw/src/**.cpp',
            'framework/librw/rw.h'
        }
        
        undefines
        {
            "NDEBUG"
        }
        
        warnings "Off"
        
    project 'librwgta'
        targetname 'librwgta.%{cfg.buildcfg}'
        language 'C++'
        kind 'StaticLib'
        
        includedirs
        {
            'framework/librw',
            'framework/librwgta/src'
        }
        
        files
        {
            'framework/librwgta/src/**.h',
            'framework/librwgta/src/**.cpp'
        }
        
        undefines
        {
            "NDEBUG"
        }
        
        warnings "Off"