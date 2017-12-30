
local lfs = require "lfs"
local db = require "tek.lib.debug"
local ui = require "tek.ui".checkVersion(112)

local CheckMark = ui.require("checkmark", 9)
local Display = ui.require("display", 0)
local Group = ui.require("group", 31)
local TextEdit = ui.require("textedit", 15)
local MenuItem = ui.require("menuitem", 10)
local Spacer = ui.require("spacer", 2)
local Text = ui.require("text", 28)
local Window = ui.require("window", 33)
local Input = ui.require("input")

local assert = assert
local concat = table.concat
local insert = table.insert
local max = math.max
local min = math.min
local remove = table.remove
local tonumber = tonumber
local tostring = tostring
local type = type

local EditWindow = Window.module("tek.ui.class.editwindow", "tek.ui.class.window")
EditWindow._VERSION = "EditWindow 7.8"

local MSG_MOUSEBUTTON = ui.MSG_MOUSEBUTTON
local MSG_MOUSEMOVE = ui.MSG_MOUSEMOVE
local MSG_FOCUS = ui.MSG_FOCUS
local MSG_REQSELECTION = ui.MSG_REQSELECTION

local FileImage = ui.getStockImage("file")
local L = ui.getLocale("editwindow-class", "schulze-mueller.de")

-------------------------------------------------------------------------------
--	EditInput: handles load/save file dialogs, status bar, and focusing
-------------------------------------------------------------------------------

local EditInput = TextEdit:newClass { _NAME = "_editinput" }

function EditInput.new(class, self)
	self = self or { }
	self.FindText = self.FindText or false
	self.VisibleMargin = self.VisibleMargin or { 3, 2, 3, 2 }
	self.MouseButtonPressed = false
	self.LastClickTime = { 0, 0 }
	self.MouseMarkCursorX = false
	self.MouseMarkCursorY = false
	self.MultiLine = true
	self.Class = "editor"
-- 	self.LineSpacing = 10
	return TextEdit.new(class, self)
end

function EditInput:textRequest(text, enterfunc, windowtitle, caption, buttontext)
	text = text or ""
	local app = self.Application
	local editinput = self
	
	app:addCoroutine(function()
		local oldcursormode = self:setCursorMode("hidden")
		local result
		
		local textfield = Input:new {
			InitialFocus = true,
			Text = text,
			show = function(self, app, window)
				self:setValue("Text", "")
				Input.show(self, app, window)
			end,
			onEnter = function(self)
				text = self:getText()
				result = true
				self.Window:hide()
			end
		} 

		local ww, wh = self.Window:getWindowDimensions()
		-- assuming here that a popup's minimum size equals its real size
		local wx, wy
		ww, wh, wx, wy = self.Application:getRequestWindow(ww, wh)
		
		local window = Window:new { 
			Class = "app-dialog",
			Title = windowtitle or L.WINDOWTITLE_FIND,
			Modal = true,
			Left = wx or false,
			Top = wy or false,
			Orientation = "vertical",
			HideOnEscape = true,
			Children = { 
				Group:new { 
					Children = {
						Text:new { Text = caption or L.CAPTION_FIND_TEXT, Class = "caption", Width = "auto" },
						textfield,
					}
				},
				Group:new { 
					Children = {
						ui.Button:new { Text = buttontext or L.BUTTON_FIND, Width = "auto",
							onClick = function(self)
								text = textfield:getText() or ""
								result = true
								self.Window:hide()
							end,
						},
						ui.Button:new { Text = L.BUTTON_CANCEL, Width = "auto",
							onClick = function(self)
								self.Window:hide()
							end
						},
					}
				}
			}
		}
		app.connect(window)
		app:addMember(window)
		window:setValue("Status", "show")
		repeat
			app:suspend(window)
		until window.Status ~= "show"
		
		if result then
			if enterfunc then
				enterfunc(text)
			end		
		end
		
		self:setCursorMode(oldcursormode)
	end)
end


function EditInput:findRequest()
	self:textRequest(self.FindText, function(text)
		self.FindText = text
		self:clearMark()
		self:markWord(text)
		self:find(text)
	end, L.WINDOWTITLE_FIND, L.CAPTION_FIND_TEXT, L.BUTTON_FIND)
