
--
--	tek.ui.class.tunnel
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--

local ui = require "tek.ui".checkVersion(112)
local db = require "tek.lib.debug"
local Frame = ui.Frame

local floor = math.floor
local max = math.max
local min = math.min
local pi = math.pi
local sin = math.sin

local Tunnel = Frame.module("tek.ui.class.tunnel", "tek.ui.class.frame")
_VERSION = "Tunnel 5.5"

-------------------------------------------------------------------------------
--	Class implementation:
-------------------------------------------------------------------------------

function Tunnel:setNumSeg(val)
	self.numseg = val
end
function Tunnel:setSpeed(val)
	self.speed = val
end
function Tunnel:setViewZ(val)
	self.viewz = val
end

function Tunnel.new(class, self)
	self = self or { }
	self.EraseBG = false
	self.MinWidth = self.MinWidth or 128
	self.MinHeight = self.MinHeight or 128
	-- movement table:
	self.dx = {  }
	self.ndx = 32
	for i = 1, self.ndx do
		self.dx[i] = sin(i * pi * 2 / 32) * 5
	end
	-- current offs in movement table:
	self.cx = 1
	self.cy = 8
	self.numseg = 6
	self.speed = 8
	self.z = 0
	self.viewz = 0x50
	self.dist = 0x100
	self.size = { 320, 256 }
	return Frame.new(class, self)
end

function Tunnel:show(drawable)
	Frame.show(self, drawable)
	self.Window:addInputHandler(ui.MSG_INTERVAL, self, self.updateInterval)
end

function Tunnel:hide()
	self.Window:remInputHandler(ui.MSG_INTERVAL, self, self.updateInterval)
	Frame.hide(self)
end

function Tunnel:draw()
	if Frame.draw(self) then
		local r1, r2, r3, r4 = self:getRect()
		local d = self.Window.Drawable
		local p0, p1 = "dark", "bright"
	
		d:fillRect(r1, r2, r3, r4, p0)
		d:pushClipRect(r1, r2, r3, r4)
	
		local sx = floor((r1 + r3) / 2)
		local sy = floor((r2 + r4) / 2)
	
		local z = self.z + self.viewz
		local cx = self.cx
		local cy = self.cy
	
		for i = 1, self.numseg do
			local x = self.size[1] * self.viewz / z
			local y = self.size[2] * self.viewz / z
			local dx = self.dx[cx] * z / 256
			local dy = self.dx[cy] * z / 256
			local x0 = floor(sx - x + dx)
			local y0 = floor(sy - y + dy)
			local x1 = floor(sx + x + dx)
			local y1 = floor(sy + y + dy)
			d:drawRect(x0, y0, x1, y1, p1)
			z = z + self.dist
			cx = cx == self.ndx and 1 or cx + 1
			cy = cy == self.ndx and 1 or cy + 1
		end
		
		d:popClipRect()
		return true
	end
end

function Tunnel:updateInterval(msg)
	self.z = self.z - self.speed
	if self.z < 0 then
		self.z = self.z + self.dist
		self.cx = self.cx == self.ndx and 1 or self.cx + 1
		self.cy = self.cy == self.ndx and 1 or self.cy + 1
	end
	self:setFlags(ui.FL_REDRAW)
	return msg
end

return Tunnel
