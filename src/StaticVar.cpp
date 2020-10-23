#include <windows.h>
#include <map>
#include <string>
#include <deque>

#include <lua.hpp>

#include "AnyLuaDataStorage.h"

#define LUAW32_API  extern "C" __declspec(dllexport) 

const char* LIB_NAMESPACE = "stv";
const char* LIB_REGISTRY_KEY_NAME_SPACE = "StaticVar.Current.Name.Space";

typedef std::map<std::string, CAnyLuaDataStorage> VAR_CONTAINER;
typedef std::pair<std::string,VAR_CONTAINER> NAME_SPACE_VAR_CONTAINER_ELEM;
typedef std::deque<NAME_SPACE_VAR_CONTAINER_ELEM> NAME_SPACE_VAR_CONTAINER;

//ToDo: про таблицы
//      индекс надо уметь сохранять в объекте с разным типом!
//      в нем методы: FromLuaStack(L)  или FromLua(L)
//                    ToLuaStack(L)        ToLua(L)
// индекс бывает любого типа! bool, number, string, table, function
// int и double в индексе можно не различать, проверено
// тип function писать через lua_dump

////////////////////////////////////////////////////////////////////////////////

static VAR_CONTAINER GloVarContainer;
static NAME_SPACE_VAR_CONTAINER NameSpaceVarContainer;
static bool IsGloVarContainerOnly = true;

////////////////////////////////////////////////////////////////////////////////
class CLocker
{
public:
	CLocker()  { EnterCriticalSection(&CritSection); }
	~CLocker() { LeaveCriticalSection(&CritSection); }
	static CRITICAL_SECTION CritSection;
};
CRITICAL_SECTION CLocker::CritSection;

////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason) {
		case DLL_PROCESS_ATTACH:
			InitializeCriticalSection(&CLocker::CritSection);
			break;
		case DLL_PROCESS_DETACH:
			DeleteCriticalSection(&CLocker::CritSection);
			break;
	}
	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////

bool getCurrentNameSpace(lua_State *L, std::string& outStr) {
	lua_pushstring(L, LIB_REGISTRY_KEY_NAME_SPACE);
    lua_gettable(L, LUA_REGISTRYINDEX);

	if (lua_isnil(L, -1))
		return false;

	outStr = lua_tostring(L, -1);
	return true;
}

void setCurrentNameSpace(lua_State *L, const char* inStr) {
	lua_pushstring(L, LIB_REGISTRY_KEY_NAME_SPACE);
	if (inStr && inStr[0])
		lua_pushstring(L, inStr);
	else
		lua_pushnil(L);
	lua_settable(L, LUA_REGISTRYINDEX);

	IsGloVarContainerOnly = false;
}

VAR_CONTAINER& getCurrentContainer(lua_State *L) {
	if (IsGloVarContainerOnly)
		return GloVarContainer;

	std::string current_name_space;
	if (!getCurrentNameSpace(L, current_name_space))
		return GloVarContainer;

	for (NAME_SPACE_VAR_CONTAINER::iterator it = NameSpaceVarContainer.begin();
	     it != NameSpaceVarContainer.end();
		 ++it) {
		if (current_name_space == it->first)
			return it->second;
	}
	NameSpaceVarContainer.push_back(NAME_SPACE_VAR_CONTAINER_ELEM(current_name_space, VAR_CONTAINER()));
	return NameSpaceVarContainer.back().second;
}

////////////////////////////////////////////////////////////////////////////////
// Library Lua functions
////////////////////////////////////////////////////////////////////////////////

static int lua_SetVar(lua_State *L) {
	const int numArg = lua_gettop(L);
	const char *varName = luaL_checkstring(L, 1);

	CLocker locker;  // Don't remove it!

	VAR_CONTAINER& current_container = getCurrentContainer(L);
	if (numArg < 2 || lua_isnil(L, 2)) {
		VAR_CONTAINER::const_iterator it = current_container.find(varName);
		if (it != current_container.end())
			current_container.erase(it);
	}
	else {
		current_container[varName] =  CAnyLuaDataStorage(L, 2);
	}

	return(0);
}

static int lua_GetVar(lua_State *L) {
	const char *varName = luaL_checkstring(L, 1);

	CLocker locker;  // Don't remove it!

	VAR_CONTAINER& current_container = getCurrentContainer(L);

	VAR_CONTAINER::const_iterator it = current_container.find(varName);
	if (it != current_container.end()) {
		it->second.PushToLua(L);
	}
	else {
		lua_pushnil(L);
	}

	return(1);
}

static int lua_SetVarList(lua_State *L) {
	luaL_checktype(L, 1, LUA_TTABLE);

	CLocker locker;  // Don't remove it!

	VAR_CONTAINER& current_container = getCurrentContainer(L);

	lua_pushnil(L);     // first key of table
	while (lua_next(L, 1)) {
		const char* var_name = lua_tostring(L, -2);  // key
		CAnyLuaDataStorage var_value(L, -1);         // value
		current_container[var_name] = var_value;
		lua_pop(L, 1);  // removes 'value'; keeps 'key' for next iteration
	}

	return(0);
}

static int lua_GetVarList(lua_State *L) {
	CLocker locker;  // Don't remove it!

	const VAR_CONTAINER& current_container = getCurrentContainer(L);

	lua_createtable(L, 0, current_container.size());

	for (VAR_CONTAINER::const_iterator it = current_container.begin();
		it != current_container.end();
		++it) {
			lua_pushstring(L, it->first.c_str());
			it->second.PushToLua(L);
			lua_rawset(L, -3);
	}
	return(1);
}

static int lua_UseNameSpace(lua_State *L) {
	CLocker locker;  // Don't remove it!

	const char * ns = lua_tostring(L, 1);
	setCurrentNameSpace(L, ns);

	return(0);
}

static int lua_GetCurrentNameSpace(lua_State *L) {
	CLocker locker;  // Don't remove it!

	std::string name_space;
	if (getCurrentNameSpace(L, name_space))
		lua_pushstring(L, name_space.c_str());
	else
		lua_pushnil(L);

	return(1);
}

////////////////////////////////////////////////////////////////////////////////

static struct luaL_reg lib_functions[] = {
	{"SetVar", lua_SetVar},
	{"GetVar", lua_GetVar},
	{"SetVarList", lua_SetVarList},
	{"GetVarList", lua_GetVarList},
	{"UseNameSpace", lua_UseNameSpace},
	{"GetCurrentNameSpace", lua_GetCurrentNameSpace},
	{NULL, NULL}
};

LUAW32_API  int luaopen_StaticVar(lua_State *L)
{
	luaL_openlib( L, LIB_NAMESPACE, lib_functions, 0);
	return 0;
}