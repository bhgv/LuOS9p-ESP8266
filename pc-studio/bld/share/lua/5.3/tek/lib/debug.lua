-------------------------------------------------------------------------------
--
--	tek.lib.debug
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	OVERVIEW::
--		Debug library - implements debug output and debug levels:
--
--		2  || TRACE || used for trace messages
--		3  || INFO  || informational messages, notices
--		4  || NOTE  || important notices
--		5  || WARN  || something unexpected happened
--		10 || ERROR || something went wrong, e.g. resource unavailable
--		20 || FAIL  || something went wrong that cannot be coped with
--
--		The default debug level is {{WARN}}. To set the debug level
--		globally, e.g.:
--				db = require "tek.lib.debug"
--				db.level = db.INFO
--
--		The default debug output stream is {{stderr}}.
--		To override it globally, e.g.:
--				db = require "tek.lib.debug"
--				db.out = io.open("logfile", "w")
--
--	FUNCTIONS::
--		- debug.console() - Enters the debug console
--		- debug.dump() - Dumps a table recursively
--		- debug.error() - Prints a text in the {{ERROR}} debug level
--		- debug.execute() - Executes a function in the specified debug level
--		- debug.fail() - Prints a text in the {{FAIL}} debug level
--		- debug.info() - Prints a text in the {{INFO}} debug level
--		- debug.note() - Prints a text in the {{NOTE}} debug level
--		- debug.print() - Prints a text in the specified debug level
--		- debug.stacktrace() - Prints a stacktrace in the specified debug level
--		- debug.trace() - Prints a text in the {{TRACE}} debug level
--		- debug.warn() - Prints a text in the {{WARN}} debug level
--		- debug.wrout() - Output function
--
--	VALUES::
--		- {{level}} - Debug level, default 5 ({{WARN}})
--		- {{out}} - Output stream, default {{io.stderr}}
--
-------------------------------------------------------------------------------

local debug = require "debug"
local getinfo = debug.getinfo
local stderr = io.stderr
local pairs = pairs
local select = select
local tonumber = tonumber
local tostring = tostring
local traceback = debug.traceback
local type = type
local unpack = unpack or table.unpack

--[[ header token for documentation generator:
module "tek.lib.debug"
local Debug = _M
]]

local Debug = { }
Debug._VERSION = "Debug 6.2"

-- symbolic:

Debug.ALWAYS = 0
Debug.TRACE = 2
Debug.INFO = 3
Debug.NOTE = 4
Debug.WARN = 5
Debug.ERROR = 10
Debug.FAIL = 20

-- global defaults:

Debug.level = Debug.WARN
Debug.out = stderr

-------------------------------------------------------------------------------
--	debug.wrout(lvl, ...): Debug output function, by default
--			function(...) out:write(...) end
-------------------------------------------------------------------------------

Debug.wrout = function(lvl, ...) Debug.out:write(...) end

-------------------------------------------------------------------------------
--	debug.format(lvl, msg, ...): Format error message
-------------------------------------------------------------------------------

function Debug.format(lvl, msg, ...)
	local t = getinfo(4, "lS")
	return ("(%02d %s:%d) " .. msg .. "\n"):format(lvl, t.short_src,
		t.currentline, ...)
end

-------------------------------------------------------------------------------
--	debug.print(lvl, msg, ...): Prints formatted text if the global debug level
--	is less or equal the specified level.
-------------------------------------------------------------------------------

function Debug.print(lvl, msg, ...)
	if Debug.level and lvl >= Debug.level then
		local t = getinfo(3, "lS")
		local arg = { }
		for i = 1, select('#', ...) do
			local v = select(i, ...)
			arg[i] = v and type(v) ~= "number" and tostring(v) or v or 0
		end
		Debug.wrout(lvl, Debug.format(lvl, msg, unpack(arg)))
	end
end

-------------------------------------------------------------------------------
--	debug.execute(lvl, func, ...): Executes the specified function if the
--	global debug level is less or equal the specified level.
-------------------------------------------------------------------------------

function Debug.execute(lvl, func, ...)
	if Debug.level and lvl >= Debug.level then
		return func(...)
	end
end

