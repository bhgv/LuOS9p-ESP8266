
local ui = require "tek.ui".checkVersion(112)
local DrawHook = ui.require("drawhook", 3)
local Region = ui.loadLibrary("region", 9)
local unpack = unpack
local newregion = Region.new

local Border = DrawHook.module("tek.ui.class.border", "tek.ui.class.drawhook")
Border._VERSION = "Border 7.5"

function Border.new(class, self)
	self = self or { }
	self.Border = self.Border or false
	self.Rect = newregion()
	return DrawHook.new(class, self)
end

function Border:getBorder()
	return unpack(self.Border, 1, 4)
end

function Border:layout(x0, y0, x1, y1)
	if not x0 then
		x0, y0, x1, y1 = self.Parent:getRect()
	end
	self.Rect:setRect(x0, y0, x1, y1)
	return x0, y0, x1, y1
end

function Border:getRegion()
	local b1, b2, b3, b4 = self:getBorder()
	local x0, y0, x1, y1 = self:layout()
	local b = newregion(x0 - b1, y0 - b2, x1 + b3, y1 + b4)
	b:subRect(x0, y0, x1, y1)
	return b, x0, y0, x1, y1
end

return Border
