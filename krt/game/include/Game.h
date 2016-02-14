#pragma once

// Game things go here!

#define GAME_NUM_STREAMING_CHANNELS     4

#include "Streaming.h"

namespace krt
{

class Game
{
public:
    Game( void );
    ~Game( void );

    std::wstring GetGamePath( std::wstring relDir );

    void RunCommandFile( std::wstring relDir );

    inline streaming::StreamMan& GetStreaming( void )           { return this->streaming; }

private:
    void LoadIDEFile( std::wstring relPath );
    void LoadIPLFile( std::wstring relPath );

    std::wstring gameDir;

    streaming::StreamMan streaming;
};

extern Game *theGame;

};