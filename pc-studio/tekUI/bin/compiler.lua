#!/usr/bin/env lua

--
--	compiler.lua - Lua compiler and module linker
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--

local Args = require "tek.lib.args"
local _, exec = pcall(function() return require "tek.lib.exec" end)
local _, lfs = pcall(function() return require "lfs" end)
local unpack = unpack or table.unpack
local loadstring = loadstring or load
local floor = math.floor
local char = string.char

-------------------------------------------------------------------------------

local PS = package and package.config:sub(1, 1) or "/"
local PM = "^(" .. PS .. "?.-)" .. PS .. "*([^" .. PS .. "]*)" .. PS .. "+$"

function splitpath(path)
	local part
	path, part = (path .. PS):match(PM)
	path = path:gsub(PS .. PS .. "*", PS)
	return path, part
end

function addpath(path, part)
	return path .. PS .. part
end

function stat(name, attr)
	return lfs.attributes(name, attr)
end

-------------------------------------------------------------------------------

local function link(fname, arg)
	local srcname
	while true do
		local arg = table.remove(arg, 1)
		if not arg or arg == "-L" then
			break
		end
		srcname = arg
	end
	local o = io.open(fname, "wb")
	if not o then
		s:close()
		io.stderr:write("cannot open ", fname, "\n")
		return
	end
	o:write("local _ENV = _ENV\n")
	for i = 1, #arg do
		local modname, fname = arg[i]:match("^([^:]*):(.*)$")
		local s = io.open(fname, "rb")
		if not s then
			error("cannot open " .. fname)
		end
		o:write('package.preload["' .. modname .. 
			'"] = function(...) local arg = _G.arg\n')
		for l in s:lines() do
			o:write(l, "\n")
		end
		o:write("end\n")
		s:close()
	end
	local s = srcname and io.open(srcname, "rb")
	if s then
		o:write("do\n")
		local lnr = 0
		for l in s:lines() do
			lnr = lnr + 1
			if lnr > 1 or not l:match("^#!") then
				o:write(l, "\n")
			end
		end
		o:write("end\n")
		s:close()
	end
	o:close()
end

-------------------------------------------------------------------------------

function tocsource(outname)
	local b
	local f = io.open(outname, "rb")
	if f then
		b = f:read("*a")
		f:close()
	end
	if not b then
		return false
	end
	f = io.open(outname, "wb")
	if f then
		local size = b:len()
		local out = function(...) f:write(...) end
		out(("const unsigned char bytecode[%d] = {\n\t"):format(size))
		for i = 1, size do
			local c = string.byte(b:sub(i, i))
			out(("%d,"):format(c))
			if i % 32 == 0 then
				out("\n\t")
			end
		end
		out("\n};\n")
		f:close()
		return true
	end
end

-------------------------------------------------------------------------------

local function compile_internal(outname, strip)
	local tmpname = outname .. ".tmp"
	local c = loadfile(outname)
	if c then
		c = string.dump(c, strip)
		local f = io.open(tmpname, "wb")
		local success
		if f then
			success = f:write(c)
			f:close()
			if success then
				os.remove(outname)
				os.rename(tmpname, outname)
				return true
			end
		end
	end
end

local function compile_external(outname, strip)
	local tmpname = outname .. ".tmp"
	local cmd = ('luac %s -o "%s" "%s"'):format(strip and "-s" or "",
		tmpname, outname)
	if os.execute(cmd) and stat(tmpname, "mode") == "file" then
		os.remove(outname)
		os.rename(tmpname, outname)
		return true
	end
end

function compile(outname, strip, internal)
	outname = outname:gsub("\\", "\\\\")
	local tmpname = outname .. ".tmp"
	local ver, rev = _VERSION:match("^Lua (%d+)%.(%d+)$")
	ver = tonumber(ver)
	rev = tonumber(rev)
	if internal or not strip or ver > 5 or (ver == 5 and rev >= 3) then
		return compile_internal(outname, strip)
	end
	if compile_external(outname, strip) then
		return true
	end
	io.stderr:write("error calling luac, compiling internally,\n")
	io.stderr:write("stripping may not be possible with Lua < 5.3\n")
	return compile_internal(outname, strip)
end

-------------------------------------------------------------------------------

local function ntobin(n)
	return char(floor(n / 0x1000000)) ..
		char(floor((n % 0x1000000) / 0x10000)) ..
		char(floor((n % 0x10000) / 0x100)) ..
		char(floor(n % 0x100))