-------------------------------------------------------------------------------
--	debug.trace(msg, ...): Prints formatted debug info with {{TRACE}} debug
--	level
-------------------------------------------------------------------------------

function Debug.trace(msg, ...) Debug.print(2, msg, ...) end

-------------------------------------------------------------------------------
--	debug.info(msg, ...): Prints formatted debug info with {{INFO}} debug level
-------------------------------------------------------------------------------

function Debug.info(msg, ...) Debug.print(3, msg, ...) end

-------------------------------------------------------------------------------
--	debug.note(msg, ...): Prints formatted debug info with {{NOTE}} debug level
-------------------------------------------------------------------------------

function Debug.note(msg, ...) Debug.print(4, msg, ...) end

-------------------------------------------------------------------------------
--	debug.warn(msg, ...): Prints formatted debug info with {{WARN}} debug level
-------------------------------------------------------------------------------

function Debug.warn(msg, ...) Debug.print(5, msg, ...) end

-------------------------------------------------------------------------------
--	debug.error(msg, ...): Prints formatted debug info with {{ERROR}} debug
--	level
-------------------------------------------------------------------------------

function Debug.error(msg, ...) Debug.print(10, msg, ...) end

-------------------------------------------------------------------------------
--	debug.fail(msg, ...): Prints formatted debug info with {{FAIL}} debug level
-------------------------------------------------------------------------------

function Debug.fail(msg, ...) Debug.print(20, msg, ...) end

-------------------------------------------------------------------------------
--	debug.stacktrace(debuglevel[, stacklevel]): Prints a stacktrace starting at
--	the function of the given level on the stack (excluding the
--	{{stracktrace}} function itself) if the global debug level is less
--	or equal the specified {{debuglevel}}.
-------------------------------------------------------------------------------

function Debug.stacktrace(lvl, level)
	Debug.print(lvl, traceback("", level or 1 + 1))
end

-------------------------------------------------------------------------------
--	debug.console(): Enters debug console.
-------------------------------------------------------------------------------

function Debug.console()
	Debug.wrout(10, 'Entering the debug console.\n')
	Debug.wrout(10, 'To redirect the output, e.g.:\n')
	Debug.wrout(10, '  db = require "tek.lib.debug"\n')
	Debug.wrout(10, '  db.out = io.open("logfile", "w")\n')
	Debug.wrout(10, 'To dump a table, e.g.:\n')
	Debug.wrout(10, '  db.dump(app)\n')
	Debug.wrout(10, 'Use "cont" to continue.\n')
	debug.debug()
end

-------------------------------------------------------------------------------
--	debug.dump(table[, outfunc]): Dumps a table as Lua source using
--	{{outfunc}}. Cyclic references are silently dropped. The default output
--	function is debug.wrout().
-------------------------------------------------------------------------------

local function f_encodenascii(c)
	return ("\\%03d"):format(c:byte())
end

local function encode(s)
	return s:gsub('([%z\001-\031\092"])', f_encodenascii)
end

local function dumpr(tab, indent, outfunc, saved)
	saved[tab] = tab
	local is = ("\t"):rep(indent)
	for key, val in pairs(tab) do
		if not saved[val] then
			outfunc(is)
			local t = type(key)
			if t == "number" or t == "boolean" then
				outfunc('[' .. tostring(key) .. '] = ')
			elseif t == "string" then
				if key:match("[^%a_]") then
					outfunc('["' .. encode(key) .. '"] = ')
				else
					outfunc(key .. ' = ')
				end
			else
				outfunc('["' .. tostring(key) .. '"] = ')
			end
			t = type(val)
			if t == "table" then
				outfunc('{\n')
				dumpr(val, indent + 1, outfunc, saved)
				outfunc(is .. '},\n')
			elseif t == "string" then
				outfunc('"' .. encode(val) .. '",\n')
			elseif t == "number" or t == "boolean" then
				outfunc(tostring(val) .. ',\n')
			else
				outfunc('"' .. tostring(val) .. '",\n')
			end
		end
	end
end

function Debug.dump(tab, outf)
	if not outf then
		outf = function(...) Debug.wrout(10, ...) end
	end
	dumpr(tab, 0, outf, { })
end

return Debug
