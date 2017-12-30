#!/usr/bin/env lua
--
--	bin/demo.lua - tekUI demo
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--

local lfs
pcall(function() lfs = require "lfs" end)
if not lfs then
	print "Sorry, this demo needs the luafilesystem library."
	print "It should be still possible to run the demos individually."
	return
end

local List = require "tek.class.list"
local ui = require "tek.ui"
local db = require "tek.lib.debug"
local rdargs = require "tek.lib.args".read

--	In workbench mode, the demo windows become draggable and resizeable even if
--	we are running on a display without a window manager. (See the attribute
--	'RootWindow' below for disabling this facility for the main window.)
ui.Mode = "workbench"

local ARGTEMPLATE = "-f=FULLSCREEN/S,-w=WIDTH/N,-h=HEIGHT/N,--help=HELP/S"
local args = rdargs(ARGTEMPLATE, arg)
if not args or args.help then
	print(ARGTEMPLATE)
	return
end


local APP_ID = "tekui-demo"
local VENDOR = "schulze-mueller.de"
local L = ui.getLocale(APP_ID, VENDOR)

function lfs.readdir(path)
	local dir, iter = lfs.dir(path)
	return function()
		local e
		repeat
			e = dir(iter)
		until e ~= "." and e ~= ".."
		return e
	end
end

-- -- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--	Load demos and insert them to the application:
-- -- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

function loaddemos(app)

	local demogroup = app:getById("demo-group")

	local demos = { }

	for fname in lfs.readdir(ui.ProgDir) do
		if fname:match("^demo_.*%.lua$") then
			fname = ui.ProgDir .. fname
			db.info("Loading demo '%s' ...", fname)
			local success, res = pcall(dofile, fname)
			if success then
				local window = res.Window
				window:addNotify("Status", "show", { ui.NOTIFY_ID,
					window.Id .. "-button", "setValue", "Selected", true })
				window:addNotify("Status", "hide", { ui.NOTIFY_ID,
					window.Id .. "-button", "setValue", "Selected", false })
				table.insert(demos, res)
			else
				db.error("*** Error loading demo '%s'", fname)
				db.error(res)
			end
		end
	end

	table.sort(demos, function(a, b) return a.Name < b.Name end)

	for _, demo in ipairs(demos) do
		ui.Application.connect(demo.Window)
		app:addMember(demo.Window)
		local button = ui.Text:new
		{
			Id = demo.Window.Id .. "-button",
			Text = demo.Name,
			Mode = "toggle",
			Class = "button",
			UserData = demo.Window
		}
		button:addNotify("Selected", true, { ui.NOTIFY_ID, "info-text", "setValue", "Text", demo.Description })
		button:addNotify("Selected", true, { ui.NOTIFY_ID, demo.Window.Id, "setValue", "Status", "show" })
		button:addNotify("Selected", false, { ui.NOTIFY_ID, demo.Window.Id, "setValue", "Status", "hide" })
		ui.Application.connect(button)
		demogroup:addMember(button)
	end

end

-------------------------------------------------------------------------------
--	Application:
-------------------------------------------------------------------------------

