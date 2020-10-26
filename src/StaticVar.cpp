#include <windows.h>
#include <map>
#include <string>
#include <sstream>

#include <lua.hpp>

#include "AnyLuaDataStorage.h"

#define LUAW32_API  extern "C" __declspec(dllexport) 

const char* LIB_NAMESPACE = "stv";
const char* LIB_REGISTRY_NAME_SPACE_KEY_PREFIX = "stv.ns.";

typedef std::map<std::string, CAnyLuaDataStorage> VAR_CONTAINER;
typedef std::string NAME_SPACE_KEY;
typedef std::map<NAME_SPACE_KEY, VAR_CONTAINER> NAME_SPACE_VAR_CONTAINER;

//ToDo: тип function писать через lua_dump

////////////////////////////////////////////////////////////////////////////////

static VAR_CONTAINER GloVarContainer;
static NAME_SPACE_VAR_CONTAINER NameSpaceVarContainer;
thread_local std::string RegistryKeyNameForTheThread(LIB_REGISTRY_NAME_SPACE_KEY_PREFIX);
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
		case DLL_THREAD_ATTACH:
			// make uniq reg key name for new thread of main()
			char buf[_MAX_U64TOSTR_BASE10_COUNT];
			if (_itoa_s(GetCurrentThreadId(), buf, 10) == 0) {
				RegistryKeyNameForTheThread += buf;
			}
			break;
	}
	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////

bool getCurrentNameSpace(lua_State *L, std::string& outStr) {
	lua_pushstring(L, RegistryKeyNameForTheThread.c_str());
	lua_gettable(L, LUA_REGISTRYINDEX);

	if (lua_isnil(L, -1))
		return false;

	outStr = lua_tostring(L, -1);
	return true;
}

void setCurrentNameSpace(lua_State *L, const char* inStr) {
	lua_pushstring(L, RegistryKeyNameForTheThread.c_str());
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

	return NameSpaceVarContainer[current_name_space];
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

	const VAR_CONTAINER& current_container = getCurrentContainer(L);

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

	lua_createtable(L, 0, int(current_container.size()));

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

static int lua_Clear(lua_State *L) {
	CLocker locker;  // Don't remove it!

	VAR_CONTAINER& current_container = getCurrentContainer(L);
	current_container.clear();

	return(0);
}

static int lua_ClearAll(lua_State *L) {
	CLocker locker;  // Don't remove it!

	GloVarContainer.clear();
	NameSpaceVarContainer.clear();

	return(0);
}


////////////////////////////////////////////////////////////////////////////////

static struct luaL_reg lib_functions[] = {
	{"SetVar", lua_SetVar},
	{"GetVar", lua_GetVar},
	{"SetVarList", lua_SetVarList},
	{"GetVarList", lua_GetVarList},
	{"UseNameSpace", lua_UseNameSpace},
	{"GetCurrentNameSpace", lua_GetCurrentNameSpace},
	{"Clear", lua_Clear},
	{"ClearAll", lua_ClearAll},
	{nullptr, nullptr}
};

LUAW32_API  int luaopen_StaticVar(lua_State *L)
{
	luaL_register(L, LIB_NAMESPACE, lib_functions);

	return 1;
}