end

local function toexecutable(outname, exe)
	if not exe then
		return false
	end
	local add, success
	local f = io.open(outname, "rb")
	if f then
		add = f:read("*a")
		f:close()
	end
	if not add then
		return false
	end
	local tmpname = outname .. ".tmp"
	f = io.open(tmpname, "wb")
	if not f then
		return false
	end
	f:write(exe)
	f:write(add)
	f:write(ntobin(add:len()))
	success = f:write("lo" .. char(0) .. char(0))
	f:close()
	if success then
		os.remove(outname)
		return os.rename(tmpname, outname)
	end
end

-------------------------------------------------------------------------------
--	main
-------------------------------------------------------------------------------

local template = "-f=FROM,-o=TO,-c=SOURCE/S,-nc=LUA/S,-s=STRIP/S,-i=INTERNAL/S,-l=LINK/M,-e=EXE/K,-h=HELP/S"
local args = Args.read(template, arg)
if not args or args["-h"] then
	print "Lua linker and compiler, with optional GUI"
	print "Available options:"
	print "  -f=FROM       Lua source file name"
	print "  -o=TO         Lua bytecode output file name"
	print "  -c=SOURCE/S   Output as C source"
	print "  -nc=LUA/S     Do not compile, output Lua"
	print "  -s=STRIP/S    Strip debug information"
	print "  -i=INTERNAL/S Do not call luac"
	print "  -l=LINK/M     List of modules to link, each as modname:filename"
	print "  -e=EXE/K      Executable file to which bytecode is appended"
	print "  -h=HELP/S     This help"
	return
end

local from, to, mods = args["-f"], args["-o"], args["-l"]

if from or (to and mods) then
	local ext = args["-c"] and ".c" or ".luac"
	to = to or (from:match("^(.*)%.lua$") or from) .. ext

	local t = { }
	if from then
		table.insert(t, from)
	end

	if mods then
		table.insert(t, "-L")
		for _, m in ipairs(mods) do
			table.insert(t, m)
		end
	end

	link(to, t)

	if not args["-nc"] then
		compile(to, args["-s"], args["-i"])
	end

	if args["-c"] then
		tocsource(to)
	end
	
	if args["-e"] then
		local f = io.open(args["-e"], "rb")
		local exe = f and f:read("*a")
		if exe then
			toexecutable(to, exe)
			f:close()
		end
	end

	return 0
end

-------------------------------------------------------------------------------
--	Run application to sample modules:
-------------------------------------------------------------------------------

local function startsample(fname)
	return exec.run(function()
		local mods = { }
		local fname = arg[1]
		arg[0] = fname
		arg[1] = nil
		local func = loadfile(fname)
		if func then
			pcall(func)
		end
		local pd = package.config:sub(1, 1) or "/"
		local _, ui = pcall(function() return require "tek.ui" end)
		local path = ui and ui.LocalPath or package.path
		for pkg in pairs(package.loaded) do
			local mod = pkg:gsub("%.", pd)
			for mname in path:gmatch("([^;]+);?") do
				local fname = mname:gsub("?", mod)
				if io.open(fname, "rb") then
					table.insert(mods, ("%s = %s"):format(pkg, fname))
					break
				end
			end
		end
		return table.concat(mods, "\n")	
	end, fname)
end

local function finishsample(c)
	local success, res = c:join()
	local mods = { }
	if res then
		for mod in res:gmatch("([^\n]*)\n?") do
			if mod ~= "" then
				table.insert(mods, mod)	
			end
		end
	end
	-- absorb signals so that they won't raise exceptions later:
	exec.getsignals("actm")
	return mods
end

-------------------------------------------------------------------------------
--	GUI main
-------------------------------------------------------------------------------

local ui = require "tek.ui"
local List = require "tek.class.list"
local Input = ui.Input
local APP_ID = "lua-compiler"
local DOMAIN = "schulze-mueller.de"
local PROGNAME = "Lua Compiler"
local VERSION = "2.0"
local AUTHOR = "Timm S. Müller"
local COPYRIGHT = "© 2009-2016, Schulze & Müller GbR"

local f = io.open(ui.ProgDir .. "tekui.exe", "rb")
local tekui_exe = f and f:read("*a")
if f then
	f:close()
end

-------------------------------------------------------------------------------
--	FileButton class:
-------------------------------------------------------------------------------

local FileButton = ui.ImageWidget:newClass()

