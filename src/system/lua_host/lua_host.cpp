/**
 * @file lua_host.cpp
 * @brief Embedded Lua runtime for MeowKit — implementation (Phase 1).
 */
#include "lua_host.hpp"

#include "esp_heap_caps.h"

extern "C" {
#include "lauxlib.h"
#include "lualib.h"
}

namespace {

/**
 * Lua allocator routed to external PSRAM.
 *
 * The ESP32-S3-WROOM-1-N16R8 has 8 MB of OPI PSRAM but only ~320 KB of
 * internal DRAM shared with WiFi/BT/LVGL. Keeping the whole VM heap in SPIRAM
 * leaves internal RAM for the rest of the system. heap_caps_realloc handles
 * malloc (ptr == nullptr), resize, and — with nsize 0 handled below — free.
 */
void* psram_alloc(void* /*ud*/, void* ptr, size_t /*osize*/, size_t nsize)
{
    if (nsize == 0) {
        heap_caps_free(ptr);
        return nullptr;
    }
    return heap_caps_realloc(ptr, nsize, MALLOC_CAP_SPIRAM);
}

/**
 * Open only the embedded-safe standard libraries. Because we register these
 * individually rather than calling luaL_openlibs(), the io/os/package objects
 * are never referenced and the linker drops them entirely — so system(),
 * popen() and the dynamic loader never make it into the image.
 */
void openSafeLibs(lua_State* L)
{
    static const luaL_Reg kLibs[] = {
        { LUA_GNAME,       luaopen_base },
        { LUA_TABLIBNAME,  luaopen_table },
        { LUA_STRLIBNAME,  luaopen_string },
        { LUA_MATHLIBNAME, luaopen_math },
        { LUA_UTF8LIBNAME, luaopen_utf8 },
        { LUA_COLIBNAME,   luaopen_coroutine },
        { nullptr, nullptr }
    };
    for (const luaL_Reg* lib = kLibs; lib->func; ++lib) {
        luaL_requiref(L, lib->name, lib->func, 1);
        lua_pop(L, 1); // requiref leaves the module on the stack
    }
}

} // namespace

namespace lua_host {

lua_State* newState()
{
    lua_State* L = lua_newstate(psram_alloc, nullptr);
    if (L == nullptr) {
        return nullptr;
    }
    openSafeLibs(L);
    return L;
}

String selfTest()
{
    const size_t psram_before = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

    lua_State* L = newState();
    if (L == nullptr) {
        return String("FAIL lua_newstate() returned null (PSRAM exhausted?)");
    }

    // Exercise tables, numeric + generic for-loops, and string building.
    // Expected result: sum of squares 1..10 = 385.
    static const char* kScript =
        "local t = {}\n"
        "for i = 1, 10 do t[i] = i * i end\n"
        "local sum = 0\n"
        "for _, v in ipairs(t) do sum = sum + v end\n"
        "return _VERSION .. ' sum_of_squares_1..10=' .. sum\n";

    String result;
    if (luaL_dostring(L, kScript) == LUA_OK) {
        result = String("OK ") + lua_tostring(L, -1);
    } else {
        result = String("ERR ") + lua_tostring(L, -1);
    }

    const int vm_kb = lua_gc(L, LUA_GCCOUNT, 0);
    lua_close(L);

    const size_t psram_after = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    const long delta_bytes = (long)psram_before - (long)psram_after;

    result += String(" | VM=") + vm_kb + "KB";
    result += String(" | PSRAM free=") + (int)(psram_after / 1024) + "KB";
    result += String(" | leak=") + delta_bytes + "B";
    return result;
}

} // namespace lua_host