app = ui.Application:new
{
	ProgramName = "tekUI Demo",
	Author = "Timm S. Müller",
	Copyright = "Copyright © 2008-2016, Schulze-Müller GbR",
	ApplicationId = APP_ID,
	Domain = VENDOR,
	Children =
	{
		ui.Window:new
		{
			Width = 400,
			Height = 500,
			MaxWidth = "none",
			MaxHeight = "none";
			MinWidth = 0,
			MinHeight = 0;
			HideOnEscape = true,
			SizeButton = true,

			UserData =
			{
				MemRefreshTickCount = 0,
				MemRefreshTickInit = 25,
				MinMem = false,
				MaxMem = false,
			},

			updateInterval = function(self, msg)
				local data = self.UserData
				data.MemRefreshTickCount = data.MemRefreshTickCount - 1
				if data.MemRefreshTickCount <= 0 then
					data.MemRefreshTickCount = data.MemRefreshTickInit
					local m = collectgarbage("count")
					data.MinMem = math.min(data.MinMem or m, m)
					data.MaxMem = math.max(data.MaxMem or m, m)
					local mem = self:getById("about-mem-used")
					if mem then
						mem:setValue("Text", ("%.0fk - min: %.0fk - max: %.0fk"):format(m,
							data.MinMem, data.MaxMem))
					end
					local gauge = self:getById("about-mem-gauge")
					if gauge then
						gauge:setValue("Min", data.MinMem)
						gauge:setValue("Max", data.MaxMem)
						gauge:setValue("Value", m)
					end
				end
				return msg
			end,

			show = function(self, drawable)
				ui.Window.show(self, drawable)
				self:addInputHandler(ui.MSG_INTERVAL, self, self.updateInterval)
			end,

			hide = function(self)
				self:remInputHandler(ui.MSG_INTERVAL, self, self.updateInterval)
				ui.Window.hide(self)
			end,

			Center = true,
			Orientation = "vertical",
			Id = "about-window",
			Status = "hide",
			Title = L.ABOUT_TEKUI,
			Children =
			{
				ui.Text:new { Text = L.ABOUT_TEKUI, Style = "font: ui-large" },
				ui.PageGroup:new
				{
					PageCaptions = { L.ABOUT_APPLICATION, L.ABOUT_LICENSE, L.ABOUT_SYSTEM },
					PageNumber = 3,
					Children =
					{
						ui.Group:new
						{
							Orientation = "vertical",
							Children =
							{
								ui.Group:new
								{
									Legend = L.ABOUT_APPLICATION_INFORMATION,
									Children =
									{
										ui.ListView:new
										{
											HSliderMode = "auto",
											VSliderMode = "auto",
											Headers = { L.PROPERTY, L.VALUE },
											Child = ui.Lister:new
											{
												SelectMode = "none",
												ListObject = List:new
												{
													Items =
													{
														{ { "ProgramName", "tekUI Demo" } },
														{ { "Version", "1.1" } },
														{ { "Author", "Timm S. Müller" } },
														{ { "Copyright", "© 2008-2014, Schulze-Müller GbR" } },
													}
												}
											}
										}
									}
								}
							}
						},
						ui.Group:new
						{
							Orientation = "vertical",
							Children =
							{
								ui.PageGroup:new
								{
									PageCaptions = { "tekUI", "Lua", L.DISCLAIMER },
									Children =
									{
										ui.ScrollGroup:new
										{
											Legend = L.TEKUI_LICENSE,
											VSliderMode = "auto",
											Child = ui.Canvas:new
											{
												AutoWidth = true,
												Child = ui.FloatText:new
												{
													Text = L.TEKUI_COPYRIGHT_TEXT
												}
											}
										},
										ui.ScrollGroup:new
										{
											Legend = L.LUA_LICENSE,
											VSliderMode = "auto",
											Child = ui.Canvas:new
											{
												AutoWidth = true,
												Child = ui.FloatText:new
												{
													Text = L.LUA_COPYRIGHT_TEXT
												}
											}
										},
										ui.ScrollGroup:new
										{
											Legend = L.DISCLAIMER,
											VSliderMode = "auto",
											Child = ui.Canvas:new
											{
												AutoWidth = true,
												Child = ui.FloatText:new
												{
													Text = L.DISCLAIMER_TEXT
												}
											}
										}
									}
								}
							}
						},
						ui.Group:new
						{
							Orientation = "vertical",
							Children =
							{
								ui.Group:new
								{
									Legend = L.SYSTEM_INFORMATION,
									Orientation = "vertical",
									Children =
									{
										ui.ScrollGroup:new
										{
											VSliderMode = "auto",
											Child = ui.Canvas:new
											{
												AutoWidth = true,
												Child = ui.FloatText:new
												{
													Text = L.INTERPRETER_VERSION:format(_VERSION) .. "\n" ..
															L.TEKUI_PACKAGE_VERSION:format(ui.VERSIONSTRING)
												}
											}
										},

										ui.Group:new
										{
											Legend = L.LUA_VIRTUAL_MACHINE,
											Columns = 2,
											Children =
											{
												ui.Text:new
												{
													Text = L.MEMORY_USAGE,
													Class = "caption",
													Style = "text-align: right; width: fill"
												},
												ui.Group:new
												{
													Orientation = "vertical",
													Children =
													{
														ui.Text:new { Id = "about-mem-used" },
														ui.Gauge:new { Id = "about-mem-gauge" },
													}
												},

												ui.Text:new
												{
													Text = L.GARBAGE_COLLECTOR,
													Class = "caption",
													Style = "text-align: right; width: fill"
												},
												ui.Group:new
												{
													Columns = 3,
													Children =
													{
														ui.Text:new
														{
															Text = L.GC_PAUSE,
															Class = "caption",
															Style = "text-align: right; width: fill"
														},
														ui.ScrollBar:new
														{
															Width = "free",
															Min = 1,
															Max = 300,
															Step = 1,
															Kind = "number",
															show = function(self, display, drawable)
																ui.ScrollBar.show(self, display, drawable)
																local ssm = collectgarbage("setpause", 0)
																collectgarbage("setpause", ssm)
																self:setValue("Value", ssm)
															end,
															onSetValue = function(self)
																ui.ScrollBar.onSetValue(self)
																local val = self.Value
																collectgarbage("setpause", val)
																self:getById("text-gcpause"):setValue("Text", ("  %d  "):format(val))
															end,
														},
														ui.Text:new
														{
															Id = "text-gcpause",
															Style = "width: fill",
															KeepMinWidth = true,
															Text = "0000",
														},
														ui.Text:new
														{
															Text = L.GC_STEPMUL,
															Class = "caption",
															Style = "text-align: right; width: fill"
														},
														ui.ScrollBar:new
														{
															Width = "free",
															Min = 1,
															Max = 300,
															Step = 1,
															Kind = "number",
															show = function(self, display, drawable)
																ui.ScrollBar.show(self, display, drawable)
																local ssm = collectgarbage("setstepmul", 0)
																collectgarbage("setstepmul", ssm)
																self:setValue("Value", ssm)
															end,
															onSetValue = function(self)
																ui.ScrollBar.onSetValue(self)
																local val = self.Value
																collectgarbage("setstepmul", val)
																self:getById("text-gcstepmul"):setValue("Text", ("  %d  "):format(val))
															end,
														},
														ui.Text:new
														{
															Id = "text-gcstepmul",
															Style = "width: fill",
															KeepMinWidth = true,
															Text = "0000",
														},
													}
												},


											}
										},

										ui.Group:new
										{
											Legend = L.DEBUGGING,
											Columns = 2,
											Children =
											{
												ui.Text:new
												{
													Text = L.DEBUG_LEVEL,
													Class = "caption",
													Style = "width: fill; text-align: right",
												},
												ui.ScrollBar:new
												{
													ArrowOrientation = "vertical",
													Min = 1,
													Max = 20,
													Value = db.level,
													Child = ui.Text:new
													{
														Id = "about-system-debuglevel",
														Class = "knob button",
														Text = tostring(db.level),
														Style = "font: ui-small;",
													},
													onSetValue = function(self, value)
														ui.ScrollBar.onSetValue(self, value)
														db.level = math.floor(self.Value)
														self:getById("about-system-debuglevel"):setValue("Text", db.level)
													end,
												},
												ui.Text:new
												{
													Text = L.DEBUG_OPTIONS,
													Class = "caption",
													Style = "width: fill; text-align: right",
												},
												ui.Group:new
												{
													Children =
													{
														ui.CheckMark:new
														{
															Text = L.SLOW_RENDERING,
															onSelect = function(self)
																ui.CheckMark.onSelect(self)
																self.Window.Drawable:setAttrs { Debug = self.Selected }
															end,
														},
														ui.Button:new
														{
															Text = L.DEBUG_CONSOLE,
															onClick = function(self)
																db.console()
															end
														}
													}
												}
											}
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
					Text = L.OKAY,
					Style = "width: fill",
					onClick = function(self)
						self.Window:setValue("Status", "hide")
					end,
				}
			}
		},
		ui.Window:new
		{
			Id = "window-main",
			HideOnEscape = true,
			MinWidth = 0, MinHeight = 0,
			Width = args.width,
			Height = args.height,
			MaxWidth = ui.HUGE, 
			MaxHeight = ui.HUGE,
			RootWindow = true, -- used in combination with workbench mode

			onHide = function(self)
				local app = self.Application
				app:addCoroutine(function()
					if app:easyRequest(false, L.CONFIRM_QUIT_APPLICATION,
						L.QUIT, L.CANCEL) == 1 then
						self.Application:quit()
					end
				end)
			end,

			Orientation = "vertical",
			onChangeStatus = function(self)
				ui.Window.onChangeStatus(self)
				if self.Status == "hide" then
					self.Application:setValue("Status", "quit")
				end
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
							Text = L.MENU_FILE,
							Children =
							{
								ui.MenuItem:new
								{
									Text = L.MENU_ABOUT,
									Shortcut = "Ctrl+b",
									onClick = function(self)
										self:getById("about-window"):setValue("Status", "show")
									end
								},
								ui.Spacer:new { },
								ui.MenuItem:new
								{
									Text = L.OPEN_ALL,
									Shortcut = "Ctrl+a",
									onClick = function(self)
										local group = self:getById("demo-group")
										for _, c in ipairs(group.Children) do
											c:setValue("Selected", true)
										end
									end
								},
								ui.MenuItem:new
								{
									Text = L.CLOSE_ALL,
									Shortcut = "Ctrl+n",
									onClick = function(self)
										local group = self:getById("demo-group")
										for _, c in ipairs(group.Children) do
											c:setValue("Selected", false)
										end
									end
								},
								ui.Spacer:new { },
								ui.MenuItem:new
								{
									Text = L.MENU_QUIT,
									Shortcut = "Ctrl+q",
									onClick = function(self)
										self:getById("window-main"):onHide()
									end,
								}
							}
						},
						ui.MenuItem:new
						{
							Text = L.MENU_SETTINGS,
							Children =
							{
								ui.MenuItem:new
								{
									Text = L.MENU_OPEN_STYLESHEET,
									onClick = function(self)
										local app = self.Application
										app:addCoroutine(function()
											local status, path, select = app:requestFile
											{
												Path = "tek/ui/style",
											}
											if status == "selected" then
												local fname = path .. "/" .. select[1]
												if fname then
													app.initStylesheets(app, fname)
													app:reconfigure()
												end
											end
										end)
									end
								}
							}
						}
					},
				},
				ui.Text:new
				{
					Text = L.TEKUI_DEMO,
					Style = "font: ui-xx-large/bi"
				},
				ui.Group:new
				{
					Children =
					{
						ui.Group:new
						{
							Weight = 0,
							Orientation = "vertical",
							Children =
							{
								ui.ScrollGroup:new
								{
									Legend = L.AVAILABLE_DEMOS,
									VSliderMode = "auto",
									Child = ui.Canvas:new
									{
										AutoPosition = true,
										KeepMinWidth = true,
										AutoWidth = true,
										AutoHeight = true,
										Child = ui.Group:new
										{
											Id = "demo-group",
											Orientation = "vertical",
										}
									}
								}
							}
						},
						ui.Handle:new { },
						ui.ScrollGroup:new
						{
							Weight = 0x10000,
							Legend = L.COMMENT,
							VSliderMode = "auto",
							Child = ui.Canvas:new
							{
								AutoWidth = true,
								Child = ui.FloatText:new
								{
									Style = "margin: 10",
									Id = "info-text",
									Text = L.DEMO_TEXT,
								}
							}
						}
					}
				}
			}
		}
	}
}

loaddemos(app)

-- -- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
--	run application:
-- -- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

app:run()
app:hide()
app:cleanup()
