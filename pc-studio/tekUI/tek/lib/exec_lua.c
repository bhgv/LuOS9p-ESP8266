/*-----------------------------------------------------------------------------
--
--	tek.lib.exec
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	FUNCTIONS::
--		- Exec.getmsg() - Get next message from own task's message queue
--		- Exec.getname() - Get the own task's name
--		- Exec.getsignals() - Get and clear own task's signals
--		- Exec.run() - Run a Lua function, file, or chunk, returning a task
--		- Exec.sendmsg() - Send a message to a named task
--		- Exec.sendport() - Send a message to a named task and port
--		- Exec.signal() - Send signals to a named task
--		- Exec.sleep() - Suspend own task for a period of time
--		- Exec.wait() - Suspend own task waiting for signals
--		- Exec.waitmsg() - Suspend own rask waiting for a message or timeout
--		- Exec.waittime() - Suspend own task waiting for signals and timeout
--
--	TASKS METHODS::
--		- child:abort() - Send abortion signal and wait for task completion
--		- child:join() - Wait for task completion
--		- child:sendmsg(msg) - Send a message to a task, see Exec.sendmsg()
--		- child:sendport(port, msg) - Send a message to a named port in the
--		task, see Exec.sendport()
--		- child:signal([sigs]) - Send signals to a task, see Exec.signal()
--		- child:terminate() - Send termination signal and wait for completion
--		of a task
--	
-------------------------------------------------------------------------------

module "tek.lib.exec"
_VERSION = "Exec 0.83"
local Exec = _M

-----------------------------------------------------------------------------*/

#include <string.h>
#include <tek/debug.h>
#include <tek/teklib.h>
#include <tek/mod/exec.h>
#include <tek/lib/tek_lua.h>
#include <tek/proto/hal.h>
#include <tek/proto/exec.h>
#include <tek/inline/exec.h>
#include "lualib.h"


#if defined(ENABLE_LAZY_SINGLETON)
#define ENABLE_TASKS
#endif

#define TEK_LIB_EXEC_CLASSNAME "tek.lib.exec*"

#if defined(ENABLE_TASKS)
#define TEK_LIB_EXEC_PROGNAME "luatask"
#define TEK_LIB_TASK_CLASSNAME "tek.lib.exec.task*"
#define TEK_LIB_EXECBASE_REGNAME "TExecBase*"
#define TEK_LIB_BASETASK_ATOMNAME "task.main"
#define TEK_LIB_TASKNAME_LEN 64
#define TEK_LIB_TASK_ATOMNAME "task.%s"
#define TEK_LIB_TASK_ATOMNAME_OFFSET	5
#endif


struct SharedState
{
	struct THandle handle;
};


struct LuaExecTask
{
	struct TExecBase *exec;
	struct TTask *task;
#if defined(ENABLE_TASKS)
	struct LuaExecTask *parent;
	char *taskname;
	char atomname[TEK_LIB_TASKNAME_LEN];
	struct SharedState *shared;
#endif
};


static const struct TInitModule tek_lib_exec_initmodules[] =
{
	{"hal", tek_init_hal, TNULL, 0},
	{"exec", tek_init_exec, TNULL, 0},
	{ TNULL, TNULL, TNULL, 0 }
};


static int tek_lib_exec_base_gc(lua_State *L)
{
	struct LuaExecTask *lexec = luaL_checkudata(L, 1, TEK_LIB_EXEC_CLASSNAME);
	struct TExecBase *TExecBase = lexec->exec;
	if (TExecBase)
	{
#if defined(ENABLE_TASKS)
		TAPTR atom = TLockAtom(TEK_LIB_BASETASK_ATOMNAME, TATOMF_NAME);
		TUnlockAtom(atom, TATOMF_DESTROY);
		if (lexec->parent == TNULL)
		{
			/* free shared state */
			TFree(lexec->shared);
		}
#endif
		TDestroy((struct THandle *) lexec->task);
		lexec->exec = TNULL;
	}
	return 0;
}


#if defined(ENABLE_TASKS)


struct LuaTaskArgs
{
	char *arg;
	size_t len;
};


struct LuaExecChild
{ 
	struct TExecBase *exec;
	struct TTask *task;
	struct LuaExecTask *parent;
	lua_State *L;
	char *taskname;
	char atomname[TEK_LIB_TASKNAME_LEN];
	int chunklen, status, luastatus, ref, numargs, numres;
	char *fname;
	struct LuaTaskArgs *args, *results;
	TBOOL abort;
};


static TUINT tek_lib_exec_string2sig(const char *ss)
{
	TUINT sig = 0;
	int c;
	while ((c = *ss++))
	{
		switch (c)
		{
			case 'A': case 'a':
				sig |= TTASK_SIG_ABORT;
				break;
			case 'M': case 'm':
				sig |= TTASK_SIG_USER;
				break;
			case 'T': case 't':
				sig |= TTASK_SIG_TERM;
				break;
			case 'C': case 'c':
				sig |= TTASK_SIG_CHLD;
				break;
		}
	}
	return sig;
}


static int tek_lib_exec_return_sig2string(lua_State *L, TUINT sig, int nret)
{
	char ss[5], *s = ss;
	if (sig & TTASK_SIG_ABORT)
		*s++ = 'a';
	if (sig & TTASK_SIG_USER)
		*s++ = 'm';
	if (sig & TTASK_SIG_TERM)
		*s++ = 't';
	if (sig & TTASK_SIG_CHLD)
		*s++ = 'c';
	*s = '\0';
	if (s == ss)
		lua_pushnil(L);
	else
		lua_pushstring(L, ss);
	return nret + 1;
}


static struct LuaExecTask *getparent(struct TExecBase *TExecBase)
{
	struct LuaExecTask *self = TGetTaskData(TNULL);
	return self ? self->parent : TNULL;
}


