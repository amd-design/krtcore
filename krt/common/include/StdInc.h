#pragma once

// common standard library headers

// containers
#include <vector>

#include <map>
#include <unordered_map>

#include <set>
#include <unordered_set>

#include <thread>
#include <mutex>

#include <memory>

#include <algorithm>
#include <numeric>

#include <string>

// integer types
#include <stdint.h>

#include <utils/Assert.h>

// useful utilities
#include <utils/LoopRange.h>
#include <utils/StringConvert.h>
#include <utils/LockUtil.h>

// manually include RW stuff as we don't want D3D lingering in our global namespace
//#include <rw.h>
#include "src/rwbase.h"
#include "src/rwplugin.h"
#include "src/rwpipeline.h"
#include "src/rwobjects.h"
#include "src/rwps2.h"
//#include "src/rwxbox.h"
//#include "src/rwd3d.h"
//#include "src/rwd3d8.h"
//#include "src/rwd3d9.h"
//#include "src/rwogl.h"

#include <RwHelpers.h>

#include <Console.Base.h>