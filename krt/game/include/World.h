#pragma once

// World class for managing all alive Entities.
#include "Entity.h"

namespace krt
{

struct World
{
    friend struct Entity;

    World( void );
    ~World( void );

private:
    NestedList <Entity> entityList;
};

};