static void tek_lib_exec_checkabort(lua_State *L, struct TExecBase *TExecBase,
	TUINT sig)
{
	if (sig & TTASK_SIG_ABORT)
	{
		struct LuaExecTask *parent = getparent(TExecBase);
		if (parent)
			TSignal(parent->task, TTASK_SIG_ABORT);
		luaL_error(L, "received abort signal");
	}
}


static struct LuaExecTask *tek_lib_exec_check(lua_State *L)
{
	struct LuaExecTask *lexec = lua_touserdata(L, lua_upvalueindex(1));
	if (lexec->exec == TNULL)
		luaL_error(L, "closed handle");
	return lexec;
}


/*-----------------------------------------------------------------------------
--	sig = Exec.getsignals([sigset]): Gets and clears the signals in {{sigset}}
--	from the own task's signal state, and returns the present signals, one
--	character per signal, concatenated to a string. If no signals are present,
--	returns '''nil'''. Possible signals and their meanings are 
--		- {{"a"}} - abort, the Lua program stops due to an error
--		- {{"t"}} - terminate, the task is asked to quit
--		- {{"c"}} - child, a child task has finished
--		- {{"m"}} - message, a message is present at the task
--	The default for {{sigset}} is {{"tcm"}}. Notes: The abortion signal cannot
--	be cleared from a task. The termination signal is a request that a task
--	may handle, but it is not enforced.
-----------------------------------------------------------------------------*/

static int tek_lib_exec_getsignals(lua_State *L)
{
	struct LuaExecTask *lexec = tek_lib_exec_check(L);
	TUINT sig = tek_lib_exec_string2sig(luaL_optstring(L, 1, "tcm"));
	sig = TExecSetSignal(lexec->exec, 0, sig & ~TTASK_SIG_ABORT);
	tek_lib_exec_checkabort(L, lexec->exec, sig);
	return tek_lib_exec_return_sig2string(L, sig, 0);
}


/*-----------------------------------------------------------------------------
--	sig = Exec.sleep([millisecs]): Suspends the task for the given number of
--	1/1000th seconds. If no timeout is specified, sleeps indefinitely. Returns
--	'''nil''' when the timeout has expired, or {{"a"}} when an abortion signal
--	occurred or was already present at the task.
-----------------------------------------------------------------------------*/

static int tek_lib_exec_sleep(lua_State *L)
{
	struct LuaExecTask *lexec = tek_lib_exec_check(L);
	TTIME dt, *pdt;
	TUINT sig;
	dt.tdt_Int64 = luaL_optnumber(L, 1, 0) * 1000;
	pdt = dt.tdt_Int64 ? &dt : TNULL;
	sig = TExecWaitTime(lexec->exec, pdt, TTASK_SIG_ABORT);
	tek_lib_exec_checkabort(L, lexec->exec, sig);
	return tek_lib_exec_return_sig2string(L, sig, 0);
}


/*-----------------------------------------------------------------------------
--	sig = Exec.waittime(millisecs[, sigset]): Suspends the task waiting for
--	the given number of 1/1000th seconds, or for any signals from the specified
--	set. Signals from {{sigset}} that occured (or were already present at
--	the task) are returned to the caller - see Exec.getsignals() for details.
--	The default signal set to wait for is {{"tc"}}. Note: The abortion signal
--	{{"a"}} is always included in the signal set.
-----------------------------------------------------------------------------*/

static int tek_lib_exec_waittime(lua_State *L)
{
	struct LuaExecTask *lexec = tek_lib_exec_check(L);
	TUINT sig = tek_lib_exec_string2sig(luaL_optstring(L, 2, "tc"));
	TTIME dt;
	dt.tdt_Int64 = luaL_checknumber(L, 1) * 1000;
	sig = TExecWaitTime(lexec->exec, &dt, sig | TTASK_SIG_ABORT);
	tek_lib_exec_checkabort(L, lexec->exec, sig);
	return tek_lib_exec_return_sig2string(L, sig, 0);
}


/*-----------------------------------------------------------------------------
--	sig = Exec.wait([sigset]): Suspends the task waiting for any signals from
--	the specified set. The signals from {{sigset}} that occured (or were
--	already present at the task) are returned to the caller - see
--	Exec.getsignals() for their format. The default signal set to wait for is
--	{{"tc"}}. Note: The abortion signal {{"a"}} is always included in the set.
-----------------------------------------------------------------------------*/

static int tek_lib_exec_wait(lua_State *L)
{
	struct LuaExecTask *lexec = tek_lib_exec_check(L);
	TUINT sig = tek_lib_exec_string2sig(luaL_optstring(L, 1, "tc"));
	sig = TExecWait(lexec->exec, sig | TTASK_SIG_ABORT);
	tek_lib_exec_checkabort(L, lexec->exec, sig);
	return tek_lib_exec_return_sig2string(L, sig, 0);
}


/*-----------------------------------------------------------------------------
--	name = Exec.getname(): Returns the own task's name.
-----------------------------------------------------------------------------*/

static int tek_lib_exec_getname(lua_State *L)
{
	struct LuaExecTask *lexec = tek_lib_exec_check(L);
	if (lexec->taskname)
		lua_pushstring(L, lexec->taskname);
	else
		lua_pushnil(L);
	return 1;
}


/*-----------------------------------------------------------------------------
--	msg, sender = Exec.getmsg(): Unlinks and returns the next message from the
--	task's message queue, or '''nil''' if no messages are present. If a message
--	is returned, then the second argument is the name of the task sending the
--	message.
-----------------------------------------------------------------------------*/

static int tek_lib_exec_getmsg(lua_State *L)
{
	struct LuaExecTask *lexec = tek_lib_exec_check(L);
	struct TExecBase *TExecBase = lexec->exec;
	char *msg = TGetMsg(TGetUserPort(TNULL));
	if (msg)
	{
		TSIZE size = TGetSize(msg) - TEK_LIB_TASKNAME_LEN;
		lua_pushlstring(L, msg, size);
		lua_pushstring(L, msg + size);
		TAckMsg(msg);
		return 2;
	}
	lua_pushnil(L);
	lua_pushnil(L);
	return 2;
}