end


function EditInput:gotoLineRequest()
	self:textRequest(self.FindText, function(lnr)
		lnr = tonumber(lnr)
		lnr = min(max(1, lnr), self:getNumLines())
		self:setCursor(false, 1, lnr, 1)
	end, L.WINDOWTITLE_GOTO_LINE, L.CAPTION_GOTO_LINE, L.BUTTON_GOTO_LINE)
end


function EditInput:loadRequest()
	local app = self.Application
	app:addCoroutine(function()
		local oldcursormode = self:setCursorMode("hidden")
		local win = self.Window
		local path, file = self:getPathFileName()
		local status, path, entry = app:requestFile 
		{
			Path = path,
			Location = file,
			Title = win.Locale.OPEN_FILE,
			Width = 580,
		}
		if status == "selected" then
			local file = entry[1]
			if file then
				local fullname = path .. "/" .. file
				self.InputShared.RecentFilename = fullname
				self:loadText(fullname)
			end
		end
		self:setCursorMode(oldcursormode)
	end)
end

function EditInput:getPathFileName()
	local path, file = self.FileName:match("^(.*)/([^/]+)$")
	if not path then
		local recentfile = self.InputShared.RecentFilename
		if recentfile then
			path, file = recentfile:match("^(.*)/([^/]+)$")
		end
		if not path then
			path = lfs.currentdir()
		end
	end
	return path, file	
end

function EditInput:saveRequest()
	local res
	local oldcursormode = self:setCursorMode("hidden")
	local win = self.Window
	local path, file = self:getPathFileName()
	local status, path, entry = self.Application:requestFile 
	{
		Path = path,
		Location = file,
		Title = win.Locale.SAVE_FILE_AS,
		SelectText = win.Locale.BUTTON_SAVE,
		FocusElement = "location",
		Width = 580,
	}
	if status == "selected" then
		local file = entry[1]
		if file then
			local fullname = path .. "/" .. file
			self.InputShared.RecentFilename = fullname
			res = self:saveText(fullname)
		end
	end
	self:setCursorMode(oldcursormode)
	return res
end

function EditInput:save()
	if self.FileName and self.FileName ~= "" then
		if not self.Changed then
			db.warn("save not necessary (saving anyway)")
		end
		self:saveText(self.FileName)
		db.warn("saved: %s", self.FileName)
	else
		self:saveRequest()
	end
end

function EditInput:updateCursorStatus()
	self.Window.CursorField:setValue("Text", 
		self.Locale.LINE_COL:format(self.CursorY, self.VisualCursorX))
end

function EditInput:updateFileNameStatus()
	self.Window.FilenameField:setValue("Text", self.FileName)
end

function EditInput:updateChangedStatus()
	local img = self.Changed and FileImage or false
	self.Window.StatusField:setValue("Image", img)
end

function EditInput:onSetCursorX(cx)
	TextEdit.onSetCursorX(self, cx)
	self:updateCursorStatus()
end

function EditInput:onSetCursorY(cy)
	TextEdit.onSetCursorY(self, cy)
	self:updateCursorStatus()
end

function EditInput:onSetFileName(fname)
	TextEdit.onSetFileName(self, fname)
	self:updateFileNameStatus()
end
		
function EditInput:onSetChanged(changed)
	self:updateChangedStatus()
end

function EditInput:show()
	TextEdit.show(self)
	-- selection events are not received by passMsg():
	self.Window:addInputHandler(MSG_REQSELECTION, self, self.handleSelection)
	-- intercept wheel events:
	self.Window:addInputHandler(MSG_MOUSEBUTTON, self, self.handleMouseButton)
	self:updateCursorStatus()
end

function EditInput:hide()
	self.Window:remInputHandler(MSG_MOUSEBUTTON, self, self.handleMouseButton)
	self.Window:remInputHandler(MSG_REQSELECTION, self, self.handleSelection)
	TextEdit.hide(self)
end

function EditInput:onFocus()
	TextEdit.onFocus(self)
	self:setActive(self.Focus)
end

function EditInput:doMark(mx, my)
	local res = TextEdit.doMark(self, mx, my)
	self.Window.Drawable:setAttrs { HaveSelection = res }
	return res
