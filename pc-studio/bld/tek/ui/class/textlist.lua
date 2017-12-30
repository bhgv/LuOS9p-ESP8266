
local ui = require "tek.ui".checkVersion(112)
local db = require "tek.lib.debug"
local ScrollGroup = ui.ScrollGroup
local TextEdit = ui.TextEdit
local Display = ui.require("display", 0)
local concat = table.concat
local max = math.max
local min = math.min
local tostring = tostring

local TextList = ScrollGroup.module("tek.ui.class.textlist", "tek.ui.class.scrollgroup")
TextList._VERSION = "TextList 2.6"

local MSG_MOUSEBUTTON = ui.MSG_MOUSEBUTTON
local MSG_MOUSEMOVE = ui.MSG_MOUSEMOVE
local MSG_FOCUS = ui.MSG_FOCUS
local MSG_REQSELECTION = ui.MSG_REQSELECTION
local MSG_KEYDOWN = ui.MSG_KEYDOWN



local EditInput = TextEdit:newClass { _NAME = "_textlist" }

function EditInput.new(class, self)
	self = self or { }
	self.MouseButtonPressed = false
	self.MouseMarkCursorX = false
	self.MouseMarkCursorY = false
	self.SelectMode = self.SelectMode or false -- false, "line", "block"
	return TextEdit.new(class, self)
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

function EditInput:cursorLeft()
	if self.SelectMode == "block" then
		TextEdit.cursorLeft(self)
	end
end

function EditInput:cursorRight()
	if self.SelectMode == "block" then
		TextEdit.cursorRight(self)
	end
end

function EditInput:cursorEOL()
	if self.SelectMode == "block" then
		TextEdit.cursorEOL(self)
	end
end

function EditInput:setup(app, win)
	TextEdit.setup(self, app, win)
	if self.HardScroll then
		local group = self.ScrollGroup
		local s = group.VSliderGroup and group.VSliderGroup.Slider
		if s then
			group.Child.VIncrement = self.LineHeight
			s.Increment = self.LineHeight
			s.Step = self.LineHeight
		end
	end
end

function EditInput:updateMark()
	TextEdit.updateMark(self)
	local mark = self.Mark
	local cx0, cy0 = mark[1], mark[2]
	local cx, cy = self.CursorX, self.CursorY
-- 	db.warn("mark: %s %s %s %s", cx0, cy0, cx, cy)
end

function EditInput:onSetCursorY()
	TextEdit.onSetCursorY(self)
	local cx, cy = self:getCursor()
-- 	db.warn("cursor: %s %s", cx, cy)
end

function EditInput:mouseUnmark()
	self.MouseMarkCursorX = false
	self.MouseMarkCursorY = false
	self:doMark()
end

function EditInput:passMsg(msg)
	local smode = self.SelectMode
	if smode then
		local slinemode = smode == "line" or smode == "lines"
		if msg[2] == MSG_FOCUS then
			local has_focus = msg[3] == 1
			self:setCursorMode(has_focus and "active" or "hidden")
		elseif msg[2] == MSG_MOUSEBUTTON then
			if msg[3] == 1 or msg[3] == 16 then -- left/middle down
				local over, x, y = self.Parent:getMouseOver(msg)
				if over then
					self:setActive(true) -- sets focus
					if self.Mark then
						self:mouseUnmark()
					end
					if msg[3] == 1 then
						self.MouseButtonPressed = true
					end
					if slinemode then
						x = 1
					end
					self:setCursorByXY(x, y)
-- 					if smode == "lines" then
-- 						self.MouseMarkCursorX, self.MouseMarkCursorY = self:getCursor()
-- 						if slinemode == "line" then
-- 							self.MouseMarkCursorX = 1
-- 						end
-- 					end
				else
					self.MouseButtonPressed = false
					self:setValue("Selected", false)
				end
			elseif msg[3] == 2 then
				self.MouseButtonPressed = false
			end
		elseif msg[2] == MSG_MOUSEMOVE then
			if self.MouseButtonPressed then
				local over, x, y = self.Parent:getMouseOver(msg)
				if slinemode then
					x = 1
				end
				self:setCursorByXY(x, y, 1)
				if over and (smode == "lines" or smode == "block") then
					local cx, cy = self:getCursor()
					if slinemode then
						cx = 1
					end
					local mx, my = self.MouseMarkCursorX, self.MouseMarkCursorY
					if cx ~= mx or cy ~= my then
						if not self.Mark then
							self:doMark(mx, my)
						end
						if self.Mark then
							self:updateMark()
						end
					end
				end
			end
		end
	end
	return TextEdit.passMsg(self, msg)
end