/*-----------------------------------------------------------------------------
--	msg, sender, sig = Exec.waitmsg([millisecs]): Suspends the task waiting
--	for the given number of 1/1000th seconds or for a message, and returns the
--	next message to the caller. If no timeout is specified, waits indefinitely.
--	Returns '''nil''' when no message arrives in the given time, or when the
--	cause for returning was that only a signal showed up in the task. The
--	second return value is the name of the sender task, see Exec.getmsg() for
--	details. The third return value contains the possible signals ({{"m"}}
--	and {{"a"}}) that caused this function to return, or '''nil''' if a
--	timeout occurred.
-----------------------------------------------------------------------------*/

static int tek_lib_exec_waitmsg(lua_State *L)
{
	struct LuaExecTask *lexec = tek_lib_exec_check(L);
	struct TExecBase *TExecBase = lexec->exec;
	TTIME dt, *pdt = TNULL, destt, nowt;
	int narg;
	TAPTR hal = TExecBase->texb_HALBase;
	struct TMsgPort *port = TGetUserPort(TNULL);
    struct TNode *node;
	TUINT sig = TSetSignal(0, 0) & (TTASK_SIG_USER | TTASK_SIG_ABORT);

	tek_lib_exec_checkabort(L, lexec->exec, sig);
	
	dt.tdt_Int64 = luaL_optnumber(L, 1, 0) * 1000;
	if (dt.tdt_Int64)
	{
		TGetSystemTime(&destt);
		TAddTime(&destt, &dt);
		pdt = &dt;
	}
	
	for (;;)
	{
		THALLock(hal, &port->tmp_Lock);
		node = port->tmp_MsgList.tlh_Head.tln_Succ;
		if (node->tln_Succ == TNULL)
			node = TNULL;
		THALUnlock(hal, &port->tmp_Lock);
		if (node)
			break;
		sig = TWaitTime(pdt, TTASK_SIG_USER | TTASK_SIG_ABORT);
		tek_lib_exec_checkabort(L, lexec->exec, sig);
		if (sig == 0)
			break;
		if (pdt)
		{
			dt = destt;
			TGetSystemTime(&nowt);
			TSubTime(&dt, &nowt);
		}
	}
	
	narg = tek_lib_exec_getmsg(L);
	return tek_lib_exec_return_sig2string(L, sig, narg);
}


static void l_message (const char *pname, const char *msg) {
  if (pname) fprintf(stderr, "%s: ", pname);
  fprintf(stderr, "%s\n", msg);
  fflush(stderr);
}


static int report (lua_State *L, int status) {
  if (status && !lua_isnil(L, -1)) {
    const char *msg = lua_tostring(L, -1);
    if (msg == NULL) msg = "(error object is not a string)";
#if !defined(TEK_LIB_EXEC_SILENT)
    l_message(TEK_LIB_EXEC_PROGNAME, msg);
#endif
    lua_pop(L, 1);
  }
  return status;
}


#if LUA_VERSION_NUM < 502

static int traceback (lua_State *L) {
  if (!lua_isstring(L, 1))  /* 'message' not a string? */
    return 1;  /* keep it intact */
  lua_getfield(L, LUA_GLOBALSINDEX, "debug");
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return 1;
  }
  lua_getfield(L, -1, "traceback");
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    return 1;
  }
  lua_pushvalue(L, 1);  /* pass error message */
  lua_pushinteger(L, 2);  /* skip this function and traceback */
  lua_call(L, 2, 1);  /* call debug.traceback */
  return 1;
}

#else

static int msghandler (lua_State *L) {
  const char *msg = lua_tostring(L, 1);
  if (msg == NULL) {  /* is error object not a string? */
    if (luaL_callmeta(L, 1, "__tostring") &&  /* does it have a metamethod */
        lua_type(L, -1) == LUA_TSTRING)  /* that produces a string? */
      return 1;  /* that is the message */
    else
      msg = lua_pushfstring(L, "(error object is a %s value)",
                               luaL_typename(L, 1));
  }
  luaL_traceback(L, L, msg, 1);  /* append a standard traceback */
  return 1;  /* return the traceback */
}

#endif


static void tek_lib_exec_allocarg(lua_State *L, struct TExecBase *TExecBase,
	int stackidx, struct LuaTaskArgs *arg)
{
	const char *s = lua_tolstring(L, stackidx, &arg->len);
	if (arg->len > 0)
	{
		arg->arg = TAlloc(TNULL, arg->len);
		if (arg->arg == TNULL)
			luaL_error(L, "out of memory");
		memcpy(arg->arg, s, arg->len);
	}
}


static struct LuaTaskArgs *tek_lib_exec_getargs(lua_State *L,
	struct TExecBase *TExecBase, int first, int narg, int extra)
{
	struct LuaTaskArgs *args = TNULL;
	if (narg + extra > 0)
	{
		int i;
		args = TAlloc0(TNULL, sizeof(struct LuaTaskArgs) * (narg + extra));
		if (args == TNULL)
			luaL_error(L, "out of memory");
		for (i = 0; i < extra; ++i)
			tek_lib_exec_allocarg(L, TExecBase, i - extra, &args[i]);
		for (i = extra; i < narg + extra; ++i)
			tek_lib_exec_allocarg(L, TExecBase, first + i - extra, &args[i]);
	}
	return args;
}


static void tek_lib_exec_freeargs(struct TExecBase *TExecBase,
	struct LuaTaskArgs *args, int numargs)
{
	if (numargs > 0 && args)
	{
		int i;
		for (i = 0; i < numargs; ++i)
			TFree(args[i].arg);
		TFree(args);
	}
}