function FileButton.init(self)
	self.Image = ui.getStockImage("file")
	self.Mode = "button"
	self.Class = "button"
	self.Height = "fill"
	self.Width = "fill"
	self.MinWidth = 15
	self.MinHeight = 17
	self.ImageAspectX = 5
	self.ImageAspectY = 7
	self.Style = "padding: 2"
	return ui.ImageWidget.init(self)
end

function FileButton:onClick()
	self.Application:addCoroutine(function()
		self:doRequest()
	end)
end

function FileButton:doRequest()
end

-------------------------------------------------------------------------------
--	CompilerApp class:
-------------------------------------------------------------------------------

local CompilerApp = ui.Application:newClass()

function CompilerApp.new(class, self)
	self = self or { }
	self.Settings = self.Settings or { }
	self.Settings.ModulePath = self.Settings.ModulePath or ""
	self.Settings.OutFileName = self.Settings.OutFileName or ""
	return ui.Application.new(class, self)
end

function CompilerApp:addModule(classname, filename, enable)
	enable = enable == nil or enable
	local group = self:getById("group-modules")
	for i = 1, #group.Children, 3 do
		local c = group.Children[i]
		if c.Text == classname then
			return 0 -- already present
		end
	end
	local filefield = Input:new
	{
		Disabled = not enable,
		Text = filename,
		Width = "free",
		VAlign = "center",
	}
	local selectbutton = FileButton:new
	{
		Disabled = not enable,
		doRequest = function(self)
			local app = self.Application
			local path, part = splitpath(filefield:getText())
			local status, path, select = app:requestFile
			{
				Title = "Select Lua module...",
				Path = path,
				Location = part
			}
			if status == "selected" and select[1] then
				app.Settings.ModulePath = path
				filefield:setValue("Enter", addpath(path, select[1]))
				app:setStatus("Module selected.")
			end
		end,
	}
	local checkmark = ui.CheckMark:new
	{
		Width = "fill",
		Selected = enable,
		Text = classname,
		onSelect = function(self)
			local app = self.Application
			ui.CheckMark.onSelect(self)
			local selected = self.Selected
			filefield:setValue("Disabled", not selected)
			selectbutton:setValue("Disabled", not selected)
			local n = app:selectModules()
			app:setStatus("%d modules selected", n)
		end,
	}
	group:addMember(checkmark)
	group:addMember(filefield)
	group:addMember(selectbutton)
	if #group.Children == 3 then
		self:getById("button-all"):setValue("Disabled", false)
		self:getById("button-none"):setValue("Disabled", false)
		self:getById("button-invert"):setValue("Disabled", false)
	end
	self:selectModules()
	return 1
end

function CompilerApp:getExcludeList()
	local exclude = { }	
	local excludefname = self:getById("text-exclude-filename"):getText()
	if excludefname then
		local f = io.open(excludefname, "rb")
		if f then
			for line in f:lines() do
				line = line:match("^(.*)%:.*$")
				if line then
					table.insert(exclude, line)
					exclude[line] = #exclude
				end
			end
			f:close()
		end
	end
	return exclude
end

function CompilerApp:sample()
	local filefield = self:getById("text-filename")
	local fname = filefield:getText()
	local exclude = self:getById("check-exclude-modules").Selected and self:getExcludeList()
	self:addCoroutine(function()
		local c = startsample(fname)
		local mods = finishsample(c)
		local n = 0
		for _, mod in ipairs(mods) do
			local classname, filename = mod:match("^(.-)%s*=%s*(.*)$")
			local disable = exclude and exclude[classname]
			n = n + self:addModule(classname, filename, not disable)
		end
		self:setStatus("%d modules added.", n)
	end)
end

