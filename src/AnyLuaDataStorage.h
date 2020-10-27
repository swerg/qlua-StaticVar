#include <lua.hpp>
#include <deque>

class CAnyLuaDataStorage {

typedef std::pair<CAnyLuaDataStorage, CAnyLuaDataStorage>  TABLE_STORAGE_ELEM;
typedef std::deque<TABLE_STORAGE_ELEM>  TABLE_STORAGE;

public:

	CAnyLuaDataStorage() {
	}

	CAnyLuaDataStorage(lua_State *L, int idx) {
		FromLua(L, idx);
	}

	CAnyLuaDataStorage(const CAnyLuaDataStorage& from) {  // copy constructor
		CopyFrom(from);
	}

	~CAnyLuaDataStorage() {
		switch (m_DataType) {
			case TDataType::String:
				delete []m_Data.d_buf.ptr;
				break;
			case TDataType::Table:
				delete (TABLE_STORAGE*)(m_Data.d_obj);
				break;
		}
	}

	void PushToLua(lua_State *L) const {
		switch (m_DataType) {
			case TDataType::Nil:
				lua_pushnil(L);
				break;
			case TDataType::Boolean:
				lua_pushboolean(L, m_Data.d_int);
				break;
			case TDataType::Number:
				lua_pushnumber(L, m_Data.d_double);
				break;
#if LUA_VERSION_NUM >= 503
			case TDataType::Integer:
				lua_pushinteger(L, m_Data.d_int64);
				break;
#endif
			case TDataType::String:
				lua_pushstring(L, (char*)m_Data.d_buf.ptr);
				break;
			case TDataType::Table: {
				const TABLE_STORAGE* tstorage = (TABLE_STORAGE*)m_Data.d_obj;
				lua_createtable(L, 0, int(tstorage->size()));
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

private:  // types

	enum class TDataType {
		None,
		Nil,
		Boolean,
		Number,
#if LUA_VERSION_NUM >= 503
		Integer, //int64
#endif
		String,
		Table,
	};

private:  // methods


	void CopyFrom(const CAnyLuaDataStorage& from) {
		FreeBufObjIfNeed(from.m_DataType);
		switch (from.m_DataType) {
			case TDataType::Nil:
			case TDataType::Boolean:
			case TDataType::Number:
#if LUA_VERSION_NUM >= 503
			case TDataType::Integer:
#endif
				m_Data = from.m_Data;
				m_DataType = from.m_DataType;
				break;
			case TDataType::String:
				AllocateBuf(from.m_Data.d_buf.len);
				memcpy(this->m_Data.d_buf.ptr, from.m_Data.d_buf.ptr, this->m_Data.d_buf.len);
				m_DataType = TDataType::String;
				break;
			case TDataType::Table:
				AllocateObj(TDataType::Table);
				*((TABLE_STORAGE*)m_Data.d_obj) = *((TABLE_STORAGE*)from.m_Data.d_obj);
				break;
			default:  // unknown types
				m_DataType = TDataType::None;
				break;
		}
	}

	void FromLua(lua_State *L, int idx) {
		if (idx < 0) {
			idx = lua_gettop(L) + idx + 1;  // fix idx to true index, not relative
		}
		FreeBufObj();
		switch(lua_type(L, idx)) {
			case LUA_TNIL:
				m_DataType = TDataType::Nil;
				break;
			case LUA_TBOOLEAN:
				m_Data.d_int = lua_toboolean(L, idx);
				m_DataType = TDataType::Boolean;
				break;
			case LUA_TNUMBER:
#if LUA_VERSION_NUM >= 503
				if (lua_isinteger(L, idx)) {
					m_Data.d_int64 = lua_tointeger(L, idx);
					m_DataType = TDataType::Integer;
				}
				else {
					m_Data.d_double = lua_tonumber(L, idx);
					m_DataType = TDataType::Number;
				}
#else
				m_Data.d_double = lua_tonumber(L, idx);
				m_DataType = TDataType::Number;
#endif
				break;
			case LUA_TTABLE: {
				AllocateObj(TDataType::Table);
				TABLE_STORAGE* tstorage = (TABLE_STORAGE*)m_Data.d_obj;
				lua_pushnil(L);     // first key of table
				while (lua_next(L, idx)) {
					tstorage->push_back(TABLE_STORAGE_ELEM(
						CAnyLuaDataStorage(L,-2), CAnyLuaDataStorage(L,-1)  // key,value
					));
					lua_pop(L, 1);  // removes 'value'; keeps 'key' for next iteration
				}}
				break;
			default:  // try to convert to TDataType::String
				const char* str = lua_tostring(L, idx);
				if (str) {
					const size_t str_len = strlen(str) + 1;
					AllocateBuf(str_len);
					memcpy(m_Data.d_buf.ptr, str, str_len);
					m_DataType = TDataType::String;
				} else {
					m_DataType = TDataType::None;
				}
				break;
		}
	}

	void FreeBufObjIfNeed(const TDataType newType) {
		if (m_DataType != newType)
			FreeBufObj();
	}

	void FreeBufObj() {
			switch (m_DataType) {
				case TDataType::String:
					m_DataType = TDataType::None;
					delete []m_Data.d_buf.ptr;
					m_Data.d_buf.ptr = nullptr;
					break;
				case TDataType::Table:
					delete (TABLE_STORAGE*)(m_Data.d_obj);
					break;
			}
	}

	void AllocateBuf(const size_t new_len) {  // allocates a new buffer in d_buf
		if (m_DataType == TDataType::String && m_Data.d_buf.ptr) {
			if (   (new_len > m_Data.d_buf.len)
				|| (m_Data.d_buf.len > 32 && m_Data.d_buf.len/new_len > 2)
				) {
				delete []m_Data.d_buf.ptr;
				m_Data.d_buf.ptr = nullptr;
			}
		}
		else {
				m_Data.d_buf.ptr = nullptr;
		}
		if (!m_Data.d_buf.ptr) {
			m_Data.d_buf.ptr = new char[new_len];
			m_Data.d_buf.len = new_len;
		}
	}

	void AllocateObj(const TDataType newType) {  // creates a new object in d_obj
		if (m_DataType == newType) { // required storage exists, clear only
			switch (m_DataType) {
				case TDataType::Table:
					((TABLE_STORAGE*)m_Data.d_obj)->clear();
					break;
			}
		}
		else {                       // m_DataType != newType --> create a new storage object
			switch (newType) {
				case TDataType::Table:
					m_Data.d_obj = new TABLE_STORAGE;
					break;
			}
			m_DataType = newType;
		}
	}

private:  // data

	TDataType m_DataType{TDataType::None};
	union {
		int     d_int;
#if LUA_VERSION_NUM >= 503
		int64_t d_int64;
#endif
		double  d_double;
		void*   d_obj;
		struct {
			void*  ptr;
			size_t len;
		} d_buf;
	} m_Data {};

};  // class CAnyLuaDataStorage