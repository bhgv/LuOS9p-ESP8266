/* Run the production call_rpc/call_cgi functions against a real Lua state. */
#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "command_types.h"

lua_State *intL;
static int fail_allocation;
int call_rpc(char **buffer, int length);
int call_cgi(char **buffer, int length);

void *styxmalloc(int bytes)
{
    assert(bytes > 0);
    if(fail_allocation)
        return NULL;
    return malloc((size_t)bytes);
}

void styxfree(void *buffer)
{
    free(buffer);
}

static void check(int (*call)(char **, int), const char *code,
                  const char *expected, size_t size, int nul_terminated)
{
    size_t length = strlen(code);
    char *buffer = malloc(length + (nul_terminated ? 1 : 0));
    int count;
    assert(buffer != NULL);
    memcpy(buffer, code, length + (nul_terminated ? 1 : 0));
    lua_pushstring(intL, "preserve caller stack");
    count = call(&buffer, (int)length);
    assert(count == (int)size);
    assert(memcmp(buffer, expected, size) == 0);
    assert(buffer[size] == '\0');
    assert(lua_gettop(intL) == 1);
    assert(strcmp(lua_tostring(intL, 1), "preserve caller stack") == 0);
    lua_pop(intL, 1);
    free(buffer);
}

int main(void)
{
    int (*calls[2])(char **, int) = {call_rpc, call_cgi};
    int i;
    intL = luaL_newstate();
    assert(intL != NULL);
    luaL_openlibs(intL);
    for(i = 0; i != 2; ++i){
        char *buffer;
        int count;
        check(calls[i], "return", "", 0, 1);
        check(calls[i], "return 'long first result', 'x'",
              "long first result\tx\t", 20, 1);
        check(calls[i], "return 42, nil, {}, true, ''", "42\t\t\t\t\t", 7, 1);
        check(calls[i], "return 'a' .. string.char(0) .. 'b'", "a\0b\t", 4, 1);
        check(calls[i], "return 'bounded'", "bounded\t", 8, 0);
        /* Syntax/runtime errors become response strings and restore the stack. */
        buffer = malloc(64);
        assert(buffer != NULL);
        strcpy(buffer, "error('command failure')");
        count = calls[i](&buffer, (int)strlen(buffer));
        assert(count > 0 && strstr(buffer, "command failure") != NULL);
        assert(lua_gettop(intL) == 0);
        free(buffer);
        buffer = malloc(64);
        assert(buffer != NULL);
        strcpy(buffer, "return )");
        count = calls[i](&buffer, (int)strlen(buffer));
        assert(count > 0 && lua_gettop(intL) == 0);
        free(buffer);
        /* Allocation failure leaves a valid empty buffer for strlen callers. */
        buffer = malloc(64);
        assert(buffer != NULL);
        strcpy(buffer, "return 'result'");
        fail_allocation = 1;
        count = calls[i](&buffer, (int)strlen(buffer));
        fail_allocation = 0;
        assert(count == 0 && buffer[0] == '\0');
        assert(lua_gettop(intL) == 0);
        free(buffer);
    }
    {
        Qid qid;
        char bytes[8];
        ulong size;
        vlong offset;
        memset(&qid, 0, sizeof(qid));
        qid.my_type = FS_RPC;
        qid.my_state = 1;
        qid.my_buf = malloc(128);
        assert(qid.my_buf != NULL);
        strcpy(qid.my_buf, "counter = (counter or 0) + 1; return 'cached'");
        qid.my_buf_idx = (int)strlen(qid.my_buf);
        size = 3;
        offset = 0;
        assert(fsrpcread(&qid, bytes, &size, &offset) == NULL);
        assert(size == 3 && memcmp(bytes, "cac", 3) == 0);
        assert(qid.my_state == 2);
        size = 3;
        offset = 3;
        assert(fsrpcread(&qid, bytes, &size, &offset) == NULL);
        assert(size == 3 && memcmp(bytes, "hed", 3) == 0);
        lua_getglobal(intL, "counter");
        assert(lua_tointeger(intL, -1) == 1);
        lua_pop(intL, 1);
        free(qid.my_buf);
    }
    lua_close(intL);
    return 0;
}
