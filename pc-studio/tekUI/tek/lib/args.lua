-------------------------------------------------------------------------------
--
--	tek.lib.args
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	OVERVIEW::
--		This library implements an argument parser.
--
--	FORMAT DESCRIPTION::
--		A string is parsed into an array of arguments according to a format
--		template. Arguments in the template are separated by commas. Each
--		argument in the template consists of a keyword, optionally followed by
--		one or more aliases delimited by equal signs, and an optional set of
--		modifiers delimited by slashes. Modifiers denote the expected
--		data type, if the argument is mandatory and whether a keyword must
--		precede its value to form a valid argument. Example argument template:
--
--				SOURCE=-s/A/M,DEST=-d/A/K
--
--		This template requires one or more arguments to satisfy {{SOURCE}},
--		and exactly one argument to satisfy {{DEST}}. Neither can be omitted.
--		Either {{-d}} (or its alias {{DEST}}) must precede the value to be
--		accepted as the second argument. Example command lines:
--
--		{{SOURCE one two three DEST foo}} || valid
--		{{DEST foo -s one}}               || valid
--		{{DEST foo}}                      || rejected; source argument missing
--		{{one two three foo}}             || rejected; keyword missing
--		{{one two dest foo}}              || valid, keys are case insensitive
--		{{one two three -d="foo" four}}   || valid, "four" added to {{SOURCE}}
--
--		An option without modifiers represents an optional string argument.
--		Available modifiers are:
--
--			* {{/S}} - ''Switch''; considered a boolean value. When the
--			keyword is present, '''true''' will be written into the
--			respective slot in the results array.
--			* {{/N}} - ''Number''; the value will be converted to a number.
--			* {{/K}} - ''Keyword''; the keyword (or an alias) must precede its
--			value.
--			* {{/A}} - ''Always required''; This argument cannot be omitted.
--			* {{/M}} - ''Multiple''; Any number of strings will be accepted,
--			and all values that cannot be assigned to other arguments will
--			show up in this argument. No more than one {{/M}} modifier may
--			appear in a template.
--			* {{/F}} - ''Fill''; literal rest of line. If present, this must
--			be the last argument.
--
--		Quoting and escaping: Double quotes can be used to enclose arguments
--		containing equal signs and spaces.
--
--	FUNCTIONS::
--		- args.read() - Parses a string or table through an argument template
--	
--	TODO::
--		Backslashes for escaping quotes, spaces and themselves
--
-------------------------------------------------------------------------------

local concat = table.concat
local insert = table.insert
local remove = table.remove
local tonumber = tonumber
local type = type

-- for testing (see below):
-- local pairs = pairs
-- local print = print
-- local ipairs = ipairs

--[[ header token for documentation generator:
module "tek.lib.args"
local Args = _M
]]

local Args = { }
Args._VERSION = "Args 2.5"

-------------------------------------------------------------------------------
--	parsetemplate: check validity and parse option template into a table
--	with records of flags and keys
-------------------------------------------------------------------------------

local function parsetemplate(s)
	if type(s) ~= "string" then
		return nil, "Invalid template"
	end
	local t = { }
	local n = 1
	for arg in s:gmatch("([^,]+),?") do
		local keys, mod = arg:match("^([^/]*)(/?[/aksnmfAKSNMF]*)$")
		if not keys then
			return nil, "Invalid modifiers in template"
		end
		mod = mod:gsub("/", ""):lower()
		local r = { { }, false }
		mod:gsub(".", function(a) 
			r[a] = true
		end)
		for alias in keys:gmatch("([^=]+)=?") do
			if not alias:match("^[%a-_][%w%-_]*") then
				return nil, "Invalid characters in keys"
			end
			alias = alias:lower()
			insert(r[1], alias)
			if t[alias] then
				return nil, "Double key in template"
			end
			t[alias] = n
		end
		insert(t, r)
		n = n + 1
	end
	insert(t, { { }, false, m = true })
	return t
end

-------------------------------------------------------------------------------
--	items: iterator over items in argument string or table
-------------------------------------------------------------------------------

