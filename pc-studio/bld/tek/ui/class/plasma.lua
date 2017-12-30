
--
--	tek.ui.class.plasma
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--

local ui = require "tek.ui".checkVersion(112)
local Frame = ui.Frame

local cos = math.cos
local floor = math.floor
local max = math.max
local min = math.min
local pi = math.pi
local sin = math.sin
local unpack = unpack or table.unpack

local Plasma = Frame.module("tek.ui.class.plasma", "tek.ui.class.frame")
Plasma._VERSION = "Plasma 6.5"

-------------------------------------------------------------------------------
--	Class implementation:
-------------------------------------------------------------------------------

function Plasma:addgradient(sr, sg, sb, dr, dg, db, num)
	dr = (dr - sr) / (num - 1)
	dg = (dg - sg) / (num - 1)
	db = (db - sb) / (num - 1)
	local pal = self.Palette
	local pali = self.PalIndex
	for i = 0, num - 1 do
		pal[pali] = floor(sr) * 65536 + floor(sg) * 256 + floor(sb)
		pali = pali + 1
		sr = sr + dr
		sg = sg + dg
		sb = sb + db
	end
	self.PalIndex = pali
end

function Plasma.new(class, self)
	self = self or { }
	self.EraseBG = false
	self.Screen = { }
	self.Palette = { }
	self.PalIndex = 0
	self.SinTab = { }
	self.Params = { 0, 0, 0, 0, 0 } -- xp1, xp2, yp1, yp2, yp3
	self.DeltaX1 = 12
	self.DeltaX2 = 13
	self.DeltaY1 = 8
	self.DeltaY2 = 11
	self.DeltaY3 = 18
	self.SpeedX1 = 7
	self.SpeedX2 = -2
	self.SpeedY1 = 9
	self.SpeedY2 = -4
	self.SpeedY3 = 5
	self.PixWidth = self.PixWidth or 5
	self.PixHeight = self.PixHeight or 5
	self.W = self.W or 80
	self.H = self.H or 50
	
	Plasma.addgradient(self, 209,219,155, 79,33,57, 68)
	Plasma.addgradient(self, 79,33,57, 209,130,255, 60)
	local sintab = self.SinTab
	for i = 0, 1023 do
		sintab[i] = sin(i / 1024 * pi * 2)
	end
	self.MinWidth = self.W * self.PixWidth
	self.MinHeight = self.H * self.PixHeight
	self.MaxWidth = self.W * self.PixWidth
	self.MaxHeight = self.H * self.PixHeight
	
	return Frame.new(class, self)
end

function Plasma:show(drawable)
	Frame.show(self, drawable)
	self.Window:addInputHandler(ui.MSG_INTERVAL, self, self.updateInterval)
end

function Plasma:hide()
	self.Window:remInputHandler(ui.MSG_INTERVAL, self, self.updateInterval)
	Frame.hide(self)
end

function Plasma:draw()
	if Frame.draw(self) then
		local r1, r2, r3, r4 = self:getRect()
		local sintab = self.SinTab
		local palette = self.Palette
		local screen = self.Screen
		local pscale = #self.Palette / 10
		local p = self.Params
		local xp1, xp2, yp1, yp2, yp3 = unpack(p)
		local yc1, yc2, yc3 = yp1, yp2, yp3
		local c
	
		local dxc1 = floor(self.DeltaX1)
		local dxc2 = floor(self.DeltaX2)
		local dyc1 = floor(self.DeltaY1)
		local dyc2 = floor(self.DeltaY2)
		local dyc3 = floor(self.DeltaY3)
	
		for y = 0, self.H - 1 do
			local xc1, xc2 = xp1, xp2
			local ysin = sintab[yc1] + sintab[yc2] + sintab[yc3] + 5
			for x = y * self.W, (y + 1) * self.W - 1 do
				c = sintab[xc1] + sintab[xc2] + ysin
				screen[x] = palette[floor(c * pscale)]
				xc1 = (xc1 + dxc1) % 1024
				xc2 = (xc2 + dxc2) % 1024
			end
			yc1 = (yc1 + dyc1) % 1024
			yc2 = (yc2 + dyc2) % 1024
			yc3 = (yc3 + dyc3) % 1024
		end
	
		self.Window.Drawable:drawRGB(r1, r2, screen, self.W, self.H, 
			self.PixWidth, self.PixHeight)
		return true
	end
end

function Plasma:updateInterval(msg)
	local p = self.Params
	local xp1, xp2, yp1, yp2, yp3 = unpack(p)
	yp1 = (yp1 + self.SpeedY1) % 1024
	yp2 = (yp2 + self.SpeedY2) % 1024
	yp3 = (yp3 + self.SpeedY3) % 1024
	xp1 = (xp1 + self.SpeedX1) % 1024
	xp2 = (xp2 + self.SpeedX2) % 1024
	p[1], p[2], p[3], p[4], p[5] = xp1, xp2, yp1, yp2, yp3
	self:setFlags(ui.FL_REDRAW)
	return msg
end

return Plasma