function EditInput:show()
	TextEdit.show(self)
	-- selection events are not received by passMsg():
	self.Window:addInputHandler(MSG_REQSELECTION, self, self.handleSelection)
-- 	-- intercept wheel events:
-- 	self.Window:addInputHandler(MSG_MOUSEBUTTON, self, self.handleMouseButton)
-- 	self:updateCursorStatus()
end

function EditInput:hide()
-- 	self.Window:remInputHandler(MSG_MOUSEBUTTON, self, self.handleMouseButton)
	self.Window:remInputHandler(MSG_REQSELECTION, self, self.handleSelection)
	TextEdit.hide(self)
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
		local utf8 = { }
		for i = 1, #clip do
			utf8[i] = tostring(clip[i])
		end
		msg:reply { UTF8Selection = concat(utf8, "\n") }
		return false
	end
	return msg
end

function EditInput:doMark(mx, my)
	local res = TextEdit.doMark(self, mx, my)
	self.Window.Drawable:setAttrs { HaveSelection = res }
	return res
end

-------------------------------------------------------------------------------
--	TextList class
-------------------------------------------------------------------------------

function TextList.new(class, self)
	self = self or { }
	if self.HardScroll == nil then
		self.HardScroll = true
	end
	if self.Latch == nil then
		self.Latch = true
	end
	self.CurrentLatch = self.Latch
	self.ListText = EditInput:new
	{
		MultiLine = true,
		UseFakeCanvasWidth = true,
		
		ReadOnly = self.ReadOnly,
		CursorStyle = self.CursorStyle,
		MarkMode = self.MarkMode,
		SelectMode = self.SelectMode,	-- false, "line", "lines", "block"
		BlinkCursor = self.BlinkCursor,
		
		Class = "textlist",
		HardScroll = self.HardScroll,
		LineSpacing = self.LineSpacing or 0,
		DragScroll = false,
		ScrollGroup = self,
		BGPens = self.BGPens,
		FGPens = self.FGPens,
		Data = self.Data,
	}
	self.ListCanvas = ui.Canvas:new
	{
		Class = "textlist",
		AutoPosition = true,
		Child = self.ListText,
		Style = self.Style -- or "",
		-- Style = "border-width: 2; margin: 2",
	}
	self.AcceptFocus = false
	self.VSliderMode = self.VSliderMode or "on"
	self.HSliderMode = self.HSliderMode or "off"
	self.Child = self.ListCanvas
	return ScrollGroup.new(class, self)
end

function TextList:getLatch()
	local top = self.VValue <= 0
	local bot = self.VValue == self.VMax
	local newlatch = false
	local curlatch = self.CurrentLatch
	if curlatch == "top" then 
		newlatch = top and "top" or bot and "bottom"
	elseif self.Latch then
		newlatch = bot and "bottom" or top and "top"
	end
	if newlatch ~= curlatch then
		self.CurrentLatch = newlatch
	end
	return newlatch
end

function TextList:addLine(text, lnr)
	local input = self.ListText
	local numl = input:getNumLines()
	lnr = max(lnr or numl + 1, 1)
	lnr = min(lnr, numl + 1)
	
	local cx, cy = input:getCursor()
	local latch = self:getLatch()
	input:suspendWindowUpdate()
	local data = input.Data
	
	if numl > 0 then
		if lnr > numl then
			input:setCursor(0, -1, numl)
			input:enter(0) -- do not follow
		else
			input:setCursor(0, 1, lnr)
			input:enter(0) -- do not follow
			input:setCursor(0, 1, lnr, 0) -- do not follow
		end
	end
	input:addChar(text)
	numl = input:getNumLines()
	if latch == "bottom" then
		input:setCursor(0, 1, numl, 1) -- follow
		input:followCursor()
	end
	
	if lnr <= cy then
		cy = min(cy + 1, numl)
	end
	
	input:setCursor(0, cx, cy, 0)
	input:releaseWindowUpdate()
	
	return input:getLineText(lnr)
end

function TextList:changeLine(lnr, text)
	local input = self.ListText
	local line = input:getLine(lnr)
	line[1]:set(text)
	input:changeLine(lnr)
	input:damageLine(lnr)
	input:updateWindow()
end

function TextList:getNumLines()
	return self.ListText:getNumLines()
end

function TextList:clear()
	self.CurrentLatch = self.Latch
	return self.ListText:newText()
end

function TextList:suspendUpdate()
	self.ListText:suspendWindowUpdate()
end

function TextList:releaseUpdate()
	self.ListText:releaseWindowUpdate()
end

function TextList:deleteLine(dy)
	self.ListText:deleteLine(dy)
end

function TextList:getLineText(lnr)
	return self.ListText:getLineText(lnr)
end

return TextList