static void tek_lib_exec_freectxargs(struct LuaExecChild *ctx)
{
	tek_lib_exec_freeargs(ctx->exec, ctx->args, ctx->numargs);
	ctx->args = TNULL;
	ctx->numargs = 0;
}


static void tek_lib_exec_run_hook(lua_State *L, lua_Debug *d)
{
	lua_getfield(L, LUA_REGISTRYINDEX, TEK_LIB_EXECBASE_REGNAME);
	if (lua_islightuserdata(L, -1))
	{
		struct TExecBase *TExecBase = lua_touserdata(L, -1);
		tek_lib_exec_checkabort(L, TExecBase, TSetSignal(0, 0));
		lua_pop(L, 1);
	}
}


static void tek_lib_exec_register_task_hook(struct lua_State *L,
	struct TExecBase *exec)
{
	lua_pushlightuserdata(L, exec);
	lua_setfield(L, LUA_REGISTRYINDEX, TEK_LIB_EXECBASE_REGNAME);
	lua_sethook(L, tek_lib_exec_run_hook, LUA_MASKCOUNT, 128);
}


static int tek_lib_exec_runchild(lua_State *L)
{
	int i;
	struct LuaExecChild *ctx = lua_touserdata(L, 1);
	struct TExecBase *TExecBase = ctx->exec;
	
	lua_gc(L, LUA_GCSTOP, 0);
	luaL_openlibs(L);
	lua_gc(L, LUA_GCRESTART, 0);
	
	lua_createtable(L, ctx->numargs + 1, 0);
	lua_pushvalue(L, -1);
	lua_setglobal(L, "arg");
	
	for (i = 0; i < ctx->numargs; ++i)
	{
		if (ctx->args[i].arg == TNULL)
			continue;
		lua_pushlstring(L, ctx->args[i].arg, ctx->args[i].len);
		lua_rawseti(L, -2, i);
	}
	tek_lib_exec_freectxargs(ctx);
	
	if (ctx->fname)
	{
		lua_pushstring(L, ctx->fname);
		lua_rawseti(L, -2, 0);
		ctx->status = luaL_loadfile(ctx->L, ctx->fname);
	}
	else if (ctx->chunklen)
		ctx->status = luaL_loadbuffer(L, (const char *) (ctx + 1), 
			ctx->chunklen, "...");
	if (ctx->status == 0)
	{
		int narg = 0;
		int base = lua_gettop(L) - narg;
#if LUA_VERSION_NUM < 502
		lua_pushcfunction(L, traceback);
#else
		lua_pushcfunction(L, msghandler);
#endif
		lua_insert(L, base);
		tek_lib_exec_register_task_hook(ctx->L, TExecBase);
		ctx->status = ctx->luastatus = lua_pcall(L, narg, LUA_MULTRET, base);
		lua_remove(L, base);
		if (ctx->status)
		{
			TDBPRINTF(TDB_TRACE,("pcall ctx->status=%d, signal to parent\n",
				ctx->status));
			struct LuaExecTask *parent = ctx->parent;
			TSignal(parent->task, TTASK_SIG_ABORT);
		}
		else
		{
			int nres = ctx->numres = lua_gettop(L) - 2;
			ctx->results = tek_lib_exec_getargs(L, TExecBase, 3, nres, 0);
		}
	}
	TDBPRINTF(TDB_TRACE,("collecting1...\n"));
	lua_gc(L, LUA_GCCOLLECT, 0);
	return report(L, ctx->status);
}


static char *tek_lib_exec_taskname(char *buf, const char *name)
{
	if (name == TNULL)
		return TNULL;
	snprintf(buf, TEK_LIB_TASKNAME_LEN - 1, TEK_LIB_TASK_ATOMNAME, name);
	buf[TEK_LIB_TASKNAME_LEN - 1] = '\0';
	return buf + TEK_LIB_TASK_ATOMNAME_OFFSET;
}


static THOOKENTRY TTAG
tek_lib_exec_run_dispatch(struct THook *hook, TAPTR task, TTAG msg)
{
	struct LuaExecChild *ctx = hook->thk_Data;
	struct TExecBase *TExecBase = ctx->exec;
	switch (msg)
	{
		case TMSG_INITTASK:
		{
			TAPTR atom;
			if (!ctx->taskname)
			{
				sprintf(ctx->atomname, "task.task: %p", task);
				ctx->taskname = ctx->atomname + TEK_LIB_TASK_ATOMNAME_OFFSET;
			}
			atom = TLockAtom(ctx->atomname, 
				TATOMF_CREATE | TATOMF_NAME | TATOMF_TRY);
			if (!atom)
				return TFALSE;
			TSetAtomData(atom, (TTAG) task);
			TUnlockAtom(atom, TATOMF_KEEP);
			return TTRUE;
		}

		case TMSG_RUNTASK:
		{
			struct LuaExecTask *parent = ctx->parent;
			TUINT sig = TTASK_SIG_CHLD;
			ctx->task = TFindTask(TNULL);
			lua_pushcfunction(ctx->L, &tek_lib_exec_runchild);
			lua_pushlightuserdata(ctx->L, ctx);
			ctx->status = lua_pcall(ctx->L, 1, 1, 0);
			TDBPRINTF(TDB_TRACE,("pcall2 ctx->status=%d\n", ctx->status));
			report(ctx->L, ctx->status);
			lua_close(ctx->L);
			if (ctx->status)
				sig |= TTASK_SIG_ABORT;
			TSignal(parent->task, sig);
			if (ctx->taskname)
			{
				TAPTR atom = TLockAtom(ctx->atomname, TATOMF_NAME);
				TUnlockAtom(atom, TATOMF_DESTROY);
			}
			break;
		}
	}
	return 0;
}


static int tek_lib_exec_write(lua_State *L, const void *p, size_t sz, void *ud)
{
	luaL_Buffer *b = ud;
	luaL_addlstring(b, p, sz);
	return 0;
}


