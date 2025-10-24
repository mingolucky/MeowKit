#include "luaTest.h"
#include <stdio.h>

LUALIB_API int lua_TestFuncTable(lua_State *L)
{
    printf("lua调用C函数\n");
    return 0;
}

LUALIB_API int lua_TestFuncTable2(lua_State *L)
{
    int num1 = luaL_checkinteger(L, 1);
    printf("传入参数: %d\n", num1);
    lua_pushinteger(L, num1 * 2); // 返回参数的两倍
    return 1;
}

// 使用luaL_Reg批量注册函数
static const luaL_Reg methods[] = {
    {"func1", lua_TestFuncTable},
    {"func2", lua_TestFuncTable2},
    {NULL, NULL} // 结束标志
};

LUAMOD_API int Lua_ModelTest(lua_State *L)
{
    // 创建一个表
    lua_newtable(L);

    // 批量注册函数到表中
    luaL_setfuncs(L, methods, 0); // 第二个参数是methods数组，第三个参数是upvalues的数量，这里为0
    return 1;
}