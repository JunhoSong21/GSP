#include <iostream>
#include "include/lua.hpp"
#pragma comment (lib, "lua55.lib")

int main()
{
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	luaL_loadfile(L, "dragon.lua");
	int error = lua_pcall(L, 0, 0, 0);
	if (error) {
		std::cout << "Error:" << lua_tolstring(L, (-1), 0);
		lua_pop(L, 1);
	}

	lua_getglobal(L, "plustwo");
	lua_pushnumber(L, 5);
	lua_pcall(L, 1, 1, 0);
	int result = (int)lua_tointeger(L, -1);
	std::cout << "Result:" << result << std::endl;

	lua_pop(L, 1);

	lua_close(L);
}