static const char *tek_lib_exec_dump(lua_State *L, size_t *len)
{
	luaL_Buffer b;
	luaL_buffinit(L, &b);
#if LUA_VERSION_NUM < 503
	lua_dump(L, tek_lib_exec_write, &b);
#else
	lua_dump(L, tek_lib_exec_write, &b, 0);
#endif
	luaL_pushresult(&b);
	lua_remove(L, -2);
	return luaL_checklstring(L, -1, len);
}


static void *tek_lib_exec_allocf(void *ud, void *ptr, size_t osize,
	size_t nsize)
{
	if (nsize != 0)
		return realloc(ptr, nsize);
	free(ptr);
	return NULL;
}


/*-----------------------------------------------------------------------------
--	child = Exec.run(what[, arg1[, ...]]): Tries to launch a Lua script,
--	function, or chunk, and returns a handle on a child task if successful.
--	The first argument can be a string denoting the filename of a Lua script,
--	a Lua function, or a table. If a table is given, either of the following
--	keys in the table is mandatory:
--		* {{"func"}}, a function value to run,
--		* {{"filename"}}, a string denoting the filename of a Lua script,
--		* {{"chunk"}}, a string value containing a Lua chunk to run.
--	Optional keys in the table:
--		* {{"taskname"}}, a string denoting the name of the task to run.
--		Task names are immutable during runtime and must be unique; the
--		top-level task's implicit name is {{"main"}}.
--		* {{"abort"}}, a boolean to indicate whether errors in the child
--		should be propagated to the parent task. Default: '''true'''
--	Additional arguments are passed to the script, in their given order.
--	Methods on the returned child task handle:
--		* child:abort() - sends abortion signal and synchronizes on completion
--		of the task
--		* child:join() - synchronizes on completion of the task
--		* child:sendmsg(msg) - sends a message to a task, see Exec.sendmsg()
--		* child:sendport(port, msg) - Sends a message to a named port in
--		the task, see Exec.sendport()
--		* child:signal([sigs]) - sends signals to a task, see Exec.signal()
--		* child:terminate() - sends termination signal and synchronizes on
--		completion of the task
--	Note that the returned child handle is subject to garbage collection. If
--	it gets collected (at the latest when the parent task is ending) and the
--	child task is still running, the child will be sent the abortion signal,
--	causing it to raise an error. This error, like any child error, also gets
--	propagated back to the parent task, unless error propagation is suppressed
--	(see above).
-----------------------------------------------------------------------------*/

static int tek_lib_exec_run(lua_State *L)
{
	struct LuaExecTask *lexec = tek_lib_exec_check(L);
	struct TExecBase *TExecBase = lexec->exec;
	struct LuaExecChild *ctx;
	const char *fname = TNULL;
	const char *chunk = TNULL;
	const char *taskname = TNULL;
	size_t extralen = 0;
	struct THook hook;
	TTAGITEM tags[2];
	int nremove = 1;
	TBOOL abort = TTRUE;
	
	for (;;)
	{
		if (lua_istable(L, 1))
		{
			lua_getfield(L, 1, "abort");
			if (lua_isboolean(L, -1))
				abort = lua_toboolean(L, -1);
			lua_pop(L, 1);
			lua_getfield(L, 1, "taskname");
			taskname = lua_tostring(L, -1);
			nremove = 2;
			lua_getfield(L, 1, "func");
			if (!lua_isnoneornil(L, -1))
				break;
			lua_pop(L, 1);
			lua_getfield(L, 1, "filename");
			if (!lua_isnoneornil(L, -1))
				break;
			lua_pop(L, 1);
			lua_getfield(L, 1, "chunk");
			if (!lua_isnoneornil(L, -1))
			{
				chunk = luaL_checklstring(L, -1, &extralen);
				break;
			}
			luaL_error(L, "required argument missing");
		}
		lua_pushvalue(L, 1);
		break;
	}
	
	if (!chunk)
	{
		if (lua_type(L, -1) == LUA_TSTRING)
			fname = luaL_checklstring(L, -1, &extralen);
		else if (lua_isfunction(L, -1) && !lua_iscfunction(L, -1))
			chunk = tek_lib_exec_dump(L, &extralen);
		else
			luaL_error(L, "not a Lua function, filename or table");
	}
	
	ctx = lua_newuserdata(L, sizeof(struct LuaExecChild) + extralen + 1);
	memset(ctx, 0, sizeof *ctx);
	ctx->exec = lexec->exec;
	ctx->parent = lexec;
	ctx->taskname = tek_lib_exec_taskname(ctx->atomname, taskname);
	ctx->abort = abort;
	
	if (fname)
	{
		ctx->fname = (char *) (ctx + 1);
		strcpy(ctx->fname, (char *) fname);
	}
	else if (chunk)
	{
		memcpy(ctx + 1, chunk, extralen);
		ctx->chunklen = extralen;
	}
	
	/* remove arguments under userdata */
	while (nremove--)
		lua_remove(L, -2);

	ctx->numargs = lua_gettop(L) - 2;
	
	/* push arg[0] on the stack, will be extraarg */
	lua_getglobal(L, "arg");
	if (lua_istable(L, -1))
	{
		lua_rawgeti(L, -1, 0);
		lua_remove(L, -2);
	}
	if (lua_type(L, -1) != LUA_TSTRING)
	{
		lua_pop(L, 1);
		lua_pushnil(L);
	}
	ctx->args = tek_lib_exec_getargs(L, TExecBase, 2, ctx->numargs++, 1);
	lua_pop(L, 1);
	
	ctx->L = lua_newstate(tek_lib_exec_allocf, TExecBase);
	if (ctx->L == TNULL)
	{
		tek_lib_exec_freeargs(TExecBase, ctx->args, ctx->numargs);
		luaL_error(L, "cannot create interpreter");
	}
	
	tags[0].tti_Tag = TTask_UserData;
	tags[0].tti_Value = (TTAG) ctx;
	tags[1].tti_Tag = TTAG_DONE;
	TInitHook(&hook, tek_lib_exec_run_dispatch, ctx);
	ctx->task = TCreateTask(&hook, tags);
	if (ctx->task == TNULL)
	{
		tek_lib_exec_freectxargs(ctx);
		lua_pop(L, 1);
		lua_pushnil(L);
		return 1;
	}
	
	lua_getfield(L, LUA_REGISTRYINDEX, TEK_LIB_TASK_CLASSNAME);
	lua_setmetatable(L, -2);
	lua_pushvalue(L, -1);
	ctx->ref = luaL_ref(L, lua_upvalueindex(2));
	if (ctx->abort)
		tek_lib_exec_register_task_hook(L, TExecBase);
	return 1;
}


