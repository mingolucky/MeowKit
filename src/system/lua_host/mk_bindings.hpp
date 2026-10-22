/**
 * @file mk_bindings.hpp
 * @brief The `mk.*` Lua API — device bindings for MeowKit plugin scripts.
 *
 * Installs a global `mk` table into a Lua state, backed by a DEVICES*. Phase 2
 * surface (orchestration-level; heavy work stays in C++):
 *   mk.display  clear/text/pixel/line/rect/circle/width/height
 *   mk.input    down(key)            -- "a","b","up","down","left","right"
 *   mk.fs       read/write/list/exists   (sandboxed to the SD card)
 *   mk.time     ms()/delay(ms)
 *   mk.sys      log/exit/heap/psram
 *   mk.color(r,g,b) -> 0xRRGGBB
 *
 * Single running script at a time: the device context is process-global.
 */
#pragma once

extern "C" {
#include "lua.h"
}

class DEVICES;

namespace mk_bindings {

/// Install the `mk` global table into L, bound to `device`. Clears the exit flag.
void install(lua_State* L, DEVICES* device);

/// True if a script called mk.sys.exit() since the last clearExit().
bool exitRequested();

/// Reset the exit-request flag (call when (re)launching a script).
void clearExit();

} // namespace mk_bindings
