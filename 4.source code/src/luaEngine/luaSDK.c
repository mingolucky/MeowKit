#include "luaSDK.h"
#include <stdio.h>

LUALIB_API int lua_TestFuncTable(lua_State *L)
{
    printf("Lua calls C function\n");
    return 0;
}

LUALIB_API int lua_TestFuncTable2(lua_State *L)
{
    int num1 = luaL_checkinteger(L, 1);
    printf("传入参数: %d\n", num1);
    lua_pushinteger(L, num1 * 2); // Returns twice the input parameter
    return 1;
}

// Use luaL_Reg to batch register functions
static const luaL_Reg methods[] = {
    {"func1", lua_TestFuncTable},
    {"func2", lua_TestFuncTable2},
    {NULL, NULL} // End marker
};

LUAMOD_API int Lua_ModelTest(lua_State *L)
{
    // Create a table
    lua_newtable(L);

    // Register functions in batch to the table
    luaL_setfuncs(L, methods, 0); 
    return 1;
}