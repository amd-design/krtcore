#include "StdInc.h"
#include "Game.h"

#include <cstdio>
#include <iostream>
#include <fstream>

namespace krt
{

Game *theGame = NULL;

Game::Game( void ) : streaming( GAME_NUM_STREAMING_CHANNELS )
{
    assert( theGame == NULL );

    // Detect where the game is installed.
    // For now, I guess we do it manually.

    const char *computerName = getenv( "COMPUTERNAME" );

    if ( stricmp( computerName, "FALLARBOR" ) == 0 )
    {
        // Bas' thing.
        this->gameDir = L"S:\\Games\\Steam\\steamapps\\common\\Grand Theft Auto 3\\";
    }
    else if ( stricmp( computerName, "DESKTOP" ) == 0 )
    {
        // Martin's thing.
        this->gameDir = L"C:\\Program Files (x86)\\Rockstar Games\\GTA San Andreas\\";
    }
    else
    {
        // Add your own, meow.
    }

    // Load game files!
    this->RunCommandFile( L"DATA\\GTA.DAT" );

    // We can only have one game :)
    theGame = this;
}

Game::~Game( void )
{
    // TODO.

    // There is no more game.
    theGame = NULL;
}

std::wstring Game::GetGamePath( std::wstring relPath )
{
    // Get some sort of relative directory from the game directory.
    // Note that we want the gameDir to have a slash at the end!
    return ( this->gameDir + relPath );
}

inline bool is_white_space( char c )
{
    return ( c == ' ' || c == '\t' );
}

static inline std::list <std::string> split_cmd_line( std::string line )
{
    std::list <std::string> argsOut;

    std::string currentArg;

    for ( char c : line )
    {
        if ( !is_white_space( c ) )
        {
            currentArg += c;
        }
        else if ( currentArg.empty() == false )
        {
            argsOut.push_back( std::move( currentArg ) );
        }
    }

    if ( currentArg.empty() == false )
    {
        argsOut.push_back( std::move( currentArg ) );
    }

    return argsOut;
}

static inline std::vector <std::string> split_csv_args( std::string line )
{
    std::vector <std::string> argsOut;

    const char *linePtr = line.c_str();

    const char *curLineStart = NULL;
    const char *curLineEnd = NULL;

    while ( char c = *linePtr )
    {
        bool wantsTokenTermination = false;
        bool wantsTokenAdd = false;

        if ( c == ',' )
        {
            size_t curLineLen = ( curLineEnd - curLineStart );

            if ( curLineLen != 0 )
            {
                wantsTokenTermination = true;
                wantsTokenAdd = true;
            }
        }
        else
        {
            if ( curLineStart == NULL )
            {
                if ( !is_white_space( c ) )
                {
                    curLineStart = linePtr;
                }
            }
            else
            {
                if ( is_white_space( c ) )
                {
                    wantsTokenTermination = true;
                }
            }
        }

        if ( wantsTokenTermination )
        {
            curLineEnd = linePtr;
        }

        if ( wantsTokenAdd )
        {
            argsOut.push_back( std::string( curLineStart, curLineEnd ) );

            curLineStart = NULL;
            curLineEnd = NULL;
        }

        linePtr++;
    }

    // Push ending token.
    {
        size_t curLineLen = ( linePtr - curLineStart );

        if ( curLineLen != 0 )
        {
            argsOut.push_back( std::string( curLineStart, linePtr ) );
        }
    }

    return argsOut;
}

static inline std::string get_cfg_line( std::fstream& in )
{
    std::string line;

    std::getline( in, line );

    size_t realStartPos = 0;

    size_t pos = 0;

    for ( char c : line )
    {
        if ( !is_white_space( c ) )
        {
            realStartPos = pos;
            break;
        }
        
        pos++;
    }

    return line.substr( pos );
}

inline bool ignore_line( const std::string& line )
{
    return ( line.empty() || line.front() == '#' );
}

void Game::LoadIDEFile( std::wstring relPath )
{
    std::wstring dataFilePath = GetGamePath( relPath );

    std::fstream ideFile( dataFilePath, std::ios::in );

    if ( !ideFile.good() )
    {
        return;
    }

    // Gotta process some IDE entries!!
    bool isInSection = false;
    bool isObjectSection = false;

    while ( !ideFile.eof() )
    {
        std::string cfgLine;

        cfgLine = get_cfg_line( ideFile );

        // Not that hard to process, meow.
        if ( ignore_line( cfgLine ) == false )
        {
            if ( !isInSection )
            {
                if ( cfgLine == "objs" )
                {
                    isObjectSection = true;

                    isInSection = true;
                }
            }
            else
            {
                if ( cfgLine == "end" )
                {
                    isObjectSection = false;

                    isInSection = false;
                }
                else
                {
                    std::vector <std::string> args = split_csv_args( cfgLine );

                    if ( isObjectSection )
                    {

                    }
                }
            }
        }
    }
}

static inline std::wstring ansi_to_wide( std::string str )
{
    return std::wstring( str.begin(), str.end() );
}

// Data file loaders!
void Game::LoadIPLFile( std::wstring relPath )
{
    std::wstring dataFilePath = GetGamePath( relPath );


}

void Game::RunCommandFile( std::wstring relPath )
{
    std::wstring cmdFilePath = GetGamePath( relPath );

    std::fstream cmdFile( cmdFilePath, std::ios::in );

    if ( cmdFile.good() == false )
    {
        // We failed for some reason :(
        return;
    }

    // Process commands in lines!
    while ( cmdFile.eof() == false )
    {
        std::string cmdLine;
        
        // Skip whitespace.
        cmdLine = get_cfg_line( cmdFile );

        // Ignore commented lines.
        if ( !ignore_line( cmdLine ) )
        {
            // Split it into arguments.
            std::list <std::string> args = split_cmd_line( cmdLine );

            if ( args.empty() == false )
            {
                std::string cmdName = args.front();

                args.pop_front();

                // Execute!
                //waiting for lovely's command impl!

                if ( cmdName == "IDE" )
                {
                    if ( args.size() >= 1 )
                    {
                        std::wstring unicodePath = ansi_to_wide( args.front() );

                        LoadIDEFile( unicodePath );
                    }
                }
            }
        }
    }
}

};