function CompilerApp:compile()

	self:addCoroutine(function()

		local savemode = -- 1=lua, 2=bytecode, 3=csource, 4=executable
			self:getById("popitem-savemode").SelectedLine
		local ext = ".luac"
		if savemode == 3 then
			ext = ".c"
		elseif savemode == 4 then
			ext = ".exe"
		end

		local srcname = self:getById("text-filename"):getText()
		local outname = self.Settings.OutFileName
		if outname == "" then
			outname = (srcname:match("^(.*)%.lua$") or srcname) .. ext
		else
			local _, srcfile = splitpath(srcname)
			local outpath = splitpath(outname)
			srcfile = (srcfile:match("^(.*)%.lua$") or srcfile) .. ext
			outname = addpath(outpath, srcfile)
		end

		local path, part = splitpath(outname)

		local status, path, select = self:requestFile
		{
			Title = "Select File to Save...",
			Path = path,
			Location = part,
			SelectText = "_Save",
		}

		if status == "selected" and select[1] then

			outname = addpath(path, select[1])

			local success, msg = true

			if io.open(outname, "rb") then
				success = self:easyRequest("Overwrite File",
					("%s\nalready exists. Overwrite it?"):format(outname),
					"_Overwrite", "_Cancel") == 1
			end

			if success then

				local group = self:getById("group-modules")
				local mods = { }
				local modgroup = group.Children
				success = true
				for i = 1, #modgroup, 3 do
					local checkbutton = modgroup[i]
					if checkbutton.Selected then
						local classname = checkbutton.Text
						local filename = modgroup[i + 1]:getText()
						if io.open(filename, "rb") then
							table.insert(mods, ("%s:%s"):format(classname, filename))
						else
							local text =
								filename == "" and "No filename specified" or
									"Cannot open file:\n" .. filename
							success = self:easyRequest("Error",
								"Error loading module " .. classname .. ":\n" ..
								text,
								"_Abort", "_Continue Anyway") == 1
						end
					end
					if not success then
						break
					end
				end

				if success then
					success, msg = pcall(link, outname,
						{ srcname, "-L", unpack(mods) })
					if not success then
						self:easyRequest("Error",
							"Compilation failed:\n" .. msg, "_Okay")
					else
						local tmpname = outname .. ".tmp"
						if savemode > 1 then
							local strip = self:getById("check-strip").Selected
							if not compile(outname, strip) then
								self:easyRequest("Error",
									"Error stripping file:\n" .. outname, "_Okay")
								success = false
							end
						end
						if success then
							local size = stat(outname, "size")
							if savemode == 4 then
								if toexecutable(outname, tekui_exe) then
									size = stat(outname, "size")
									self:setStatus("Executable saved, size: %d bytes", size)
								end
							elseif savemode == 3 then
								if tocsource(outname) then
									self:setStatus("C source saved, binary size: %d bytes", size)
								end
							elseif savemode == 2 then
								self:setStatus("Bytecode saved, size: %d bytes", size)
							else
								self:setStatus("Lua saved, size: %d bytes", size)
							end
						else
							self:setStatus("Error saving file.")
						end
					end
				end
			end
		end
		self.Settings.OutFileName = outname
	end)
end

function CompilerApp:setStatus(text, ...)
	self:getById("text-status"):setValue("Text", text:format(...))
end

function CompilerApp:deleteModules(mode)
	local g = self:getById("group-modules")
	local n, nd = 0, 0
	if mode == "all" then
		while #g.Children > 0 do
			g:remMember(g.Children[1])
			nd = nd + 1
		end
	elseif mode == "selected" then
		for i = #g.Children - 2, 1, -3 do
			local c = g.Children[i]
			if c.Selected then
				g:remMember(g.Children[i])
				g:remMember(g.Children[i])
				g:remMember(g.Children[i])
				nd = nd + 1
			else
				n = n + 1
			end
		end
	end
	if n == 0 then
		self:getById("button-all"):setValue("Disabled", true)
		self:getById("button-none"):setValue("Disabled", true)
		self:getById("button-invert"):setValue("Disabled", true)
	end
	self.Application:selectModules()
	return nd
end

function CompilerApp:selectModules(mode)
	local exclude = self:getExcludeList()
	local g = self:getById("group-modules")
	local n = 0
	for i = 1, #g.Children, 3 do
		local c = g.Children[i]
		if mode == "toggle" then
			c:setValue("Selected", not c.Selected)
		elseif mode == "all" then
			c:setValue("Selected", true)
		elseif mode == "none" then
			c:setValue("Selected", false)
		elseif mode == "exclude" then
			local classname = c.Text
			local disable = exclude and exclude[classname]
			c:setValue("Selected", not disable)
		end
		if c.Selected then
			n = n + 1
		end
	end
	self:getById("button-delete"):setValue("Disabled", n == 0)
	return n
end

-------------------------------------------------------------------------------
--	Application:
-------------------------------------------------------------------------------

