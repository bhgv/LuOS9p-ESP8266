-------------------------------------------------------------------------------
--
--	tek.ui.class.slider
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
--		[[#tek.ui.class.numeric : Numeric]] /
--		Slider ${subclasses(Slider)}
--
--		This class implements a slider for adjusting a numerical value.
--
--	ATTRIBUTES::
--		- {{AutoCapture [ISG]}} (boolean)
--			If '''true''' and the slider is receiving the focus, it reacts
--			on keyboard shortcuts instantly; otherwise, it must be selected
--			first (and deselected afterwards). Default: '''true'''
--		- {{Child [IG]}} ([[#tek.ui.class.widget : Widget]])
--			A Widget object for being used as the slider's knob. By default,
--			a knob widget of the style class {{"knob"}} is created internally.
--		- {{Kind [IG]}} (string)
--			Kind of the slider:
--				- {{"scrollbar"}} - for scrollbars. Sets the additional
--				style class {{"knob-scrollbar"}}.
--				- {{"number"}} - for adjusting numbers. Sets the additional
--				style class {{"knob-number"}}.
--			Default: '''false''', the kind is unspecified.
--		- {{Orientation [IG]}} (string)
--			Orientation of the slider, which can be {{"horizontal"}} or
--			{{"vertical"}}. Default: {{"horizontal"}}
--		- {{Range [ISG]}} (number)
--			The size of the slider, i.e. the range that it represents.
--			Setting this value invokes the Slider:onSetRange() method.
--		- {{Step [ISG]}} (boolean or number)
--			If '''true''' or {{1}}, integer steps are enforced. If a number,
--			steps of that size are enforced. If '''false''', the default, the
--			slider knob moves continuously.
--
--	IMPLEMENTS::
--		- Slider:onSetRange() - Handler for the {{Range}} attribute
--
--	OVERRIDES:
--		- Object.addClassNotifications()
--		- Element:cleanup()
--		- Area:draw()
--		- Area:hide()
--		- Area:layout()
--		- Class.new()
--		- Widget:onFocus()
--		- Widget:onHold()
--		- Area:passMsg()
--		- Area:setState()
--		- Element:setup()
--		- Area:show()
--		- Numeric:onSetMax()
--		- Numeric:onSetValue()
--
-------------------------------------------------------------------------------

local ui = require "tek.ui".checkVersion(112)
local Area = ui.require("area", 55)
local Numeric = ui.require("numeric", 5)
local Region = ui.loadLibrary("region", 10)
local Widget = ui.require("widget", 25)

local floor = math.floor
local max = math.max
local min = math.min

local Slider = Numeric.module("tek.ui.class.slider", "tek.ui.class.numeric")
Slider._VERSION = "Slider 27.1"

local FL_REDRAW = ui.FL_REDRAW

-------------------------------------------------------------------------------
--	addClassNotifications: overrides
-------------------------------------------------------------------------------

function Slider.addClassNotifications(proto)
	Slider.addNotify(proto, "Range", Slider.NOTIFY_ALWAYS, 
		{ Slider.NOTIFY_SELF, "onSetRange" })
	return Numeric.addClassNotifications(proto)
end

Slider.ClassNotifications = 
	Slider.addClassNotifications { Notifications = { } }

-------------------------------------------------------------------------------
--	init: overrides
-------------------------------------------------------------------------------

function Slider.new(class, self)
	self = self or { }
	self.AutoCapture = self.AutoCapture == nil and true or self.AutoCapture
	self.AutoPosition = self.AutoPosition ~= nil and self.AutoPosition or false
	self.BGRegion = false
	self.Captured = false
	self.Child = self.Child or Widget:new {
		Flags = ui.FL_DONOTBLIT,
		Class = "knob knob-" .. (self.Kind or "normal")
	}
	self.ClickDirection = false
	self.HoldXY = { }
	self.Step = self.Step or false
	self.Kind = self.Kind or false
	self.Mode = "button"
	self.Move0 = false
	self.Orientation = self.Orientation or "horizontal"
	self.Pos0 = 0
	self.Range = self.Range or false
	self = Numeric.new(class, self)
	self.Range = max(self.Max, self.Range or self.Max)
	return self
end

-------------------------------------------------------------------------------
--	connect: overrides
-------------------------------------------------------------------------------

function Slider:connect(parent)
	-- our parent is also our knob's parent:
	self.Child:connect(parent)
	return Numeric.connect(self, parent)
end

-------------------------------------------------------------------------------
--	decodeProperties: overrides
-------------------------------------------------------------------------------

function Slider:decodeProperties(p)
	Numeric.decodeProperties(self, p)
	self.Child:decodeProperties(p)
end

-------------------------------------------------------------------------------
--	setup: overrides
-------------------------------------------------------------------------------

function Slider:setup(app, window)
	if self.Orientation == "horizontal" then
		self.MaxWidth = "none"
		self.Height = "fill"
	else
		self.Width = "fill"
		self.MaxHeight = "none"
	end
	Numeric.setup(self, app, window)
	self.Child:setup(app, window)
end

-------------------------------------------------------------------------------
--	cleanup: overrides
-------------------------------------------------------------------------------

function Slider:cleanup()
	self.Child:cleanup()
	Numeric.cleanup(self)
	self.BGRegion = false
end

-------------------------------------------------------------------------------
--	show: overrides
-------------------------------------------------------------------------------

function Slider:show()
	Numeric.show(self)
	self.Child:show()
end

-------------------------------------------------------------------------------
--	hide: overrides
-------------------------------------------------------------------------------

function Slider:hide()
	self:setCapture(false)
	self.Child:hide()
	Numeric.hide(self)
end

-------------------------------------------------------------------------------
--	askMinMax: overrides
-------------------------------------------------------------------------------

function Slider:askMinMax(m1, m2, m3, m4)
	local w, h = self.Child:askMinMax(0, 0, 0, 0)
	if self.Orientation == "horizontal" then
		w = w + w
	else
		h = h + h
	end
	return Numeric.askMinMax(self, m1 + w, m2 + h, m3 + w, m4 + h)
end

-------------------------------------------------------------------------------
--	getKnobValue()
-------------------------------------------------------------------------------

function Slider:getKnobValue(v)
	v = v or self.Value
	local i = self.Step
	if not i then
		return v
	end
	if i == true or v == self.Max then
		return floor(v)
	end
	return floor(v / i) * i
end

-------------------------------------------------------------------------------
--	getKnobRect: internal
-------------------------------------------------------------------------------

function Slider:getKnobRect()
	local r1, r2, r3, r4 = self:getRect()
	if r1 then
		local c = self.Child
		local p1, p2, p3, p4 = self:getPadding()
		local m1, m2, m3, m4 = c:getMargin()
		local km1, km2 = c:getMinMax()
		local x0 = r1 + p1 + m1
		local y0 = r2 + p2 + m2
		local x1 = r3 - p3 - m3
		local y1 = r4 - p4 - m4
		local r = self.Range - self.Min
		local v = self:getKnobValue()
		if r > 0 then
			if self.Orientation == "horizontal" then
				local w = x1 - x0 - km1 + 1
				x0 = max(x0, x0 + floor((v - self.Min) * w / r))
				x1 = min(x1, x0 + floor((self.Range - self.Max) * w / r) +
					km1)
			else
				local h = y1 - y0 - km2 + 1
				y0 = max(y0, y0 + floor((v - self.Min) * h / r))
				y1 = min(y1, y0 + floor((self.Range - self.Max) * h / r) +
					km2)
			end
		end
		return x0 - m1, y0 - m2, x1 + m3, y1 + m4
	end
end

-------------------------------------------------------------------------------
--	updateBGRegion:
-------------------------------------------------------------------------------

function Slider:updateBGRegion()
	local bg = (self.BGRegion or Region.new()):setRect(self:getRect())
	self.BGRegion = bg
	local c = self.Child
	local r1, r2, r3, r4 = c:getRect()
	local c1, c2, c3, c4 = c:getBorder()
	bg:subRect(r1 - c1, r2 - c2, r3 + c3, r4 + c4)
end

-------------------------------------------------------------------------------
--	layout: overrides
-------------------------------------------------------------------------------

function Slider:layout(r1, r2, r3, r4, markdamage)
	local res = Numeric.layout(self, r1, r2, r3, r4, markdamage)
	local x0, y0, x1, y1 = self:getKnobRect()
	if x0 then
		local res2 = self.Child:layout(x0, y0, x1, y1, markdamage)
		if res or res2 then
			self:updateBGRegion()
			return true
		end	
	end
end

-------------------------------------------------------------------------------
--	damage: overrides
-------------------------------------------------------------------------------

function Slider:damage(r1, r2, r3, r4)
	Numeric.damage(self, r1, r2, r3, r4)
	self.Child:damage(r1, r2, r3, r4)
end

-------------------------------------------------------------------------------
--	erase: overrides
-------------------------------------------------------------------------------

function Slider:erase()
	local bg = self.BGRegion
	if bg then
		local d = self.Window.Drawable
		d:setBGPen(self:getBG())
		bg:forEach(d.fillRect, d)
	end
end

-------------------------------------------------------------------------------
--	draw: overrides
-------------------------------------------------------------------------------

function Slider:draw()
	local res = Numeric.draw(self)
	self.Child:draw()
	return res
end

-------------------------------------------------------------------------------
--	clickContainer:
-------------------------------------------------------------------------------

function Slider:clickContainer(xy)
	if not self.ClickDirection then
		local c = self.Child
		local b1, b2, b3, b4 = c:getBorder()
		local r1, r2, r3, r4 = c:getRect()
		if self.Orientation == "horizontal" then
			if xy[1] < r1 - b1 then
				self.ClickDirection = -1
			elseif xy[1] > r3 + b3 then
				self.ClickDirection = 1
			end
		else
			if xy[2] < r2 - b2 then
				self.ClickDirection = -1
			elseif xy[2] > r4 + b4 then
				self.ClickDirection = 1
			end
		end
	end
	if self.ClickDirection then
		self:increase(self.ClickDirection * self.Increment)
	end
end

-------------------------------------------------------------------------------
--	onHold: overrides
-------------------------------------------------------------------------------

function Slider:onHold()
	Numeric.onHold(self)
	if self.Hold then
		if not self.Move0 then
			if self.HoldXY[1] then
				self:clickContainer(self.HoldXY)
			end
		end
	else
		self.ClickDirection = false
	end
end

-------------------------------------------------------------------------------
--	startMove:
-------------------------------------------------------------------------------

function Slider:startMove(x, y)
	local b1, b2, b3, b4 = self.Child:getBorder()
	local r1, r2, r3, r4 = self.Child:getRect()
	if x >= r1 - b1 and x <= r3 + b3 and
		y >= r2 - b2 and y <= r4 + b4 then
	 	self.Move0 = { x, y }
	 	self.Pos0 = self.Value
		return self
	end
	return false
end

-------------------------------------------------------------------------------
--	doMove:
-------------------------------------------------------------------------------

function Slider:doMove(x, y)
	local r1, r2, r3, r4 = self:getRect()
	local c = self.Child
	local m1, m2, m3, m4 = c:getMargin()
	local newv
	local km1, km2 = c:getMinMax()
	if self.Orientation == "horizontal" then
		local w = r3 - r1 - m3 - m1 - km1 + 1
		newv = self.Pos0 +
			(x - self.Move0[1]) * (self.Range - self.Min) / max(w, 1)
	else
		local h = r4 - r2 - m4 - m2 - km2 + 1
		newv = self.Pos0 +
			(y - self.Move0[2]) * (self.Range - self.Min) / max(h, 1)
	end
	newv = self:getKnobValue(newv)
	self:setValue("Value", newv)
end

-------------------------------------------------------------------------------
--	updateSlider: internal
-------------------------------------------------------------------------------

function Slider:updateSlider()
	local knob = self.Child
	local x0, y0, x1, y1 = self:getKnobRect()
	if x0 and self.Window:relayout(knob, x0, y0, x1, y1) then
		self:updateBGRegion()
		if self:checkFlags(FL_REDRAW) then
			-- also redraw child if we're slated for redraw already:
			knob:setFlags(FL_REDRAW)
		end
		self:setFlags(FL_REDRAW)
	end	
end

-------------------------------------------------------------------------------
--	onSetValue: overrides
-------------------------------------------------------------------------------

function Slider:onSetValue()
	Numeric.onSetValue(self)
	self:updateSlider()
end

-------------------------------------------------------------------------------
--	onSetMax: overrides
-------------------------------------------------------------------------------

function Slider:onSetMax()
	Numeric.onSetMax(self)
	self:updateSlider()
end

-------------------------------------------------------------------------------
--	Slider:onSetRange(): This handler is invoked when the {{Range}}
--	attribute has changed.
-------------------------------------------------------------------------------

function Slider:onSetRange()
	self:updateSlider()
end

-------------------------------------------------------------------------------
--	passMsg: overrides
-------------------------------------------------------------------------------

function Slider:passMsg(msg)
	local mx, my = self:getMsgFields(msg, "mousexy")
	local win = self.Window
	if win.HoverElement == self then
		if msg[2] == ui.MSG_MOUSEBUTTON then
			if msg[3] == 64 then -- wheelup
				self:decrease()
				return false -- absorb
			elseif msg[3] == 128 then -- wheeldown
				self:increase()
				return false -- absorb
			end
		elseif msg[2] == ui.MSG_KEYDOWN then
			if msg[3] == 0xf023 then -- PgUp
				self:decrease(self.Range - (self.Max - self.Min + 1))
			elseif msg[3] == 0xf024 then -- PgDown
				self:increase(self.Range - (self.Max - self.Min + 1))
			end
		end
	end
	if msg[2] == ui.MSG_MOUSEBUTTON then
		if msg[3] == 1 then -- leftdown:
			if win.HoverElement == self and not self.Disabled then
				if self:startMove(mx, my) then
					win:setMovingElement(self)
				else
					-- otherwise the container was clicked:
					self.HoldXY[1] = mx
					self.HoldXY[2] = my
					self:clickContainer(self.HoldXY)
				end
			end
		elseif msg[3] == 2 then -- leftup:
			if win.MovingElement == self then
				win:setMovingElement()
			end
			self.Move0 = false
			self.ClickDirection = false
		end
	elseif msg[2] == ui.MSG_MOUSEMOVE then
		if win.MovingElement == self then
			self:doMove(mx, my)
			-- do not pass message to other elements while dragging:
			return false
		end
	end
	return Numeric.passMsg(self, msg)
end


-------------------------------------------------------------------------------
--	handleInput:
-------------------------------------------------------------------------------

function Slider:handleInput(msg)
	if msg[2] == ui.MSG_KEYDOWN then
		if msg[3] == 13 and self.Captured and not self.AutoCapture then
			self:setCapture(false)
			return false
		end
		local na = not self.AutoCapture
		local h = self.Orientation == "horizontal"
		if msg[3] == 0xf010 and (na or h) or
			msg[3] == 0xf012 and (na or not h) then
			self:decrease()
			return false
		elseif msg[3] == 0xf011 and (na or h) or
			msg[3] == 0xf013 and (na or not h) then
			self:increase()
			return false
		end
	end
	return msg
end

-------------------------------------------------------------------------------
--	onFocus: overrides
-------------------------------------------------------------------------------

function Slider:onFocus()
	Numeric.onFocus(self)
	local focused = self.Focus
	if self.AutoCapture then
		self:setCapture(focused)
	elseif not focused then
		self:setCapture(false)
	end
end

-------------------------------------------------------------------------------
--	onSelect: overrides
-------------------------------------------------------------------------------

function Slider:onSelect()
	Numeric.onSelect(self)
	local selected = self.Selected
	if selected and not self.AutoCapture then
		-- enter captured mode:
		self:setCapture(true)
	end
end

-------------------------------------------------------------------------------
--	setCapture: [internal] Sets the element's capture mode. If captured,
--	keyboard shortcuts can be used to adjust the slider's knob.
-------------------------------------------------------------------------------

function Slider:setCapture(onoff)
	if self:checkFocus() then
		if onoff and not self.Captured then
			self.Window:addInputHandler(ui.MSG_KEYDOWN, self, self.handleInput)
		elseif not onoff and self.Captured then
			self.Window:remInputHandler(ui.MSG_KEYDOWN, self, self.handleInput)
		end
		self.Captured = onoff
		self:setState()
	end
end

-------------------------------------------------------------------------------
--	setState: overrides
-------------------------------------------------------------------------------

function Slider:setState(bg, fg)
	if not bg and self.Captured then
		bg = self.Properties["background-color:active"]
	end
	Widget.setState(self, bg)
end

-------------------------------------------------------------------------------
--	reconfigure()
-------------------------------------------------------------------------------

function Slider:reconfigure()
	Numeric.reconfigure(self)
	self.Child:reconfigure()
end

return Slider
