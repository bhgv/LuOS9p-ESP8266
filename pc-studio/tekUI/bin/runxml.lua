#!/usr/bin/env lua

--
--	runxml.lua - XML to Lua+tekUI converter, runner, and checker
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--

local _VERSION = "runxml 1.0"

local ui = require "tek.ui"
local db = require "tek.lib.debug"
local Args = require "tek.lib.args"
require "lxp.lom"

local fdout = io.stdout

local function out(...)
	fdout:write(...)
end

local function outf(fmt, ...)
	fdout:write(fmt:format(...))
end

local initonlyattrs = 
{
	ActivateOnRMB = "boolean",
	AutoPosition = "boolean",
	EraseBG = "boolean",
	InitialFocus = "boolean",
	TrackDamage = "boolean",
}

local function checkattribute(class, attr, val)
	if initonlyattrs[attr] then
		return true
	end
	return pcall(function() ui[class]:new()[attr] = val or "Test" end)
end

local function assertattribute(class, attr, val)
	assert(checkattribute(class, attr, val),  
		"*** Class " .. class .. " has no attribute '".. attr .."'")
end

local function assertclass(class)
	assert(pcall(function() ui[class]:new() end), "*** Unknown class '"..class.."'")
end



local template = "SRC,EXECUTE=-e/S,CHECK=-c/S,VERSION=-v/S,HELP=-h/S"
local args = Args.read(template, arg)
if not args or args["-h"] then
	print "Usage: runxml.lua [options] xml-src"
	print "Options:"
	print "  -e   pass code on to the Lua interpreter"
	print "  -c   perform runtime validity checks against tekUI implementation"
	print "  -v   show version info and exit"
	print "  -h   this help"
	print "Converts well-formed XML to a tekUI application's Lua code."
	return os.exit(1)
end

if args.version then
	print(_VERSION)
	return os.exit(1)
end

local fname = args.src
if not fname then
	print "No file specified."
	return os.exit(1)
end
local file = io.open(fname)
if not file then
	print("Error reading '" .. fname .. "'")
	return os.exit(1)
end

if args.execute then
	fdout = io.popen("lua", "w")
end


local dom = lxp.lom.parse(file:read("*a"))
if not dom then
	print("Ill-formed XML")
	return os.exit(1)
end

local function recurse(tab, indent)
	indent = indent or 0
	local tag = tab.tag:lower()
	
	assertclass(tag)
	
	local is = ("\t"):rep(indent)
	local is2 = ("\t"):rep(indent + 1)

	outf("%sui.%s:new {\n", is, tag)

	for i, a in ipairs(tab.attr) do
		if args.check then
			assertattribute(tag, a, "Test")
		end
		outf('%s%s = "%s",\n', is2, a, tab.attr[a])
	end

	local children = { }

	for i, e in ipairs(tab) do
		if type(e) == "table" then
			if e.tag == "method" then
				local name = e.attr.Name
				local data = table.concat(e)
				assert(name, "Need a 'Name' attribute for a method")

				local argnames = { }
				argnames[1] = "self"
				for i = 1, 16 do
					local argname = "Arg" .. i
					local a = e.attr[argname]
					if a then
						argnames[i] = a
					elseif i > 1 then
						break
					end
				end

				outf('%s%s = function(%s)\n', is2, name, table.concat(argnames, ", "))
				outf('%s\n', data)
				outf('%send,\n', is2)

			else
				table.insert(children, e)
			end
		else
			local d = e:match("^%s*(.-)%s*$")
			if d ~= "" then
				if checkattribute(tag, "Text", "Test") then
					outf('%sText = "%s",\n', is2, d)
				end
			end
		end
	end

	if #children > 0 then
-- 		assertattribute(tag, "Children", { })
		local single = tag == "scrollgroup" or tag == "canvas"
		assert(not single or #children == 1)
		if single then
			outf("%sChild = \n", is2)
			recurse(children[1], indent + 1)
		else
			outf("%sChildren = {\n", is2)
			for i, e in ipairs(children) do
				recurse(e, indent + 2)
			end
		end
		if not single then
			outf("%s},\n", is2)
		end
	end

	if indent == 0 then
		outf('%s}\n', is)
	else
		outf('%s},\n', is)
	end
end

outf('local ui = require "tek.ui"\n')
outf('local app = ')

if dom.tag:lower() == "tekui:application" then
	for i, e in ipairs(dom) do
		if e.tag then
			local tag = e.tag:lower()
			if tag == "application" then
				recurse(e)
			elseif tag == "notifications" then
				for i, e in ipairs(e) do
					if e.attr then

						local id = e.attr.Id
						local attr = e.attr.Attribute
						local value = e.attr.Value
						local method = e.attr.Method
						
						assert(id, "need 'Id' attribute for notification")
						assert(attr, "need 'Attribute' attribute for notification")
						assert(value, "need 'Value' attribute for notification")
						assert(method, "need 'Method' attribute for notification")

						local argnames = { }
						local target = e.attr.Target or "self"
						if target == "self" then
							table.insert(argnames, "ui.NOTIFY_SELF")
						elseif target == "window" then
							table.insert(argnames, "ui.NOTIFY_WINDOW")
						elseif target == "application" then
							table.insert(argnames, "ui.NOTIFY_APPLICATION")
						else
							table.insert(argnames, "ui.NOTIFY_ID")
							table.insert(argnames, '"' .. target .. '"')
						end

						table.insert(argnames, '"' .. method .. '"')

						if value == "true" then
							value = "true"
						elseif value == "false" then
							value = "false"
						elseif value then
							value = '"' .. value .. '"'
						else
							value = "ui.NOTIFY_ALWAYS"
						end

						for i = 1, 16 do
							local argname = "Arg" .. i
							local a = e.attr[argname]
							if a then
								table.insert(argnames, '"' .. a .. '"')
							elseif i > 1 then
								break
							end
						end

						outf('app:getById("%s"):addNotify("%s", %s, { %s } )\n',
							id, attr, value, table.concat(argnames, ", "))

					end
				end

			end
		end
	end
else
	recurse(dom)
end

outf('app:run()\n')

return os.exit(0)
