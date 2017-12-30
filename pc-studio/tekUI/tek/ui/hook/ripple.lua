
local ui = require "tek.ui"
local DrawHook = require "tek.ui.class.drawhook"

local assert = assert
local floor = math.floor
local max = math.max
local min = math.min
local type = type

local RippleHook = DrawHook.module("tek.ui.class.ripple", "tek.ui.class.drawhook")
RippleHook._VERSION = "RippleHook 5.1"

function RippleHook.new(class, self)
	self = self or { }
	self.Defs = { } -- offset, ratio1, ratio2, max1, max2
	self.Orientation = self.Orientation or false
	self.Params = { } -- n, m, x, y, dx, dy, dx2, dy2
	self.DrawDotFunc = false
	return DrawHook.new(class, self)
end

function RippleHook:connect(parent)
	assert(parent.Parent)
	return DrawHook.connect(self, parent)
end

function RippleHook:setup(app, win)
	local e = self.Parent
	local d = self.Defs
	local props = e.Properties
	
	d[1] = props["effect-padding"]
	d[2] = props["effect-ratio"]
	d[3] = props["effect-ratio2"]
	d[4] = props["effect-maxnum"]
	d[5] = props["effect-maxnum2"]
	d[6] = props["effect-color"] or "border-shine"
	d[7] = props["effect-color2"] or "border-shadow"
	
	local o = self.Orientation or props["effect-orientation"]
	if o == false or o == "inline" then
		o = e.Parent.Orientation == "horizontal"
	elseif o == "across" then
		o = e.Parent.Orientation == "vertical"
	else
		o = o ~= "vertical"
	end
	self.Orientation = o and "horizontal" or "vertical"
	if props["effect-kind"] == "slant" then
		d[1], d[2], d[3], d[4], d[5] =
			d[1] or 9, d[2] or 0x44, d[3] or 0xc0, d[4] or 6, d[5] or 1
		self.DrawDotFunc = RippleHook.drawSlant
	else
		d[1], d[2], d[3], d[4], d[5] =
			d[1] or 2, d[2] or 0x23, d[3] or 0xc0, d[4] or 6, d[5] or 3
		self.DrawDotFunc = RippleHook.drawDot
	end

	DrawHook.setup(self, app, win)
end

function RippleHook:layout(x0, y0, x1, y1)
	local d = self.Defs
	local p = self.Params
	local w = x1 - x0 + 1
	local h = y1 - y0 + 1
	local offs = d[1] + 2
	local pad = d[1] * 0x100
	if self.Orientation == "horizontal" then
		p[1] = min(floor((w * d[2] + pad) / (offs * 0x100)), d[4])
		p[2] = min(floor((h * d[3] + pad) / (offs * 0x100)), d[5])
		p[3] = x0 + floor((w - (p[1] - 1) * offs) / 2)
		p[4] = y0 + floor((h - (p[2] - 1) * offs) / 2)
		p[5] = offs
		p[6] = 0
		p[7] = 0
		p[8] = offs
	else
		p[1] = min(floor((h * d[2] + pad) / (offs * 0x100)), d[4])
		p[2] = min(floor((w * d[3] + pad) / (offs * 0x100)), d[5])
		p[3] = x0 + floor((w - (p[2] - 1) * offs) / 2)
		p[4] = y0 + floor((h - (p[1] - 1) * offs) / 2)
		p[5] = 0
		p[6] = offs
		p[7] = offs
		p[8] = 0
	end
end

function RippleHook:draw()
	local defs = self.Defs
	local d = self.Parent.Window.Drawable
	local p1 = defs[6]
	local p2 = defs[7]
	local p = self.Params
	local x0, y0 = p[3], p[4]
	local draw = self.DrawDotFunc
	for i = 1, p[1] do
		local x, y = x0, y0
		for j = 1, p[2] do
			draw(d, x, y, p1, p2)
			x = x + p[7]
			y = y + p[8]
		end
		x0 = x0 + p[5]
		y0 = y0 + p[6]
	end
end

function RippleHook.drawSlant(d, x, y, p1, p2)
	d:drawLine(x - 2, y, x, y - 2, p1)
	d:drawLine(x, y + 2, x + 2, y, p2)
end

function RippleHook.drawDot(d, x, y, p1, p2)
	d:drawLine(x - 1, y - 1, x, y - 1, p1)
	d:drawPoint(x - 1, y, p1)
	d:drawPoint(x, y, p2)
end

return RippleHook