static TAPTR tek_lib_exec_locktask(struct TExecBase *TExecBase,
	const char *name, TAPTR *ref)
{
	if (name && strcmp(name, "*p") != 0)
	{
		char atomname[TEK_LIB_TASKNAME_LEN];
		tek_lib_exec_taskname(atomname, name);
		*ref = TLockAtom(atomname, TATOMF_NAME | TATOMF_SHARED);
		if (*ref)
			return (TAPTR) TGetAtomData(*ref);
	}
	else
	{
		struct LuaExecTask *parent = getparent(TExecBase);
		*ref = TNULL;
		if (parent)
			return parent->task;
	}
	return TNULL;
}


static void tek_lib_exec_unlocktask(struct TExecBase *TExecBase, TAPTR ref)
{
	if (ref)
		TUnlockAtom(ref, TATOMF_KEEP);
}


/*-----------------------------------------------------------------------------
--	success = Exec.sendmsg(taskname, msg): Sends a message to a named task.
--	The special name {{"*p"}} addresses the parent task. Returns '''true'''
--	if the task was found and the message sent.
-----------------------------------------------------------------------------*/

static int tek_lib_exec_sendmsg(lua_State *L)
{
	struct LuaExecTask *lexec = tek_lib_exec_check(L);
	struct TExecBase *TExecBase = lexec->exec;
	const char *taskname = luaL_checkstring(L, 1);
	size_t msglen;
	const char *src = luaL_checklstring(L, 2, &msglen);
	TAPTR ref, task;
	char *msg = TAllocMsg(msglen + TEK_LIB_TASKNAME_LEN);
	if (msg == TNULL)
		luaL_error(L, "out of memory");
	task = tek_lib_exec_locktask(TExecBase, taskname, &ref);
	if (task)
	{
		memcpy(msg, src, msglen);
        strcpy(msg + msglen, lexec->taskname);
		TPutMsg(TGetUserPort(task), TNULL, msg);
		tek_lib_exec_unlocktask(TExecBase, ref);
		lua_pushboolean(L, TTRUE);
	}
	else
	{
		TFree(msg);
		lua_pushnil(L);
	}
	return 1;
}


/*-----------------------------------------------------------------------------
--	success = Exec.signal(taskname[, sigs]): Sends signals to a named task,
--	by default the termination signal {{"t"}}. The special name {{"*p"}}
--	addresses the parent task. Returns '''true''' if the task was found and
--	the signals sent. See Exec.getsignals() for details on the signal format.
-----------------------------------------------------------------------------*/

static int tek_lib_exec_signal(lua_State *L)
{
	struct LuaExecTask *lexec = tek_lib_exec_check(L);
	struct TExecBase *TExecBase = lexec->exec;
	const char *taskname = luaL_checkstring(L, 1);
	TUINT sig = tek_lib_exec_string2sig(luaL_optstring(L, 2, "t"));
	TAPTR ref;
	TAPTR task = tek_lib_exec_locktask(TExecBase, taskname, &ref);
	if (task)
	{
		TSignal(task, sig);		
		tek_lib_exec_unlocktask(TExecBase, ref);
		lua_pushboolean(L, TTRUE);
	}
	else
		lua_pushnil(L);
	return 1;
}


static int tek_lib_exec_child_sigjoin(lua_State *L, TUINT sig, TBOOL sigparent,
	TBOOL do_results)
{
	struct LuaExecChild *ctx = luaL_checkudata(L, 1, TEK_LIB_TASK_CLASSNAME);
	struct TExecBase *TExecBase = ctx->exec;
	struct TTask *self = TFindTask(TNULL), *task = ctx->task;
	union TTaskRequest *req = &self->tsk_Request;
	TBOOL abort = TFALSE;
	int nres = 0;
	if (!task)
		return 0;
	
	TDBPRINTF(TDB_TRACE,("child_join sig=%d sigparent=%d\n", sig, sigparent));
	
	/* signal task */
	TSignal(task, sig);
	/* prepare destroy request */
	self->tsk_ReqCode = TTREQ_DESTROYTASK;
	req->trq_Task.trt_Task = task;
	/* send to exec */
	TPutMsg(TExecBase->texb_ExecPort, &self->tsk_SyncPort, self);
	for (;;)
	{
		/* wait for return of request or receiving abort ourselves: */
		TUINT sig = TWait(TTASK_SIG_SINGLE | TTASK_SIG_ABORT);
		if (sig & TTASK_SIG_SINGLE)
			break;
		if (sig & TTASK_SIG_ABORT)
		{
			/* forward to child task */
			TSignal(task, TTASK_SIG_ABORT);
			if (sigparent)
			{
				/* also forward to own parent task */
				struct LuaExecTask *parent = getparent(TExecBase);
				if (parent)
					TSignal(parent->task, TTASK_SIG_ABORT);
				abort = TTRUE;
			}
		}
	}
	/* take delivery of replied destroy request */
	TGetMsg(&self->tsk_SyncPort);
	/* free task */	
	TFreeTask(task);
	
	ctx->task = TNULL;
	tek_lib_exec_freectxargs(ctx);
	if (!abort && do_results)
	{
		int i;
		nres = ctx->numres + 1;
		lua_pushboolean(L, ctx->luastatus == 0);
		for (i = 0; i < ctx->numres; ++i)
			lua_pushlstring(L, ctx->results[i].arg, ctx->results[i].len);
	}
	tek_lib_exec_freeargs(TExecBase, ctx->results, ctx->numres);
	luaL_unref(L, lua_upvalueindex(1), ctx->ref);
	if (abort)
		luaL_error(L, "received abort signal");
	return nres;
}


