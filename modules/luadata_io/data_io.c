#include <lua.h>
#include <lauxlib.h>

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "data.h"

inline static bool
check_offset(data_t *data, size_t offset, size_t length)
{
	size_t last_pos = data->offset + data->length - 1;
	return offset >= data->offset && offset <= last_pos;
}

inline static bool
check_length(data_t *data, size_t offset, size_t length)
{
	return offset + length <= data->offset + data->length;
}

inline bool
check_limits(data_t *data, size_t offset, size_t length)
{
	return check_offset(data, offset, length) &&
		check_length(data, offset, length);
}


static int
data_io(lua_State *L, bool rd)
{
	data_t  *data  = lua_touserdata(L, 1);
	size_t	offset = data->offset;
	size_t  length = data->raw->size;
	ssize_t n;

	if (!lua_isnone(L, 2))
		offset += luaL_checknumber(L, 2);
	if (!lua_isnone(L, 3))
		length = luaL_checknumber(L, 3);

	if (!check_limits(data, offset, length)) {
		lua_pushstring(L, "Invalid combination of offset and length");
		lua_error(L);
	}

	if (rd) {
		n = read(0, data->raw->ptr + offset, length);
		if (n < 0) {
			lua_pushstring(L, "Failed to read data");
			lua_error(L);
		}
	} else {
		n = write(1, data->raw->ptr + offset, length);
		if (n < 0) {
			lua_pushstring(L, "Failed to write data");
			lua_error(L);
		}
	}

	lua_pushnumber(L, n);
	return 1;
}

static int
data_read(lua_State *L)
{
	return data_io(L, 1);
}

static int
data_write(lua_State *L)
{
	return data_io(L, 0);
}




#include "modules.h"

static const LUA_REG_TYPE data_io_lib[ ] = {
	{ LSTRKEY( "read" ), LFUNCVAL( data_read ) },
	{ LSTRKEY( "write" ), LFUNCVAL( data_write ) },
		{ LNILKEY, LNILVAL }
};


int
luaopen_data_io(lua_State *L)
{
/*
#if LUA_VERSION_NUM >= 502
	luaL_newlib(L, data_io_lib);
#else
	luaL_register(L, "data_io", data_io_lib);
#endif

	return 1;
*/
	return 0;
}

MODULE_REGISTER_MAPPED(DATA_IO, data_io, data_io_lib, luaopen_data_io);


