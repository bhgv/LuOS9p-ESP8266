-------------------------------------------------------------------------------
--
--	tek.ui.class.gauge
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
--		Gauge ${subclasses(Gauge)}
--
--		This class implements a gauge for the visualization of
--		numerical values.
--
--	OVERRIDES::
--		- Area:askMinMax()
--		- Element:cleanup()
--		- Area:draw()
--		- Area:hide()
--		- Area:layout()
--		- Class.new()
--		- Numeric:onSetValue()
--		- Element:setup()
--		- Area:show()
--
-------------------------------------------------------------------------------

local ui = require "tek.ui".checkVersion(112)
local Numeric = ui.require("numeric", 1)
local Region = ui.loadLibrary("region", 9)

local floor = math.floor
local max = math.max
local min = math.min

local Gauge = Numeric.module("tek.ui.class.gauge", "tek.ui.class.numeric")
Gauge._VERSION = "Gauge 17.5"

-------------------------------------------------------------------------------
-- Gauge:
-------------------------------------------------------------------------------

function Gauge.new(class, self)
	self = self or { }
	self.BGRegion = false
	self.Child = self.Child or ui.Frame:new { Class = "gauge-fill" }
	self.Mode = "inert"
	self.Orientation = self.Orientation or "horizontal"
	return Numeric.new(class, self)
end

-------------------------------------------------------------------------------
--	connect: overrides
-------------------------------------------------------------------------------

function Gauge:connect(parent)
	-- our parent is also our knob's parent
	-- (suggesting it that it rests in a Group):
	self.Child:connect(parent)
	return Numeric.connect(self, parent)
end

-------------------------------------------------------------------------------
--	decodeProperties: overrides
-------------------------------------------------------------------------------

function Gauge:decodeProperties(p)
	Numeric.decodeProperties(self, p)
	self.Child:decodeProperties(p)
end

-------------------------------------------------------------------------------
--	setup: overrides
-------------------------------------------------------------------------------

function Gauge:setup(app, window)
	if self.Orientation == "horizontal" then
		self.MaxWidth = "none"
		self.Height = "auto"
	else
		self.Width = "auto"
		self.MaxHeight = "none"
	end
	Numeric.setup(self, app, window)
	self.Child:setup(app, window)
end

-------------------------------------------------------------------------------
--	cleanup: overrides
-------------------------------------------------------------------------------

function Gauge:cleanup()
	self.Child:cleanup()
	Numeric.cleanup(self)
	self.BGRegion = false
end

-------------------------------------------------------------------------------
--	show: overrides
-------------------------------------------------------------------------------

function Gauge:show()
	Numeric.show(self)
	self.Child:show()
end

-------------------------------------------------------------------------------
--	hide: overrides
-------------------------------------------------------------------------------

function Gauge:hide()
	self.Child:hide()
	Numeric.hide(self)
end

-------------------------------------------------------------------------------
--	askMinMax: overrides
-------------------------------------------------------------------------------

function Gauge:askMinMax(m1, m2, m3, m4)
	local w, h = self.Child:askMinMax(0, 0, 0, 0)
	return Numeric.askMinMax(self, m1 + w, m2 + h, m3 + w, m4 + h)
end

-------------------------------------------------------------------------------
--	getKnobRect:
-------------------------------------------------------------------------------

function Gauge:getKnobRect()
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
		local r = self.Max - self.Min
		if self.Orientation == "horizontal" then
			local w = x1 - x0 - km1 + 1
			x1 = min(x1, x0 + floor((self.Value - self.Min) * w / r) + km1)
		else
			local h = y1 - y0 - km2 + 1
			y0 = max(y0, 
				y1 - floor((self.Value - self.Min) * h / r) - km2)
		end
		return x0 - m1, y0 - m2, x1 + m3, y1 + m4
	end
end

-------------------------------------------------------------------------------
--	updateBGRegion:
-------------------------------------------------------------------------------

function Gauge:updateBGRegion()
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

function Gauge:layout(r1, r2, r3, r4, markdamage)
	local res = Numeric.layout(self, r1, r2, r3, r4, markdamage)
	local x0, y0, x1, y1 = self:getKnobRect()
	local res2 = self.Child:layout(x0, y0, x1, y1, markdamage)
	if res or res2 then
		self:updateBGRegion()
		return true
	end
end

-------------------------------------------------------------------------------
--	damage: overrides
-------------------------------------------------------------------------------

function Gauge:damage(r1, r2, r3, r4)
	Numeric.damage(self, r1, r2, r3, r4)
	self.Child:damage(r1, r2, r3, r4)
end

-------------------------------------------------------------------------------
--	erase: overrides
-------------------------------------------------------------------------------

function Gauge:erase()
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

function Gauge:draw()
	local res = Numeric.draw(self)
	self.Child:draw()
	return res
end

-------------------------------------------------------------------------------
--	onSetValue: overrides
-------------------------------------------------------------------------------

function Gauge:onSetValue()
	Numeric.onSetValue(self)
	self:rethinkLayout(2)
end

return Gauge
