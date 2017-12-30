-------------------------------------------------------------------------------
--
--	tek.ui.class.input
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	OVERVIEW::
--		[[#ClassOverview]] :
--		[[#tek.class : Class]] /
--		[[#tek.class.object : Object]] /
--		[[#tek.ui.class.element : Element]] /
--		[[#tek.ui.class.area : Area]] /
--		[[#tek.ui.class.frame : Frame]] /
--		[[#tek.ui.class.widget : Widget]] /
--		[[#tek.ui.class.group : Group]] /
--		[[#tek.ui.class.scrollgroup : ScrollGroup]] /
--		Input ${subclasses(Input)}
--
--		Text input class
--
--	ATTRIBUTES::
--
--	STYLE PROPERTIES::
--
--	IMPLEMENTS::
--		- Input:getText()
--		- Input:onEnter()
--		- Input:onSetText()
--		- Input:onSetChanged()
--
--	OVERRIDES::
--		- Class.new()
--		- Widget:onDisable()
--		- Widget:onActivate()
--		- Widget:onFocus()
--		- Widget:checkFocus()
--		- Widget:activate()
--
-------------------------------------------------------------------------------

local db = require "tek.lib.debug"
local ui = require "tek.ui".checkVersion(112)
local Canvas = ui.require("canvas", 36)
local Display = ui.require("display", 0)
local TextEdit = ui.require("textedit", 17)
local ScrollGroup = ui.require("scrollgroup", 0)
local concat = table.concat
local tostring = tostring
local type = type

local Input = ScrollGroup.module("tek.ui.class.input", "tek.ui.class.scrollgroup")
Input._VERSION = "Input 4.8"

local MSG_MOUSEBUTTON = ui.MSG_MOUSEBUTTON
local MSG_MOUSEMOVE = ui.MSG_MOUSEMOVE
local MSG_FOCUS = ui.MSG_FOCUS
local MSG_REQSELECTION = ui.MSG_REQSELECTION
local MSG_KEYDOWN = ui.MSG_KEYDOWN

-------------------------------------------------------------------------------

local EditInput = TextEdit:newClass { _NAME = "_editinput" }

function EditInput.new(class, self)
	self = self or { }
	self.MouseButtonPressed = false
	self.LastClickTime = { 0, 0 }
	self.MouseMarkCursorX = false
	self.MouseMarkCursorY = false
	return TextEdit.new(class, self)
end

function EditInput:askMinMax(m1, m2, m3, m4)
	m1 = m1 + self.FWidth * 2
	m2 = m2 + self.LineHeight
	return TextEdit.askMinMax(self, m1, m2, m3, m4)
end

function EditInput:show()
	TextEdit.show(self)
	self.Window:addInputHandler(MSG_REQSELECTION, self, self.handleSelection)
end

function EditInput:hide()
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
		self:setCursorMode(has_focus and "active" or "hidden")
	elseif msg[2] == MSG_MOUSEBUTTON then
		if msg[3] == 1 or msg[3] == 16 then -- left/middle down
-- 			if self.Window.HoverElement ~= self then
-- 				self:setValue("Focus", false)
-- 			else
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
							self:setCursor(-1, 1)
							self:doMark()
							self:setCursor(-1, self:getLineLength() + 1)
						end
						ct[1], ct[2] = ts, tu
					end
				else
					self.MouseButtonPressed = false
					self:setValue("Selected", false) -- only unselect
					-- self:setActive(false) -- remove selection
				end
-- 			end
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
		clip = self.Application:obtainClipboard()
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

function EditInput:setActive(activate)
	if activate and not self.Disabled then
		self:setValue("Focus", true)
		self:setCursor(-1)
	else
		if self.Mark then
			self:doMark() -- unmark
		end
		self:setValue("Focus", false)
		self:setCursor(0)
	end
end

function EditInput:handleKeyboard(msg)
	if msg[2] == MSG_KEYDOWN then
		local code = msg[3]
		local qual = msg[6]
		msg = self.ScrollGroup:handleKeyboard(msg)
		if not msg then
			return false
		end
		if qual == 4 and code == 99 then -- CTRL-c
			local clip = self:copyMark()
			if clip then
				self.Window.Drawable:setAttrs { HaveClipboard = true }
				self.Window.Drawable:setSelection(concat(clip, "\n"), 2)
			end
			return false -- absorb
		elseif qual == 4 and code == 118 then -- CTRL-v
			self:pasteClip(self:getSelection())
			return false -- absorb
		elseif qual == 4 and code == 120 then -- CTRL-x
			local clip = self:cutMark()
			if clip then
				self.Window.Drawable:setAttrs { HaveClipboard = true }
				self.Window.Drawable:setSelection(concat(clip, "\n"), 2)
			end
			return false -- absorb
		elseif code == 9 and not self.MultiLine then
			return msg -- pass msg on, but not to our superclass
		elseif code == 13 and not self.MultiLine then
			self:setEditing(false)
			local p = self.ScrollGroup
			p:setValue("Text", self:getText())
			p:setValue("Enter", self:getText(), true)
			if p.EnterNext then
				local w = self.Window
				local ne = w:getNextElement(self)
				if ne then
					w:setFocusElement(ne)
				end
			end
			return false
		end
	end
	return TextEdit.handleKeyboard(self, msg)
end

function EditInput:onSetChanged(changed)
	self.ScrollGroup:setValue("Changed", changed)
end

function EditInput:setup(app, window)
	TextEdit.setup(self, app, window)
	if self.HardScroll and self.MultiLine then
		local h = self.LineHeight
		local s = self.ScrollGroup.VSliderGroup.Slider
		s.Increment = h
		s.Step = h
	end
end

-------------------------------------------------------------------------------
--	addClassNotifications: overrides
-------------------------------------------------------------------------------

function Input.addClassNotifications(proto)
	Input.addNotify(proto, "Enter", ui.NOTIFY_ALWAYS,
		{ ui.NOTIFY_SELF, "onEnter" })
	Input.addNotify(proto, "Text", ui.NOTIFY_ALWAYS,
		{ ui.NOTIFY_SELF, "onSetText" })
	Input.addNotify(proto, "Changed", ui.NOTIFY_ALWAYS,
		{ ui.NOTIFY_SELF, "onSetChanged" })
	return ScrollGroup.addClassNotifications(proto)
end

Input.ClassNotifications = Input.addClassNotifications { Notifications = { } }


function Input.new(class, self)
	self = self or { }
	
	self.Changed = false
	self.Enter = false
	self.EnterNext = self.EnterNext or false
	self.TabEnter = self.TabEnter or false
	self.Text = self.Text or "" -- needed?
--	self.InitialFocus = self.InitialFocus or false
	
	if not self.AutoPosition then
		self.AutoPosition = true
	end

	self.AcceptFocus = false
	self.HSliderMode = "off"
	self.VSliderMode = self.MultiLine and "auto" or "off"
	
	self.EditInput = EditInput:new {
		AutoPosition = true,
		BlinkCursor = true,
		CursorStyle = self.CursorStyle,
		Disabled = self.Disabled,
		HardScroll = true,
		InitialFocus = self.InitialFocus,
		KeyCode = self.KeyCode,
		LineSpacing = self.LineSpacing or 2,
		MaxLength = self.MaxLength,
		MinWidth = self.MinWidth,
		MultiLine = self.MultiLine or false,
		PasswordChar = self.PasswordChar,
		ReadOnly = false,
		ScrollGroup = self,
		Size = self.Size,
		Style = self.Style or "",
		UseFakeCanvasWidth = true, --not not self.MultiLine,
	}
	
	self.EditInput:newText(self.Text)
	
	self.Child = Canvas:new {
		AutoWidth = false,
		AutoPosition = true,
		Child = self.EditInput,
		Class = "inputfield",
		Height = self.Height or self.MultiLine and "fill" or "auto",
		KeepMinHeight = true,
		KeepMinWidth = true,
		Width = self.Width,
	}
	
	self.EditInput:addStyleClass(self.Class)
	self.Child:addStyleClass(self.Class)
	
	self.Style = "border:0; margin:0; padding:0"
	self.InitialFocus = false
	self.Width = self.Size and "auto" or self.Width
	self = ScrollGroup.new(class, self)
	return self
end

function Input:onSetChanged()
	self.EditInput:setValue("Changed", self.Changed)
end

function Input:addChar(utf8char)
	self.EditInput:addChar(utf8char)
end

function Input:backSpace()
	self.EditInput:backSpace()
end

function Input:delChar()
	self.EditInput:delChar()
end

function Input:getChar(cx)
	local e = self.EditInput
	local l = e:getLineText(1)
	cx = cx or e:getCursor()
	return l:sub(cx)
end

function Input:getCursor()
	local cx = self.EditInput:getCursor() 
	return cx
end

function Input:moveCursorLeft()
	local e = self.EditInput
	e:cursorLeft()
	local cx = e:getCursor()
	return cx
end

function Input:moveCursorRight()
	local e = self.EditInput
	e:cursorRight()
	local cx = e:getCursor()
	return cx
end

function Input:setCursor(cx)
	local e = self.EditInput
	if cx == "eol" then
		e:cursorEOL()
	else
		e:setCursor(false, cx, 1, 1)
	end
end

function Input:onEnter()
	self:setValue("Text", self.Enter)
	self:setCursor("eol")
end

function Input:getText(p0, p1)
	return self.EditInput:getText()
-- 	return self.EditInput:getLineText(1):sub(p0, p1)
end

function Input:onSetText()
	self.EditInput:newText(self.Text)
end

function Input:onDisable()
	ScrollGroup.onDisable(self)
	self.EditInput:setValue("Disabled", self.Disabled)
	self.EditInput:damageLine(1)
end

function Input:onActivate()
	self.EditInput:setValue("Focus", true)
	self.EditInput:setValue("Active", self.Active)
end

function Input:onFocus()
-- 	self.EditInput:setCursorMode(self.Focus and "active" or "hidden")
	self.EditInput:setValue("Focus", self.Focus)
end

function Input:checkFocus()
	return self.EditInput:checkFocus()
end

function Input:activate(mode)
	return self.EditInput:activate(mode)
end

function Input:handleKeyboard(msg)
	return msg
end

function Input:cursorSOF()
	return self.EditInput:cursorSOF()
end

function Input:cursorEOF()
	return self.EditInput:cursorEOF()
end

function Input:cursorSOL()
	return self.EditInput:cursorSOL()
end

function Input:cursorEOL()
	return self.EditInput:cursorEOL()
end

return Input