end

function EditInput:mouseUnmark()
	self.MouseMarkCursorX = false
	self.MouseMarkCursorY = false
	self:doMark()
end

function EditInput:passMsg(msg)
	if msg[2] == MSG_FOCUS then
		local has_focus = msg[3] == 1
		self:setCursorMode(has_focus and "active" or "still")
	elseif msg[2] == MSG_MOUSEBUTTON then
		if msg[3] == 1 or msg[3] == 16 then -- left/middle down
			local over, x, y = self.Parent:getMouseOver(msg)
			if over then
				self:setActive(true) -- sets focus and editing
				if self.Mark then
					self:mouseUnmark()
				end
				if msg[3] == 1 then
					self.MouseButtonPressed = true
				end
				self:setCursorByXY(x, y)
				self.MouseMarkCursorX, self.MouseMarkCursorY = self:getCursor()
				if msg[3] == 1 then -- only left mousebutton, dblclick word:
					local ts, tu = Display.getTime()
					local ct = self.LastClickTime
					if self.Window:checkDblClickTime(ct[1], ct[2], ts, tu) then
						local w0, w1, cy = self:findCurrentWord()
						if w0 then
							self:setCursor(-1, w0, cy)
							self:doMark()
							self:setCursor(-1, w1, cy)
						end
					end
					ct[1], ct[2] = ts, tu
				end
-- 			else
-- 				self.MouseButtonPressed = false
-- 				self:setValue("Selected", false)
			end
		elseif msg[3] == 2 then
			self.MouseButtonPressed = false
		elseif msg[3] == 32 then -- middleup
			if self.Editing then
				self:pasteClip(self:getSelection(2))
			end
		end
	elseif msg[2] == MSG_MOUSEMOVE then
		if self.MouseButtonPressed then
			local over, x, y = self.Parent:getMouseOver(msg)
			self:setCursorByXY(x, y, 1)
			if over then
				local cx, cy = self:getCursor()
				local mx, my = self.MouseMarkCursorX, self.MouseMarkCursorY
				if not self.Mark and (cx ~= mx or cy ~= my) then
					self:doMark(mx, my)
				end
			end
		end
	end
	return TextEdit.passMsg(self, msg)
end

function EditInput:handleSelection(msg)
	local which = msg[3]
	local clip 
	if which == 1 then -- clipboard
		clip = self:getClipboard()
	elseif which == 2 then -- selection
		local mx0, my0, mx1, my1 = self:getMark()
		if mx0 then
			clip = self:toClip(mx0, my0, mx1, my1)
		end
	end
	if clip then
		msg:reply { UTF8Selection = concat(clip, "\n") }
		return false
	end
	return msg
end

function EditInput:handleMouseButton(msg)
	if msg[3] == 64 then -- wheelup
		local over = self.Parent:getMouseOver(msg)
		if over then
			self:setActive(true)
			self:cursorUp()
			self:mouseUnmark()
			return false
		end
		return msg
	elseif msg[3] == 128 then -- wheeldown
		local over = self.Parent:getMouseOver(msg)
		if over then
			self:setActive(true)
			self:cursorDown()
			self:mouseUnmark()
			return false
		end
	end
	return msg
end

function EditInput:setActive(activate)
	if activate then
		self:setValue("Focus", true)
		self.InputShared.ActiveInput = self
		self:setValue("FileName", self.FileName, true)
		self:setValue("Changed", self.Changed, true)
		self:setCursor(-1)
	end
end

