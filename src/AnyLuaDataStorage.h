#include <lua.hpp>

class CAnyLuaDataStorage {

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
		if (m_DataType == LUA_TSTRING)
			delete []m_Data.d_buf.ptr;
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

private:

	void CopyFrom(const CAnyLuaDataStorage& from) {
		FreeBufIfNeed(from.m_DataType);
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
			default:  // unknown types
				m_DataType = LUA_TNONE;
				break;
		}
	}

	void FromLua(lua_State *L, int idx) {
		const int newType = lua_type(L, idx);
		FreeBufIfNeed(newType);
		switch(newType) {
			case LUA_TBOOLEAN:
				m_Data.d_int = lua_toboolean(L, idx);
				m_DataType = LUA_TBOOLEAN;
				break;
			case LUA_TNUMBER:
				m_Data.d_double = lua_tonumber(L, idx);
				m_DataType = LUA_TNUMBER;
				break;
			default:
				const char* str = lua_tostring(L, idx);
				const size_t str_len = strlen(str) + 1;
				AllocateBuf(str_len);
				memcpy(m_Data.d_buf.ptr, str, str_len);
				m_DataType = LUA_TSTRING;
				break;
		}
	}

	void FreeBufIfNeed(const int newType) {
		if (newType != m_DataType) {
			switch (m_DataType) {
				case LUA_TSTRING:
					m_DataType = LUA_TNONE;
					delete []m_Data.d_buf.ptr;
					m_Data.d_buf.ptr = NULL;
					break;
			}
		}
	}

	void AllocateBuf(const size_t new_len) {
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

	short int m_DataType;
	union {
		int     d_int;
		double  d_double;
		struct {
			void*  ptr;
			size_t len;
		} d_buf;
	} m_Data;

};  // class CAnyLuaDataStorage