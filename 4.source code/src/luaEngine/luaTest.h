
#ifndef __LUA_TEST_H__
#define __LUA_TEST_H__

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// 函数声明
LUALIB_API int lua_TestFuncTable(lua_State *L);
LUALIB_API int lua_TestFuncTable2(lua_State *L);
LUAMOD_API int Lua_ModelTest(lua_State *L);

#ifdef __cplusplus
}
#endif

#endif // __LUA_TEST_H__