function EditInput:checkChanges(must_close_all, finalizer)
	local app = self.Application
	local win = self.Window
	local nchanged = 0
	local L = win.Locale
	
	for i = 1, #win.EditInputs do
		if win.EditInputs[i].Changed then
			nchanged = nchanged + 1
		end
	end
	
	if nchanged == 0 then
		finalizer()
		return
	end

	if must_close_all and (nchanged > 1 or not self.Changed) then
		app:addCoroutine(function()
			app:easyRequest(L.EXIT_APPLICATION,
				L.STILL_UNSAVED_CHANGES,
				L.BUTTON_OK)
		end)
		return 
	end
			
	if self.Changed then
		app:addCoroutine(function()
			local res = app:easyRequest(L.CLOSE_WINDOW,
				L.ABOUT_TO_LOSE_CHANGES,
				L.BUTTON_SAVE, L.BUTTON_DISCARD, L.BUTTON_CANCEL)
			if res == 3 then
				return
			elseif res == 1 then
				if win:getEditInput():saveRequest() then
					nchanged = nchanged - 1
				end
			elseif res == 2 then
				nchanged = nchanged - 1
			end
			if not must_close_all or nchanged == 0 then
				finalizer()
			end
		end)
	else
		if must_close_all and nchanged > 0 then
			app:addCoroutine(function()
				app:easyRequest(L.EXIT_APPLICATION,
					L.STILL_UNSAVED_CHANGES,
					L.BUTTON_OK)
			end)
			return
		end
		finalizer()
	end
end

-------------------------------------------------------------------------------
--	EditWindow: places an editor with accompanying elements, menus and
--	shortcuts in a window
-------------------------------------------------------------------------------

function EditWindow.createNewInput(self, L, shared, input_table, initial)
	local fontname = self.FontName
	if self.FontSize then
		fontname = (fontname or "") .. ":" .. self.FontSize
	end
	
	local hardscroll = self.HardScroll
	local autowrap = false
	
	local input = EditInput:new {
		AutoIndent = true,
-- 		AutoPosition = true,
		InitialFocus = initial,
		InputShared = shared,
		FontName = fontname,
		Clipboard = self.Clipboard,
		DragScroll = self.DragScroll,
		HardScroll = self.HardScroll,
		BlinkCursor = self.BlinkCursor,
		CursorStyle = self.CursorStyle,
		Locale = L,
		AutoWrap = autowrap,
		ScrollGroup = false,
		Canvas = false,
		Style = "margin: 0",
		ReadOnly = false,
		setup = function(self, app, window)
			EditInput.setup(self, app, window)
			if hardscroll then
				local h = self.LineHeight
				local s = self.ScrollGroup.VSliderGroup.Slider
				s.Increment = h
				s.Step = h
			end
		end
	}
	local canvas = ui.Canvas:new {
		AutoWidth = autowrap,
		AutoPosition = true,
		Child = input,
-- 		Style = "margin: 20; border-width:20",
-- 		Height = 18*20,
	}
	
	local scrollgroup = ui.ScrollGroup:new {
		AcceptFocus = false,
		VSliderMode = "on",
		HSliderMode = "off",
		Child = canvas,
		
		-- enforce hard scroll jumps under all circumstances:
-- 		onSetCanvasTop = function(self, y)
-- 			if hardscroll then -- and y < self.VMax then
-- 				local h = input.LineHeight
-- 				y = y - y % h
-- 			end
-- -- 			db.warn("y=%s y/lh=%s", y, y/input.LineHeight)
-- 			ui.ScrollGroup.onSetCanvasTop(self, y)
-- 		end
	}
	
	input.ScrollGroup = scrollgroup
	input.Canvas = canvas
	
	insert(input_table, input)
	
	return input
end

function EditWindow:getEditInput(activate)
	assert(self.InputShared.ActiveInput)
	local input = self.InputShared.ActiveInput
	if activate then
		input:setActive(true)
	end
	return input
end