local function items(rest)
	if type(rest) == "table" then
		return function()
			local r = concat(rest, " ")
			local s = remove(rest, 1)
			return s, r
		end
	end
	return function()
		if rest then
			local oldrest = rest
			local arg, newa = rest:match('^%s*"([^"]*)"%s*=?%s*(.*)')
			if not newa then
				arg, newa = rest:match("^%s*([^%s=]+)%s*=?%s*(.*)")
			end
			rest = newa ~= "" and newa or nil
			return arg, oldrest
		end
	end
end

-------------------------------------------------------------------------------
--	res, msg = args.read(template, args): Parses {{args}} according to
--	{{template}}. {{args}} can be a string or a table of strings. If the
--	arguments can be successfully matched against the template, the result
--	will be a table of parsed arguments, indexed by both keywords and
--	numerical order in which they appear in {{template}}, and the second
--	argument will be set to the number of arguments in the template. If the
--	arguments cannot be matched against the template, the result will be
--	'''nil''' followed by an error message.
-------------------------------------------------------------------------------

function Args.read(template, args)
	local tmpl, msg = parsetemplate(template)
	if not tmpl then
		return nil, msg
	end
	
	local nextitem, msg = items(args)
	if not nextitem then
		return nil, msg
	end
	
	local multi = { }
	local nextn = 1
	local done
	local n
	
	repeat
		while true do
			
			local waitkey
			
			n = nextn
			nextn = n + 1
			
			local t = tmpl[n]
			if not t then
				done = true -- no more in template
				break
			end
			
			if t[2] and not t.m then
				break -- current argument already filled
			end
			
			if t.s or t.k then
				break -- wait for arg
			end
			
			-- get next value
			local item, rest = nextitem()
			if not item then
				done = true
				break -- no more items
			end
			
			local rec = tmpl[item:lower()]
			if rec then
				-- got key
				waitkey = rec
				rec = tmpl[rec]
			end
			
			if rec and (not rec[2] or rec.m) then
				nextn = n -- retry in next turn
				n = waitkey
				t = tmpl[n]
				if not rec.s then
					item, rest = nextitem()
					if not item then
						return nil, "Value expected for key : " .. t[1][1]
					end
				end
			elseif waitkey then
				return nil, "Key expected : " .. tmpl[waitkey][1][1]
			end
			
			if t.m then
				if t.k or waitkey then
					t[2] = t[2] or { }
					insert(t[2], item)
				else
					insert(multi, item)
				end
				nextn = n -- redo
			elseif t.f then
				t[2] = rest
				done = true
				break
			elseif t.n then
				if not item:match("^%-?%d*%.?%d*$") then
					return nil, "Number expected"
				end
				t[2] = tonumber(item)
			elseif t.s then
				t[2] = true
			else
				t[2] = item
			end
		end
	until done
	
	-- unfilled /A steal from multi:
	for i = #tmpl - 1, 1, -1 do
		local t = tmpl[i]
		if not t[2] and t.a and not t.m then
			if not t.k then
				t[2] = remove(multi)
			end
			if not t[2] then
				return nil, "Required argument missing : " .. t[1][1]
			end
		end
	end
	
	-- put remaining multi into /M:
	for i = 1, #tmpl - 1 do
		local t = tmpl[i]
		if t.m then
			if not t.k or t[2] then
				t[2] = t[2] or { }
				while #multi > 0 do
					local val = remove(multi, 1)
					insert(t[2], val)
				end
				if t.a and #t[2] == 0 then
					return nil, "Required argument missing : " .. t[1][1]
				end
			end
		end
	end
	
	-- arguments left?
	if n == #tmpl and #multi > 0 then
		return nil, "Too many arguments"
	end

	local res = { }
	for i = 1, #tmpl - 1 do
		local t = tmpl[i]
		res[i] = t[2]
		for j = 1, #t[1] do
			local key = t[1][j]
			res[key] = res[i]
		end
	end
	
	return res, #tmpl - 1
end

-------------------------------------------------------------------------------
--	unit tests:
-------------------------------------------------------------------------------