/*-----------------------------------------------------------------------------
--	child:signal([sig]) - Sends the task the given signals, by default the
--	termination signal {{"t"}}.
-----------------------------------------------------------------------------*/

static int tek_lib_exec_child_signal(lua_State *L)
{
	struct LuaExecChild *ctx = luaL_checkudata(L, 1, TEK_LIB_TASK_CLASSNAME);
	TUINT sig = tek_lib_exec_string2sig(luaL_optstring(L, 2, "t"));
	if (ctx->task)
		TExecSignal(ctx->exec, ctx->task, sig);
	return 0;
}


/*-----------------------------------------------------------------------------
--	success, res1, ... = child:terminate(): This function is equivalent to
--	child:join(), except for that it sends the child task the termination
--	signal {{"t"}} before joining.
-----------------------------------------------------------------------------*/

static int tek_lib_exec_child_terminate(lua_State *L)
{
	struct LuaExecChild *ctx = luaL_checkudata(L, 1, TEK_LIB_TASK_CLASSNAME);
	return tek_lib_exec_child_sigjoin(L, TTASK_SIG_TERM, ctx->abort, TTRUE);
}


/*-----------------------------------------------------------------------------
--	child:abort(): Sends the task the abortion signal {{"a"}} and synchronizes
--	on its completion.
-----------------------------------------------------------------------------*/

static int tek_lib_exec_child_abort(lua_State *L)
{
	return tek_lib_exec_child_sigjoin(L, TTASK_SIG_ABORT, TFALSE, TFALSE);
}


/*-----------------------------------------------------------------------------
--	success, res1, ... = child:join(): Synchronizes the caller on completion
--	of the child task. The first return value is boolean and indicative of
--	whether the task completed successfully. While suspended in this function
--	and if an error occurs in the child task, an error is propagated to the
--	caller also, unless error propagation is suppressed - see Exec.run(). If
--	successful, return values from the child task will be converted to
--	strings and returned as additional return values.
-----------------------------------------------------------------------------*/

static int tek_lib_exec_child_join(lua_State *L)
{
	struct LuaExecChild *ctx = luaL_checkudata(L, 1, TEK_LIB_TASK_CLASSNAME);
	return tek_lib_exec_child_sigjoin(L, 0, ctx->abort, TTRUE);
}


static int tek_lib_exec_child_gc(lua_State *L)
{
	struct LuaExecChild *ctx = luaL_checkudata(L, 1, TEK_LIB_TASK_CLASSNAME);
	return tek_lib_exec_child_sigjoin(L, TTASK_SIG_ABORT, ctx->abort, TFALSE);
}


static TBOOL tek_lib_exec_sendtaskport(struct TTask *task,
	const char *portname, const char *buf, size_t len)
{
	struct TExecBase *TExecBase = TGetExecBase(task);
	char atomname[256];
	TAPTR atom;
	TBOOL success = TFALSE;
	sprintf(atomname, "msgport.%s.%p", portname, task);
	atom = TLockAtom(atomname, TATOMF_SHARED | TATOMF_NAME);
	if (atom)
	{
		TAPTR imsgport = (TAPTR) TGetAtomData(atom);
		if (imsgport)
		{
			TAPTR msg = TAllocMsg0(len);
			if (msg)
			{
				memcpy(msg, buf, len);
				TPutMsg(imsgport, TNULL, msg);
				success = TTRUE;
			}
		}
		TUnlockAtom(atom, TATOMF_KEEP);
	}
	return success;
}

/*-----------------------------------------------------------------------------
--	success = Exec.sendport(taskname, portname, msg): Sends the message to
--	the named message port in the named task. Returns '''true''' if the task
--	and port could be found and the message was sent.
-----------------------------------------------------------------------------*/

static int tek_lib_exec_sendport(lua_State *L)
{
	TBOOL success = TFALSE;
	struct LuaExecTask *lexec = tek_lib_exec_check(L);
	struct TExecBase *TExecBase = lexec->exec;
	const char *taskname = luaL_checkstring(L, 1);
	const char *portname = luaL_checkstring(L, 2);
	size_t len;
	const char *buf = luaL_checklstring(L, 3, &len);
	TAPTR ref;
	struct TTask *task = tek_lib_exec_locktask(TExecBase, taskname, &ref);
	if (task)
	{
		success = tek_lib_exec_sendtaskport(task, portname, buf, len);
		tek_lib_exec_unlocktask(TExecBase, ref);
	}
	lua_pushboolean(L, success);
	return 1;
}


static int tek_lib_exec_child_sendport(lua_State *L)
{
	TBOOL success = TFALSE;
	struct LuaExecChild *ctx = luaL_checkudata(L, 1, TEK_LIB_TASK_CLASSNAME);
	if (ctx->task)
	{
		size_t len;
		const char *portname = luaL_checkstring(L, 2);
		const char *buf = luaL_checklstring(L, 3, &len);
		success = tek_lib_exec_sendtaskport(ctx->task, portname, buf, len);
	}
	lua_pushboolean(L, success);
	return 1;
}


/*-----------------------------------------------------------------------------
--	child:sendmsg(msg) - Sends the task the given message.
-----------------------------------------------------------------------------*/

