-- KRT project workspace
newoption {
    trigger = "with-librw",
    value = "path",
    description = "Path to librw (default ../vendor/framework/librw)"
}

newoption {
    trigger = "with-librwgta",
    value = "path",
    description = "Path to librwgta (default ../vendor/framework/librwgta)"
}

if not _OPTIONS['with-librw'] then
    _OPTIONS['with-librw'] = '../vendor/framework/librw/'
end

if not _OPTIONS['with-librwgta'] then
    _OPTIONS['with-librwgta'] = '../vendor/framework/librwgta/'
end

workspace 'krt'
    configurations { 'Debug', 'Release' }

    flags { 'Symbols', 'Unicode' }

    includedirs { 'common/include/' }

    architecture 'x64'

    location 'build/windows/'

    filter 'system:windows'
        includedirs { 'dependencies/include/wtl/' }
        defines { 'RW_D3D9' }

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
            _OPTIONS['with-librw'],
            _OPTIONS['with-librwgta']
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
            'game/**.cpp',
            '**.natvis'
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
            _OPTIONS['with-librw'] .. 'src'
        }

        files
        {
            _OPTIONS['with-librw'] .. '/src/**.h',
            _OPTIONS['with-librw'] .. '/src/**.cpp',
            _OPTIONS['with-librw'] .. 'rw.h'
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
            _OPTIONS['with-librw'],
            _OPTIONS['with-librwgta'] .. '/src'
        }

        files
        {
            _OPTIONS['with-librwgta'] .. '/src/**.h',
            _OPTIONS['with-librwgta'] .. '/src/**.cpp'
        }

        undefines
        {
            "NDEBUG"
        }

        warnings "Off"