function EditWindow.new(class, self)

	self = self or { }
	local window = self -- for use as an upvalue

	self.WindowMenu = false	
	self.Orientation = "vertical"
	self.Running = true
	self.Clipboard = self.Clipboard or false
	
	self.Locale = L

	self.CursorField = Text:new
	{
		Style = "text-align: left; font:ui-small",
		Width = "auto",
		Text = self.Locale.LINE_COL,
		KeepMinWidth = true,
	}
	
	self.StatusField = ui.ImageWidget:new 
	{ 
		Style = "border-style: inset",
		Mode = "button",
		MinWidth = 20,
		ImageAspectX = 2,
		ImageAspectY = 3,
		Width = "auto",
		Height = "fill",
		Mode = "inert",
	}
	
	self.FilenameField = Text:new
	{
		Style = "text-align: left; font:ui-small",
		KeepMinWidth = true,
	}
		
	-- create instances:
	
	self.NumInputs = 1
		
	local shared = self.InputShared or { }
	
	self.InputShared = shared
	
	self.EditInputs = { }
	for i = 1, self.NumInputs do
		EditWindow.createNewInput(self, L, shared, self.EditInputs, i == 1)
	end
	
	if self.FileName then
		local fnames = type(self.FileName) == "table" and self.FileName
			or { self.FileName }
		for i = 1, min(#self.EditInputs, #fnames) do
			self.EditInputs[i]:loadText(fnames[i])
		end
	end

	local inputchildren = { }
	for i = 1, #self.EditInputs do
		insert(inputchildren, self.EditInputs[i].ScrollGroup)
		if i < #self.EditInputs then
			insert(inputchildren, ui.Handle:new { })
		end
	end
	
	self.EditGroup = ui.Group:new { Children = inputchildren }

	self.Children = 
	{
		Group:new
		{
			Class = "menubar",
			Children =
			{
				MenuItem:new
				{
					Text = L.MENU_FILE,
					Children =
					{
						MenuItem:new 
						{
							Text = L.MENU_VIEW_NEW,
							Shortcut = "Ctrl+n",
							onClick = function()
								local ni = ui.Handle:new { }
								self.EditGroup:addMember(ni)
								local ni = EditWindow.createNewInput(self, L, self.InputShared, self.EditInputs)
								self.EditGroup:addMember(ni.ScrollGroup)
								ni:setActive(true)
								self.NumInputs = self.NumInputs + 1
								assert(self.NumInputs == #self.EditInputs)
								self:buildWindowMenu()
							end
						},
						MenuItem:new 
						{
							Text = L.MENU_VIEW_CLOSE,
							Shortcut = "Ctrl+W",
							onClick = function()
								local editinput = window:getEditInput()
								editinput:checkChanges(false, function()
									if self.NumInputs > 1 then
										local e = editinput.ScrollGroup
										local e0 = e:getPrev()
										local e1 = e:getNext()
										if e0 and e0:instanceOf(ui.Handle) then
											self.EditGroup:remMember(e0)
										elseif e1 and e1:instanceOf(ui.Handle) then
											self.EditGroup:remMember(e1)
										end
										self.EditGroup:remMember(e)
										local found
										for i = 1, #self.EditInputs do
											if self.EditInputs[i] == editinput then
												remove(self.EditInputs, i)
												found = true
												break
											end
										end
										assert(found)
										if self.InputShared.ActiveInput == editinput then
											self.InputShared.ActiveInput = false
										end
										self.EditInputs[1]:setActive(true) -- !!? argh
										self.NumInputs = self.NumInputs - 1
										assert(self.NumInputs == #self.EditInputs)
										self:buildWindowMenu()
									else
										editinput:newText()
										editinput:setActive(true)
									end
								end)
							end
						},
						Spacer:new { },
						MenuItem:new
						{
							Text = L.MENU_FILE_OPEN,
							Shortcut = "Ctrl+o",
							onClick = function(self)
								local editinput = window:getEditInput()
								editinput:checkChanges(false, function()
									editinput:loadRequest()
								end)
							end
						},
						MenuItem:new
						{
							Text = L.MENU_FILE_SAVE_AS,
							Shortcut = "Ctrl+S",
							onClick = function(self)
								local editinput = window:getEditInput()
								self.Application:addCoroutine(function()
									editinput:saveRequest()
								end)
							end
						},
						MenuItem:new
						{
							Text = L.MENU_FILE_SAVE,
							Shortcut = "Ctrl+s",
							onClick = function(self)
								local editinput = window:getEditInput()
								self.Application:addCoroutine(function()
									editinput:save()
								end)
							end
						},
						MenuItem:new 
						{
							Text = L.MENU_FILE_CLOSE,
							Shortcut = "Ctrl+w",
							onClick = function(self)
								local editinput = window:getEditInput()
								editinput:checkChanges(false, function()
									editinput:newText()
								end)
							end
						},
						Spacer:new { },
						MenuItem:new
						{
							Text = L.MENU_FILE_QUIT,
							Shortcut = "Ctrl+q",
							onClick = function(self)
								window:getEditInput():checkChanges(true, function()
									window:quit()
								end)
							end
						}
					}
				},
				MenuItem:new
				{
					Text = L.MENU_EDIT,
					Children =
					{
						MenuItem:new
						{
							Text = L.MENU_EDIT_DELETE_LINE,
							Shortcut = "Ctrl+k",
							onClick = function(self)
								window:getEditInput():deleteLine()
								self.Application:setLastKey() -- unset, allow retrig
							end
						},
						Spacer:new { },
--						MenuItem:new
--						{
--							Text = L.MENU_EDIT_MARK_START,
--							Shortcut = "Ctrl+b",
--							onClick = function(self)
--								window:getEditInput("activate"):doMark()
--							end
--						},
						MenuItem:new
						{
							Text = L.MENU_EDIT_MARK_COPY,
							Shortcut = "Ctrl+c",
							onClick = function(self)
								local editinput = window:getEditInput("activate")
								local clip = editinput:copyMark()
								window.Drawable:setAttrs { HaveClipboard = true }
								window.Drawable:setSelection(concat(clip, "\n"), 2)
							end
						},
						MenuItem:new
						{
							Text = L.MENU_EDIT_MARK_CUT,
							Shortcut = "Ctrl+x",
							onClick = function(self)
								local clip = window:getEditInput("activate"):cutMark()
								window.Drawable:setAttrs { HaveClipboard = true }
								window.Drawable:setSelection(concat(clip, "\n"), 2)
							end
						},
						MenuItem:new
						{
							Text = L.MENU_EDIT_MARK_PASTE,
							Shortcut = "Ctrl+v",
							onClick = function(self)
								local editinput = window:getEditInput("activate")
								editinput:pasteClip(editinput:getSelection())
								self.Application:setLastKey() -- unset, allow retrig
							end
						},
						
						Spacer:new { },
						
						MenuItem:new
						{
							Text = L.MENU_INDENT_BLOCK,
							Shortcut = "Ctrl+i",
							onClick = function(self)
								window:getEditInput("activate"):indentBlock(1)
								self.Application:setLastKey() -- unset, allow retrig
							end
						},
						MenuItem:new
						{
							Text = L.MENU_UNINDENT_BLOCK,
							Shortcut = "Ctrl+I",
							onClick = function(self)
								window:getEditInput("activate"):indentBlock(-1)
								self.Application:setLastKey() -- unset, allow retrig
							end
						},
						
						Spacer:new { },
						
						MenuItem:new
						{
							Text = L.MENU_EDIT_DEL_WORD_NEXT,
							Shortcut = "Ctrl+Del",
							onClick = function(self)
								window:getEditInput("activate"):deleteNextWord()
								self.Application:setLastKey() -- unset, allow retrig
							end
						},
						MenuItem:new
						{
							Text = L.MENU_EDIT_DEL_WORD_PREV,
							Shortcut = "Ctrl+BckSpc",
							onClick = function(self)
								window:getEditInput("activate"):deletePrevWord()
								self.Application:setLastKey() -- unset, allow retrig
							end
						},
						
					}
				},
				MenuItem:new
				{
					Text = L.MENU_MOVE,
					Children =
					{
						MenuItem:new
						{
							Text = L.MENU_MOVE_SOF,
							Shortcut = "Ctrl+Pos1",
							onClick = function(self)
								window:getEditInput("activate"):cursorSOF()
							end
						},
						MenuItem:new
						{
							Text = L.MENU_MOVE_EOF,
							Shortcut = "Ctrl+End",
							onClick = function(self)
								window:getEditInput("activate"):cursorEOF()
							end
						},
						
						Spacer:new { },
						
						MenuItem:new
						{
							Text = L.MENU_MOVE_WORD_NEXT,
							Shortcut = "Ctrl+Right",
							onClick = function(self)
								window:getEditInput("activate"):wordNext()
								self.Application:setLastKey() -- unset, allow retrig
							end
						},
						MenuItem:new
						{
							Text = L.MENU_MOVE_WORD_PREV,
							Shortcut = "Ctrl+Left",
							onClick = function(self)
								window:getEditInput("activate"):wordPrev()
								self.Application:setLastKey() -- unset, allow retrig
							end
						},
						MenuItem:new
						{
							Text = L.MENU_MOVE_GOTO_LINE,
							Shortcut = "Ctrl+g",
							onClick = function(self)
								window:getEditInput():gotoLineRequest()
							end
						},
						
						Spacer:new { },
						
						MenuItem:new
						{
							Text = L.MENU_MOVE_FIND,
							Shortcut = "Ctrl+f",
							onClick = function(self)
								local editinput = window:getEditInput()
								editinput:findRequest()
							end
						},
						MenuItem:new
						{
							Text = L.MENU_MOVE_FINDNEXT,
							Shortcut = "F3",
							onClick = function(self)
								local editinput = window:getEditInput()
								local findtext = editinput.FindText
								if findtext and findtext ~= "" then
									editinput:setActive(true)
									editinput:find(findtext)
									self.Application:setLastKey() -- unset, allow retrig
								else
									editinput:findRequest()
								end
							end
						},
						Spacer:new { },
						MenuItem:new
						{
							Text = L.MENU_MOVE_BOOKMARKS_TOGGLE,
							Shortcut = "Alt+b",
							onClick = function(self)
								window:getEditInput("activate"):toggleBookmark()
							end
						},
						MenuItem:new
						{
							Text = L.MENU_MOVE_BOOKMARKS_CLEAR,
							onClick = function(self)
								window:getEditInput("activate"):clearBookmarks()
							end
						},
						
						MenuItem:new
						{
							Text = L.MENU_MOVE_BOOKMARKS_PREV,
							Shortcut = "Alt+Up",
							onClick = function(self)
								window:getEditInput("activate"):prevBookmark()
								self.Application:setLastKey() -- unset, allow retrig
							end
						},
						
						MenuItem:new
						{
							Text = L.MENU_MOVE_BOOKMARKS_NEXT,
							Shortcut = "Alt+Down",
							onClick = function(self)
								window:getEditInput("activate"):nextBookmark()
								self.Application:setLastKey() -- unset, allow retrig
							end
						},
						
					}
				},
-- 				MenuItem:new
-- 				{
-- 					Text = L.MENU_OPTIONS,
-- 					Children =
-- 					{
-- 						CheckMark:new 
-- 						{ 
-- 							Text = L.MENU_ACCEL_SCROLL,
-- 							Class = "menuitem",
-- 							Selected = self.DragScroll,
-- 							onSelect = function(self)
-- 								CheckMark.onSelect(self)
-- 								window:getEditInput().DragScroll = self.Selected and 3
-- 								window:getEditInput():setValue("Focus", true)
-- 							end
-- 						},
-- 						CheckMark:new 
-- 						{ 
-- 							Text = L.MENU_FULLSCREEN,
-- 							Class = "menuitem",
-- 							Selected = window.FullScreen,
-- 							onSelect = function(self)
-- 								CheckMark.onSelect(self)
-- 								window.FullScreen = self.Selected
-- 								window:hide()
-- 							end
-- 						}
-- 					}
-- 				},
						
			}
		},
		
		self.EditGroup,
		
		Group:new
		{
			Children =
			{
				window.CursorField,
				window.StatusField,
				window.FilenameField,
			}
		}
	}
	
	return Window.new(class, self)
end


function EditWindow:buildWindowMenu()
	if self.WindowMenu then
		self.Children[1]:remMember(self.WindowMenu)
		self.WindowMenu = false
	end
	if self.NumInputs > 1 then
		local children = { }
		for i = 1, self.NumInputs do
			insert(children, ui.MenuItem:new
			{
				Text = tostring(i),
				Shortcut = "Alt+"..i,
				onClick = function()
					self.EditInputs[i]:setActive(true)
				end
			})
		end
		self.WindowMenu = MenuItem:new
		{
			Text = L.MENU_WINDOW,
			Children = children
		}
		self.Children[1]:addMember(self.WindowMenu)
	end
end


function EditWindow:onHide()
	self:getEditInput():checkChanges(true, function()
		self:quit()
	end)
end


function EditWindow:quit()
	self.Running = false
	self.Application:quit()
end

function EditWindow:setup(app, win)
	Window.setup(self, app, win)
end

return EditWindow