static int tek_lib_exec_child_sendmsg(lua_State *L)
{
	struct LuaExecChild *ctx = luaL_checkudata(L, 1, TEK_LIB_TASK_CLASSNAME);
	if (ctx->task)
	{
		struct TExecBase *TExecBase = ctx->exec;
		size_t len;
		const char *buf = luaL_checklstring(L, 2, &len);
		char *msg = TAllocMsg(len + TEK_LIB_TASKNAME_LEN);
		if (msg == TNULL)
			luaL_error(L, "out of memory");
		memcpy(msg, buf, len);
		strcpy(msg + len, ctx->parent->taskname);
		TPutMsg(TGetUserPort(ctx->task), TNULL, msg);
	}
	else
		luaL_error(L, "closed handle");
	return 0;
}


static TBOOL tek_lib_exec_init_link_to_parent(lua_State *L, 
	struct LuaExecTask *lexec)
{
	struct TExecBase *TExecBase = lexec->exec;
	TAPTR atom;
	
	/* task's userdata initially contains a pointer to
	** childtask-userdata in parent Lua state */
	struct LuaExecChild *ctx = TGetTaskData(TNULL);
	
	/* now relink to own task */
	TSetTaskData(TNULL, lexec);
	
	lexec->parent = ctx ? ctx->parent : TNULL;
	if (ctx && ctx->taskname)
	{
		/* child context */
		strcpy(lexec->atomname, ctx->atomname);
		lexec->taskname = lexec->atomname + TEK_LIB_TASK_ATOMNAME_OFFSET;
		lexec->shared = ctx->parent->shared;
	}
	else
	{
		/* root context */
		lexec->taskname = tek_lib_exec_taskname(lexec->atomname, "main");
		lexec->shared = TAlloc(TNULL, sizeof(struct SharedState));
		if (lexec->shared == TNULL)
			return TFALSE;
	}
	atom = TLockAtom(TEK_LIB_BASETASK_ATOMNAME, 
		TATOMF_CREATE | TATOMF_NAME | TATOMF_TRY);
	if (atom)
	{
		TSetAtomData(atom, (TTAG) lexec->task);
		TUnlockAtom(atom, TATOMF_KEEP);
	}

	return TTRUE;
}


static const luaL_Reg tek_lib_exec_child_methods[] =
{
	{ "__gc", tek_lib_exec_child_gc },
	{ "abort", tek_lib_exec_child_abort },
	{ "join", tek_lib_exec_child_join },
	{ "sendmsg", tek_lib_exec_child_sendmsg },
	{ "sendport", tek_lib_exec_child_sendport },
	{ "signal", tek_lib_exec_child_signal },
	{ "terminate", tek_lib_exec_child_terminate },
	{ TNULL, TNULL }
};


#endif


static const luaL_Reg tek_lib_exec_funcs[] =
{
#if defined(ENABLE_TASKS)
	{ "getmsg", tek_lib_exec_getmsg },
	{ "getname", tek_lib_exec_getname },
	{ "getsignals", tek_lib_exec_getsignals },
	{ "run", tek_lib_exec_run },
	{ "sendmsg", tek_lib_exec_sendmsg },
	{ "sendport", tek_lib_exec_sendport },
	{ "signal", tek_lib_exec_signal },
	{ "sleep", tek_lib_exec_sleep },
	{ "wait", tek_lib_exec_wait },
	{ "waitmsg", tek_lib_exec_waitmsg },
	{ "waittime", tek_lib_exec_waittime },
#endif
	{ TNULL, TNULL }
};


static const luaL_Reg tek_lib_exec_methods[] =
{
	{ "__gc", tek_lib_exec_base_gc },
	{ TNULL, TNULL }
};


TMODENTRY int luaopen_tek_lib_exec(lua_State *L)
{
	struct TExecBase *TExecBase;
	struct LuaExecTask *lexec;
	TTAGITEM tags[2];
	
	luaL_newmetatable(L, TEK_LIB_EXEC_CLASSNAME);
	tek_lua_register(L, NULL, tek_lib_exec_methods, 0);
	/* execmeta */

#if defined(ENABLE_TASKS)
	luaL_newmetatable(L, TEK_LIB_TASK_CLASSNAME);
	/* execmeta, taskmeta */
	lua_pushvalue(L, -2);
	/* execmeta, taskmeta, execmeta */
	tek_lua_register(L, NULL, tek_lib_exec_child_methods, 1);
	lua_pushvalue(L, -1);
	/* execmeta, taskmeta, taskmeta */
	lua_setfield(L, -2, "__index");
	lua_pop(L, 1);
	/* execmeta */
#endif
	
	lexec = lua_newuserdata(L, sizeof(struct LuaExecTask));
	/* execmeta, luaexec */
	lexec->exec = TNULL;
	
	lua_pushvalue(L, -1);
	/* execmeta, luaexec, luaexec */
	lua_pushvalue(L, -3);
	/* execmeta, luaexec, luaexec, execmeta */
	tek_lua_register(L, "tek.lib.exec", tek_lib_exec_funcs, 2);
	/* execmeta, luaexec, libtab */

	lua_pushvalue(L, -2);
	/* execmeta, luaexec, libtab, libtab */
	lua_pushvalue(L, -4);
	/* execmeta, luaexec, libtab, libtab, execmeta */
	lua_remove(L, -4);
	lua_remove(L, -4);
	/* libtab, libtab, execmeta */

	lua_setmetatable(L, -2);
	/* libtab, libtab */
	lua_setfield(L, -2, "base");
	/* libtab */

	tags[0].tti_Tag = TExecBase_ModInit;
	tags[0].tti_Value = (TTAG) tek_lib_exec_initmodules;
	tags[1].tti_Tag = TTAG_DONE;

	lexec->task = TEKCreate(tags);
	if (lexec->task == TNULL)
		luaL_error(L, "Failed to initialize TEKlib");
	lexec->exec = TExecBase = TGetExecBase(lexec->task);

#if defined(ENABLE_TASKS)
	if (!tek_lib_exec_init_link_to_parent(L, lexec))
	{
		lua_pop(L, 1);
		return 0;
	}
#endif

	return 1;
}