CompilerApp:new
{
	ApplicationId = APP_ID,
	Domain = DOMAIN,
	Children =
	{
		ui.Window:new
		{
			Style = "width: 400; height: 300",
			HideOnEscape = true,
			Center = true,
			Orientation = "vertical",
			Id = "window-about",
			Status = "hide",
			Title = "About Compiler",
			Children =
			{
				ui.Text:new
				{
					Text = "About Lua Compiler",
					Style = "font: ui-large"
				},
				ui.Group:new
				{
					Orientation = "vertical",
					Children =
					{
						ui.Group:new
						{
							Orientation = "vertical",
							Legend = "Application Information",
							Children =
							{
								ui.ListView:new
								{
									HSliderMode = "auto",
									VSliderMode = "auto",
									Child = ui.Lister:new
									{
										SelectMode = "none",
										ListObject = List:new
										{
											Items =
											{
												{ { "ProgramName", PROGNAME } },
												{ { "Version", VERSION } },
												{ { "Author", AUTHOR } },
												{ { "Copyright", COPYRIGHT } },
											}
										}
									}
								},
								ui.ScrollGroup:new
								{
									VSliderMode = "auto",
									Child = ui.Canvas:new 
									{
										AutoWidth = true,
										Child = ui.FloatText:new
										{
											Text = [[
Earlier versions of this program were based on
luac.lua by Luiz Henrique de Figueiredo, recent versions
borrow from the method shown in lua-amalg by Philipp Janda.
											]]
										}
									}
								}
							}
						}
					}
				},
				ui.Button:new
				{
					InitialFocus = true,
					Text = "_Okay",
					Style = "width: fill",
					onClick = function(self)
						self.Window:setValue("Status", "hide")
					end
				}
			}
		},
		ui.Window:new
		{
			Id = "window-main",
			Title = "Lua compiler and module linker",
			Orientation = "vertical",
			HideOnEscape = true,
			onHide = function(self)
				local app = self.Application
				app:addCoroutine(function()
					if app:easyRequest("Exit Application",
						"Do you really want to\n" ..
						"quit the application?",
						"_Quit", "_Cancel") == 1 then
						app:quit()
					end
				end)
			end,
			Children =
			{
				ui.Group:new
				{
					Class = "menubar",
					Children =
					{
						ui.MenuItem:new
						{
							Text = "File",
							Children =
							{
								ui.MenuItem:new
								{
									Text = "About",
									Shortcut = "Ctrl+?",
									onClick = function(self)
										self:getById("window-about"):setValue("Status", "show")
									end
								},
								ui.Spacer:new { },
								ui.MenuItem:new
								{
									Text = "_Quit",
									Shortcut = "Ctrl+Q",
									onClick = function(self)
										self:getById("window-main"):onHide()
									end
								}
							}
						}
					}
				},
				ui.Group:new
				{
					Legend = "Compile and save to bytecode",
					Width = "fill",
					Orientation = "vertical",
					Children =
					{
						ui.Group:new
						{
							Children =
							{
								ui.Text:new
								{
									Text = "_Lua Source:",
									Class = "caption",
									Width = "fill",
									KeyCode = true,
								},
								Input:new
								{
									Id = "text-filename",
									Style = "text-align: right",
									onEnter = function(self)
										Input.onEnter(self)
										local text = self:getText()
										local notexist = io.open(text, "rb") == nil
										local app = self.Application
										app:getById("button-run"):setValue("Disabled", notexist)
										app:getById("button-compile"):setValue("Disabled", notexist)
									end,
								},
								FileButton:new
								{
									KeyCode = "l",
									doRequest = function(self)
										local app = self.Application
										local filefield = app:getById("text-filename")
										local path, part = splitpath(filefield:getText())
										local status, path, select = app:requestFile
										{
											Title = "Select Lua source...",
											Path = path,
											Location = part
										}
										if status == "selected" and select[1] then
											local newfname = addpath(path, select[1])
											filefield:setValue("Enter", newfname)
											app:setStatus("Run the script to collect module dependencies.")
										else
											app:setStatus("File selection cancelled.")
										end
									end,
								}
							}
						},
						ui.Group:new
						{
							Children =
							{
								ui.CheckMark:new 
								{ 
									Id = "check-exclude-modules",
									Text = "Exclude modules from list:", 
									Selected = true, 
									Width = "auto" 
								},
								Input:new
								{
									Id = "text-exclude-filename",
									Text = "tek/lib/MODLIST",
									Style = "text-align: right",
								},
								FileButton:new
								{
									doRequest = function(self)
										local app = self.Application
										local filefield = app:getById("text-exclude-filename")
										local path, part = splitpath(filefield:getText())
										local status, path, select = app:requestFile
										{
											Title = "Select Module list file...",
											Path = path,
											Location = part
										}
										if status == "selected" and select[1] then
											local newfname = addpath(path, select[1])
											filefield:setValue("Enter", newfname)
										end
									end,
								},
								ui.Button:new 
								{ 
									Text = "Exclude now", 
									Width = "auto",
									onClick = function(self)
										self.Application:selectModules("exclude")
									end
								}
							}
						},
						ui.Group:new
						{
							Width = "fill",
							Children =
							{
								ui.Group:new
								{
									Width = "auto",
									Orientation = "vertical",
									Children =
									{
										ui.CheckMark:new
										{
											Id = "check-strip",
											Text = "Strip _Debug Information",
										},
										ui.Group:new
										{
											Columns = 2,
											Children =
											{
												ui.Text:new
												{
													Class = "caption",
													Text = "Save _Format",
													KeyCode = true,
												},
												ui.PopList:new
												{
													SelectedLine = 2,
													Id = "popitem-savemode",
													KeyCode = "f",
													ListObject = List:new
													{
														Items =
														{
															{ { "Lua Source" } },
															{ { "Bytecode" } },
															{ { "C Source" } },
															tekui_exe and { { "Executable" } } or nil
														}
													}
												}
											}
										}
									}
								},
								ui.Button:new
								{
									Id = "button-compile",
									Width = "fill",
									Height = "fill",
									Text = "Compile, Link, and _Save",
									Disabled = true,
									onClick = function(self)
										self.Application:compile()
									end
								}
							}
						}
					}
				},
				ui.Group:new
				{
					Legend = "Modules to link",
					Children =
					{
						ui.Group:new
						{
							Orientation = "vertical",
							Width = "fill",
							Children =
							{
								ui.Button:new
								{
									Id = "button-run",
									Text = "_Run",
									Disabled = true,
									onClick = function(self)
										self.Application:setStatus("Running sample...")
										self.Application:sample()
									end
								},
								ui.PopItem:new
								{
									Id = "button-add",
									Text = "Add _Module",
									Children =
									{
										ui.Group:new
										{
											Legend = "Add new Module",
											Children =
											{
												ui.Text:new
												{
													Class = "caption",
													Text = "Module Name:",
												},
												Input:new
												{
													MinWidth = 200,
													InitialFocus = true,
													onEnter = function(self)
														Input.onEnter(self)
														local text = self:getText()
														if text ~= "" then
															self.Application:addModule(text, "")
															self.Application:setStatus("Module %s added - please select a file name for it.", text)
														end
														self.Window:finishPopup()
													end
												}
											}
										}
									}
								},
								ui.Spacer:new { },
								ui.Button:new
								{
									Id = "button-all",
									Text = "Select _All",
									Disabled = true,
									onClick = function(self)
										self.Application:selectModules("all")
									end
								},
								ui.Button:new
								{
									Id = "button-none",
									Text = "Select _None",
									Disabled = true,
									onClick = function(self)
										self.Application:selectModules("none")
									end
								},
								ui.Button:new
								{
									Id = "button-invert",
									Text = "_Invert Sel.",
									Disabled = true,
									onClick = function(self)
										self.Application:selectModules("toggle")
									end
								},
								ui.Button:new
								{
									Id = "button-delete",
									Text = "Delete Sel.",
									Disabled = true,
									onClick = function(self)
										local app = self.Application
										local n = app:selectModules()
										local text = n == 1 and "one module" or
											("%d modules"):format(n)
										app:addCoroutine(function()
											if app:easyRequest("Delete Modules",
												"Are you sure that you want to\n" ..
													"delete " .. text .. " from the list?",
												"_Delete", "_Cancel") == 1 then
												local n = self.Application:deleteModules("selected")
												self.Application:setStatus("%d modules deleted from the list", n)
											end
										end)
									end
								}
							}
						},
						ui.ScrollGroup:new
						{
							VSliderMode = "auto",
							HSliderMode = "auto",
							Child = ui.Canvas:new
							{
								Id = "group-canvas",
								AutoPosition = true,
								AutoWidth = true,
								AutoHeight = true,
								Child = ui.Group:new
								{
									Columns = 3,
									Id = "group-modules",
									Orientation = "vertical"
								}
							}
						}
					}
				},
				ui.Text:new
				{
					Id = "text-status",
					Text = "Please select a Lua source.",
					KeepMinWidth = true
				}
			}
		}
	}
}:run()