-- local function test(template, args, expected)
-- 	if expected == false then
-- 		expected = nil
-- 	end
-- 	local function result(status)
-- 		print(status .. ': argparse("'..template..'", "'..args..'")')
-- 		return status
-- 	end
-- 	local res = Args.read(template, args)
-- 	if res == expected then
-- 		return result("ok")
-- 	elseif not res or not expected then
-- 		return result("failed")
-- 	else
-- 		for key, val in pairs(expected) do
-- 			if type(val) == "table" then
-- 				if #val ~= #res[key] then
-- 					return result("failed")
-- 				end
-- 				for i, v in ipairs(val) do
-- 					if res[key][i] ~= v then
-- 						return result("failed")
-- 					end
-- 				end
-- 			else
-- 				if res[key] ~= val then
-- 					return result("failed")
-- 				end
-- 			end
-- 		end
-- 		return result("ok")
-- 	end
-- end
-- 
-- test("bla,fasel", "1", {bla="1"})
-- test("bla,fasel", "1 2", {bla="1",fasel="2"})
-- test("bla,fasel", "1 2 3", false)
-- test("bla,fasel", "bla fasel fasel bla", {bla="fasel",fasel="bla"})
-- test("bla,fasel", "bla fasel fasel bla blub", false)
-- test("src/m/a,dst/a", "1", false)
-- test("src/m/a,dst/a", "1 2", {src={"1"},dst="2"})
-- test("src/m/a,dst/a", "1 2 3", {src={"1","2"},dst="3"})
-- test("src=-s/m/a,dst=-d/a", "-s 1 -s 2 -d 3 4", {src={"1","2","4"},dst="3"})
-- test("src=-s/m/a/k,dst=-d/a/k", "-s 1 -s 2 -d 3", {src={"1","2"},dst="3"})
-- test("src=-s/m/a/k,dst=-d/a/k", "-s 1 -s 2 3", false)
-- test("bla/a,fasel/a", "1", false)
-- test("bla/a,fasel/a", "1 2", {bla="1",fasel="2"})
-- test("bla/a/k,fasel/a/k", "bla 1 fasel 2", {bla="1",fasel="2"})
-- test("bla/a/k,fasel/a/k", "bla 1 fasel", false)
-- test("bla/a/k,fasel/a", "bla 1 fasel", false)
-- test("SOURCE=-s/A/M,DEST=-d/A/K", "SOURCE one two three DEST foo", {source={"one","two","three"},dest="foo"})
-- test("SOURCE=-s/A/M,DEST=-d/A/K", "DEST foo -s one", {source={"one"},dest="foo"})
-- test("SOURCE=-s/A/M,DEST=-d/A/K", "one two three foo", false)
-- test("SOURCE=-s/A/M,DEST=-d/A/K", "one two dest foo", {source={"one","two"},dest="foo"})
-- test("SOURCE=-s/A/M,DEST=-d/A/K", 'one two three -d="foo" four', {source={"one","two","three","four"},dest="foo"})
-- test("a/s,b/s", "a b", {a=true, b=true})
-- test("file/k,bla/k", "file foo bla fasel", {file="foo", bla="fasel"})
-- test("file/k,bla/k,a/s", "file foo bla fasel", {file="foo", bla="fasel"})
-- test("file=-f/k,bla/k,a/s", "-f foo bla fasel a", {file="foo", bla="fasel", a=true})
-- test("file=-f/k,bla/k,a/s", "-f foo a", {file="foo", a=true})
-- test("file=-f/k,bla/k,a/s", "bla fasel a", {bla="fasel", a=true})
-- test("file=-f/k,silent=-s/s,targets/m", "-f file -s", {file="file", silent=true, targets={}})
-- test("file=-f/k,silent=-s/s,targets/m", "-f file -s eins", {file="file", silent=true, targets={"eins"}})
-- test("file=-f/k,silent=-s/s,targets/m", "-f file eins", {file="file", targets={"eins"}})
-- test("file=-f,silent=-s/s,targets/m", "eins", {file="eins"})
-- test("file=-f/k,silent=-s/s,targets/m", "targets eins", {targets={"eins"}})
-- test("file=-f/k,silent=-s/s,targets/m", "eins", {targets={"eins"}})
-- test("file=-f/k,silent=-s/s,targets/m", "eins", {targets={"eins"}})
-- test("file=-f/k,silent=-s/s,targets/m", "eins -f file", {file="file", targets={"eins"}})
-- test("PATHNAME,MD5SUM/K", "hallo", {pathname="hallo"})
-- test("PATHNAME,MD5SUM/K", "md5sum hallo", {md5sum="hallo"})
-- test("foo/a,bar/m", "eins zwei", {foo="eins", bar={"zwei"}})
-- test("foo/a,bar/k/m", "eins bar zwei", {foo="eins", bar={"zwei"}})
-- test("IP/A,MASK/K,DNS/M", "IP 1 MASK 2 3 4", {ip="1", mask="2", dns={"3", "4"}})
-- test("IP/A,MASK/K,DNS/M", "IP 1 MASK 2 DNS 3 4", {ip="1", mask="2", dns={"3", "4"}})
-- test("IP/A,MASK/K,DNS/K/M", "IP 1 MASK 2 DNS 3 4", {ip="1", mask="2", dns={"3", "4"}})
-- test("src=-s/m/a/k,dst=-d/a/k", "-s 1 2 -d 3", {src={"1","2"},dst="3"})
-- test("src=-s/m/a/k,dst=-d/a/k", "-d 3 -s 1 2", {src={"1","2"},dst="3"})
-- test("src=-s/m/a/k,dst=-d/a/k", "1 -d 3 -s 2", {src={"2","1"},dst="3"})
-- test("src=-s/m/a,dst=-d/a", "1 2 3 4", {src={"1","2","3"},dst="4"})
-- test("src=-s/m/a,dst=-d/a", "1 2 3 -d 4", {src={"1","2","3"},dst="4"})
-- test("src=-s/m/a,dst=-d/a", "1 2 -d 3 4", {src={"1","2","4"},dst="3"})
-- test("src=-s/m/a/k,dst=-d/a/k", "-s 1 -s 2 -d 3", {src={"1","2"},dst="3"})
-- test("src=-s/m/a/k,dst=-d/a/k", "1 2 -d 3", false)
-- test("foo/a,bar/k/m", "eins zwei", false)
-- test("src=-s/m/a/k,dst=-d/a/k", "1 2 -d 3", false)
-- test("hallo", "hallo", false)
-- -- test("src=-s/m/a/k,dst=-d/a/k", "1 -d 3 -s 2", {src={"1","2"},dst="3"})
-- test("src=-s/m/a/k,dst=-d/a/k", "1 -d 3 -s 2", {src={"2","1"},dst="3"})
-- -- test("src=-s/m/a,dst=-d/a", "1 2 3 -s 4", {src={"1","2","4"},dst="3"})
-- test("src=-s/m/a,dst=-d/a", "1 2 3 -s 4", {src={"4","1","2"},dst="3"})
-- test("SOURCE=-s/A/M,DEST=-d/A/K", "SOURCE one two three DEST foo", {source={"one","two","three"},dest="foo"})
-- test("IP/A,MASK/K,DNS/M", "IP 1 MASK 2 DNS 3 4", {ip="1", mask="2", dns={"3", "4"}})
-- test("IP/A,MASK/K,DNS/K/M", "IP 1 MASK 2 DNS 3 4", {ip="1", mask="2", dns={"3", "4"}})
-- test("src=-s/m/a/k,dst=-d/a/k", "1 -d 3 -s 2", {src={"2","1"},dst="3"})
-- test("src=-s/m/a,dst=-d/a", "1 2 3 -s 4", {src={"4","1","2"},dst="3"})
-- test("src=-s/m/a/k,dst=-d/a/k", "-s 1 2 -d 3", {src={"1","2"},dst="3"})
-- test("src=-s/m/a/k,dst=-d/a/k", "-d 3 -s 1 2", {src={"1","2"},dst="3"})
-- test("src=-s/m/a,dst=-d/a", "-s 1 -s 2 -d 3 4", {src={"1","2","4"},dst="3"})

return Args
