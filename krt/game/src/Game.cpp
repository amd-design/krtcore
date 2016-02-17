#include "StdInc.h"
#include "Game.h"

#include <cstdio>
#include <iostream>
#include <fstream>

#include "Console.h"
#include "Console.CommandHelpers.h"

#include "CdImageDevice.h"
#include "vfs\Manager.h"

#include <src/rwgta.h>

namespace krt
{

Game *theGame = NULL;

Game::Game( void ) : streaming( GAME_NUM_STREAMING_CHANNELS ), texManager( streaming ), modelManager( streaming, texManager )
{
    assert( theGame == NULL );

    // We can only have one game :)
    theGame = this;

    // Initialize RW.
    rw::platform = rw::PLATFORM_D3D9;
    rw::loadTextures = false;

    gta::attachPlugins();

    // Detect where the game is installed.
    // For now, I guess we do it manually.

    const char *computerName = getenv( "COMPUTERNAME" );

    if ( stricmp( computerName, "FALLARBOR" ) == 0 )
    {
        // Bas' thing.
        this->gameDir = "S:\\Games\\CleanSA\\GTA San Andreas\\";
    }
    else if ( stricmp( computerName, "DESKTOP" ) == 0 )
    {
        // Martin's thing.
        this->gameDir = "C:\\Program Files (x86)\\Rockstar Games\\GTA San Andreas\\";
    }
    else
    {
        // Add your own, meow.
    }

    // Mount the main game archive.
    this->LoadIMG( "MODELS\\GTA3.IMG" );
    this->LoadIMG( "MODELS\\GTA_INT.IMG" );

    // Load game files!
    this->RunCommandFile( "DATA\\GTA.DAT" );

    // Do a test that loads all game models.
    //modelManager.LoadAllModels();
}

Game::~Game( void )
{
    // TODO.

    // There is no more game.
    theGame = NULL;
}

void make_lower( std::string& str )
{
    std::transform( str.begin(), str.end(), str.begin(), ::tolower );
}

inline std::string get_file_name( std::string path, std::string *extOut = NULL )
{
    const char *pathIter = path.c_str();

    const char *fileNameStart = pathIter;
    const char *fileExtEnd = NULL;

    while ( char c = *pathIter )
    {
        if ( c == '\\' || c == '/' )
        {
            fileNameStart = ( pathIter + 1 );
        }

        if ( c == '.' )
        {
            fileExtEnd = pathIter;
        }

        pathIter++;
    }

    if ( fileExtEnd != NULL )
    {
        if ( extOut )
        {
            *extOut = std::string( fileExtEnd + 1, pathIter );
        }

        return std::string( fileNameStart, fileExtEnd );
    }

    if ( extOut )
    {
        extOut->clear();
    }

    return std::string( fileNameStart, pathIter );
}

void Game::LoadIMG( std::string relPath )
{
    // Get the name of the IMG and mount it as it is.
    std::string containerName = get_file_name( relPath );

    make_lower( containerName );

    streaming::CdImageDevice *imgDevice = new streaming::CdImageDevice();

    if ( imgDevice == NULL )
    {
        // We kinda cannot, so bail.
        return;
    }

    bool didOpenImage = imgDevice->OpenImage( this->GetGamePath( relPath ) );

    if ( !didOpenImage )
    {
        delete imgDevice;

        return;
    }

    vfs::DevicePtr myDevPtr( imgDevice );

    // Temporary for now.
    std::string pathPrefix = this->GetDevicePathPrefix();

    vfs::Mount( myDevPtr, pathPrefix );

    // We now have to parse the contents of the IMG archive. :)
    {
        vfs::FindData findData;

        auto findHandle = imgDevice->FindFirst( pathPrefix, &findData );

        if ( findHandle != vfs::Device::InvalidHandle )
        {
            do
            {
                std::string fileExt;

                std::string fileName = get_file_name( findData.name, &fileExt );

                std::string realDevPath = ( pathPrefix + findData.name );

                // We process things depending on file extension, for now.
                if ( stricmp( fileExt.c_str(), "TXD" ) == 0 )
                {
                    // Register this TXD.
                    this->GetTextureManager().RegisterResource( fileName, myDevPtr, realDevPath );
                }
            }
            while ( imgDevice->FindNext( findHandle, &findData ) );

            imgDevice->FindClose( findHandle );
        }
    }

    // Alright, let's pray for the best!
}

std::string Game::GetGamePath( std::string relPath )
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

static inline std::string get_cfg_line( std::istream& in )
{
    std::string line;

    std::getline( in, line );

    const char *lineIter = line.c_str();

    const char *lineStartPos = lineIter;

    while ( char c = *lineIter )
    {
        if ( !is_white_space( c ) )
        {
            if ( lineStartPos == NULL )
            {
                lineStartPos = lineIter;
            }
        }

        if ( c == '\r' )
        {
            break;
        }

        lineIter++;
    }

    return std::string( lineStartPos, lineIter );
}

inline bool ignore_line( const std::string& line )
{
    return ( line.empty() || line.front() == '#' );
}

// Yea rite, lets just do this.
inline std::stringstream get_file_stream( std::string relPath )
{
    std::string dataFilePath = theGame->GetGamePath( relPath );

    vfs::StreamPtr filePtr = vfs::OpenRead( relPath );

    std::vector <std::uint8_t> fileData = filePtr->ReadToEnd();

    return std::stringstream( std::string( fileData.begin(), fileData.end() ), std::ios::in );
}

void Game::LoadIDEFile( std::string relPath )
{
    std::stringstream ideFile = get_file_stream( GetGamePath( relPath ) );

    // Gotta process some IDE entries!!
    bool isInSection = false;
    bool isObjectSection = false;
    bool isTexParentSection = false;

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
                else if ( cfgLine == "txdp" )
                {
                    isTexParentSection = true;

                    isInSection = true;
                }
            }
            else
            {
                if ( cfgLine == "end" )
                {
                    isObjectSection = false;
                    isTexParentSection = false;

                    isInSection = false;
                }
                else
                {
                    std::vector <std::string> args = split_csv_args( cfgLine );

                    if ( isObjectSection )
                    {
                        if ( args.size() == 5 )
                        {
                            // Process this. :)
                            streaming::ident_t id = atoi( args[0].c_str() );
                            const std::string& modelName = args[1];
                            const std::string& txdName = args[2];
                            float lodDistance = atof( args[3].c_str() );
                            std::uint32_t flags = atoi( args[4].c_str() );

                            // TODO: meow, actually implement this.
                            // need a good resource location idea for this :3

                            modelManager.RegisterResource( id, modelName, txdName, lodDistance, flags, modelName + ".dff" );
                        }
                        // TODO: is there any different format with less or more arguments?
                        // maybe for IPL?
                    }
                    else if ( isTexParentSection )
                    {
                        if ( args.size() == 2 )
                        {
                            const std::string& txdName = args[0].c_str();
                            const std::string& txdParentName = args[1].c_str();

                            // Register a TXD dependency.
                            texManager.SetTexParent( txdName, txdParentName );
                        }
                    }
                }
            }
        }
    }
}

ConsoleCommand ideLoadCmd( "IDE",
    [] ( const std::string& fileName )
{
    theGame->LoadIDEFile( fileName );
});

// Data file loaders!
void Game::LoadIPLFile( std::string relPath )
{
    std::string dataFilePath = GetGamePath( relPath );

    
}

void Game::RunCommandFile( std::string relPath )
{
	vfs::StreamPtr stream = vfs::OpenRead(this->GetGamePath(relPath));
	std::vector<uint8_t> string = stream->ReadToEnd();

	console::Context localConsole;

	ConsoleCommand iplLoadCmd(&localConsole, "IPL",
							  [] (const std::string& fileName)
	{
		theGame->LoadIPLFile(fileName);
	});

	ConsoleCommand imgMountCmd(&localConsole, "IMG",
							   [] (const std::string& path)
	{
		theGame->LoadIMG(path);
	});

	localConsole.AddToBuffer(std::string(reinterpret_cast<char*>(string.data()), string.size()));
	localConsole.ExecuteBuffer();
}

};