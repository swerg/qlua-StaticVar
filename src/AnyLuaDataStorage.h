#include <lua.hpp>
#include <deque>

class CAnyLuaDataStorage {

typedef std::pair<CAnyLuaDataStorage, CAnyLuaDataStorage>  TABLE_STORAGE_ELEM;
typedef std::deque<TABLE_STORAGE_ELEM>  TABLE_STORAGE;

public:

	CAnyLuaDataStorage()  : m_DataType(LUA_TNONE) {
	}

	CAnyLuaDataStorage(lua_State *L, int idx)  : m_DataType(LUA_TNONE) {
		FromLua(L, idx);
	}

	CAnyLuaDataStorage(const CAnyLuaDataStorage& from) : m_DataType(LUA_TNONE) {  // copy constructor
		CopyFrom(from);
	}

	~CAnyLuaDataStorage() {
		switch (m_DataType) {
			case LUA_TSTRING:
				delete []m_Data.d_buf.ptr;
				break;
			case LUA_TTABLE:
				delete (TABLE_STORAGE*)(m_Data.d_obj);
				break;
		}
	}

	void PushToLua(lua_State *L) const {
		switch (m_DataType) {
			case LUA_TBOOLEAN:
				lua_pushboolean(L, m_Data.d_int);
				break;
			case LUA_TNUMBER:
				lua_pushnumber(L, m_Data.d_double);
				break;
			case LUA_TSTRING:
				lua_pushstring(L, (char*)m_Data.d_buf.ptr);
				break;
			case LUA_TTABLE: {
				const TABLE_STORAGE* tstorage = (TABLE_STORAGE*)m_Data.d_obj;
				lua_createtable(L, 0, tstorage->size());
				for (TABLE_STORAGE::const_iterator it = tstorage->begin();
					it != tstorage->end();
					++it) {
						it->first.PushToLua(L);
						it->second.PushToLua(L);
						lua_rawset(L, -3);
				}}
				break;
			default:
				lua_pushnil(L);
				break;
		}
	}
	
	
	CAnyLuaDataStorage& operator=(const CAnyLuaDataStorage& right) {
		if (this == &right) {
			return *this;
		}
		CopyFrom(right);
		return *this;
	}

private:  // methods

	void CopyFrom(const CAnyLuaDataStorage& from) {
		FreeBufObjIfNeed(from.m_DataType);
		switch (from.m_DataType) {
			case LUA_TBOOLEAN:
			case LUA_TNUMBER:
				m_Data = from.m_Data;
				m_DataType = from.m_DataType;
				break;
			case LUA_TSTRING:
				AllocateBuf(from.m_Data.d_buf.len);
				memcpy(this->m_Data.d_buf.ptr, from.m_Data.d_buf.ptr, this->m_Data.d_buf.len);
				m_DataType = LUA_TSTRING;
				break;
			case LUA_TTABLE:
				AllocateObj(LUA_TTABLE);
				*((TABLE_STORAGE*)m_Data.d_obj) = *((TABLE_STORAGE*)from.m_Data.d_obj);
				break;
			default:  // unknown types
				m_DataType = LUA_TNONE;
				break;
		}
	}

	void FromLua(lua_State *L, int idx) {
		if (idx < 0) {
			idx = lua_gettop(L) + idx + 1;  // fix idx to true index, not relative
		}
		const int newType = lua_type(L, idx);
		FreeBufObjIfNeed(newType);
		switch(newType) {
			case LUA_TBOOLEAN:
				m_Data.d_int = lua_toboolean(L, idx);
				m_DataType = LUA_TBOOLEAN;
				break;
			case LUA_TNUMBER:
				m_Data.d_double = lua_tonumber(L, idx);
				m_DataType = LUA_TNUMBER;
				break;
			case LUA_TTABLE: {
				AllocateObj(LUA_TTABLE);
				TABLE_STORAGE* tstorage = (TABLE_STORAGE*)m_Data.d_obj;
				lua_pushnil(L);     // first key of table
				while (lua_next(L, idx)) {
					tstorage->push_back(TABLE_STORAGE_ELEM(
						CAnyLuaDataStorage(L,-2), CAnyLuaDataStorage(L,-1)  // key,value
					));
					lua_pop(L, 1);  // removes 'value'; keeps 'key' for next iteration
				}}
				break;
			default:  // try convert to LUA_TSTRING
				const char* str = lua_tostring(L, idx);
				const size_t str_len = strlen(str) + 1;
				AllocateBuf(str_len);
				memcpy(m_Data.d_buf.ptr, str, str_len);
				m_DataType = LUA_TSTRING;
				break;
		}
	}

	void FreeBufObjIfNeed(const int newType) {
		if (newType != m_DataType) {
			switch (m_DataType) {
				case LUA_TSTRING:
					m_DataType = LUA_TNONE;
					delete []m_Data.d_buf.ptr;
					m_Data.d_buf.ptr = NULL;
					break;
				case LUA_TTABLE:
					delete (TABLE_STORAGE*)(m_Data.d_obj);
					break;
			}
		}
	}

	void AllocateBuf(const size_t new_len) {  // allocates a new buffer in d_buf
		if (m_DataType == LUA_TSTRING && m_Data.d_buf.ptr) {
			if (   (new_len > m_Data.d_buf.len)
				|| (m_Data.d_buf.len > 32 && m_Data.d_buf.len/new_len > 2)
				) {
				delete []m_Data.d_buf.ptr;
				m_Data.d_buf.ptr = NULL;
			}
		}
		else {
				m_Data.d_buf.ptr = NULL;
		}
		if (!m_Data.d_buf.ptr) {
			m_Data.d_buf.ptr = new char[new_len];
			m_Data.d_buf.len = new_len;
		}
	}

	void AllocateObj(const int newType) {  // creats a new object in d_obj
		if (m_DataType == newType) { // required storage exists, clear only
			switch (m_DataType) {
				case LUA_TTABLE:
					((TABLE_STORAGE*)m_Data.d_obj)->clear();
					break;
			}
		}
		else {                       // m_DataType != newType --> create a new storage object
			switch (newType) {
				case LUA_TTABLE:
					m_Data.d_obj = new TABLE_STORAGE;
					break;
			}
			m_DataType = newType;
		}
	}

private:  // data

	short int m_DataType;
	union {
		int     d_int;
		double  d_double;
		void*   d_obj;
		struct {
			void*  ptr;
			size_t len;
		} d_buf;
	} m_Data;

};  // class CAnyLuaDataStorage