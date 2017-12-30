
local ui = require "tek.ui".checkVersion(112)
local Border = require "tek.ui.class.border"
local assert = assert
local floor = math.floor
local max = math.max
local min = math.min
local tonumber = tonumber
local unpack = unpack or table.unpack

local DefaultBorder = Border.module("tek.ui.border.default", "tek.ui.class.border")
DefaultBorder._VERSION = "DefaultBorder 9.4"

function DefaultBorder.new(class, self)
	self = self or { }
	self.Border = self.Border or false
	self.Legend = self.Legend or false
	self.LegendFont = false
	self.LegendWidth = false
	self.LegendHeight = false
	return Border.new(class, self)
end

function DefaultBorder:setup(app, win)
	
	Border.setup(self, app, win)
	
	local e = self.Parent
	local b = self.Border
	local props = e.Properties

	local pclass = ""
	for i = 0, 4, 4 do
		local bs = props["border-style" .. pclass]
		local p1 = "border-shadow"
		local p2, p3, p4 = p1, p1, p1
		if bs == "outset" or bs == "groove" then
			p1 = "border-shine"
			p2 = p1
		elseif bs == "inset" or bs == "ridge" then
			p3 = "border-shine"
			p4 = p3
		end
		b[5 + i] = props["border-left-color" .. pclass] or p1
		b[6 + i] = props["border-top-color" .. pclass] or p2
		b[7 + i] = props["border-right-color" .. pclass] or p3
		b[8 + i] = props["border-bottom-color" .. pclass] or p4
		pclass = ":active"
	end
	
	b[13] = props["border-rim-color"] or "border-rim"
	b[14] = props["border-focus-color"] or "border-focus"
	b[15] = tonumber(props["border-rim-width"]) or 0
	b[16] = tonumber(props["border-focus-width"]) or 0
	b[17] = props["border-legend-color"] or "border-legend"

	-- p[14]...p[17]: temp rect
	-- p[18]...p[21]: temp border

	local l = self.Legend
	if l then
		local f = app.Display:openFont(props["border-legend-font"])
		self.LegendFont = f
		self.LegendWidth, self.LegendHeight = f:getTextSize(l)
	end

end

function DefaultBorder:cleanup()
	self.LegendFont = 
		self.Application.Display:closeFont(self.LegendFont)
	Border.cleanup(self)
end

function DefaultBorder:getBorder()
	local b = self.Border
	return b[1], b[2] + (self.LegendHeight or 0), b[3], b[4]
end

local function drawBorderRect(d, b, b1, b2, b3, b4, p1, p2, p3, p4)
	if b1 > 0 then
		d:fillRect(b[18] - b1, b[19], b[18] - 1, b[21], p1)
	end
	if b2 > 0 then
		d:fillRect(b[18] - b1, b[19] - b2, b[20] + b3, b[19] - 1, p2)
	end
	if b3 > 0 then
		d:fillRect(b[20] + 1, b[19], b[20] + b3, b[21], p3)
	end
	if b4 > 0 then
		d:fillRect(b[18] - b1, b[21] + 1, b[20] + b3, b[21] + b4, p4)
		b[21] = b[21] + b4
		b[25] = b[25] - b4
	end
	b[18] = b[18] - b1
	b[22] = b[22] - b1
	b[19] = b[19] - b2
	b[23] = b[23] - b2
	b[20] = b[20] + b3
	b[24] = b[24] - b3
-- 	b[21] = b[21] + b4
-- 	b[25] = b[25] - b4
end

function DefaultBorder:draw()
	local e = self.Parent
	local w = e.Window
	local d = w.Drawable
	local props = e.Properties
	local b = self.Border
	local rw = b[15]

	b[18], b[19], b[20], b[21] = self.Rect:get()
	
	b[22], b[23], b[24], b[25] = unpack(self.Border, 1, 4)
	local gb, gox, goy = e:getBGElement():getBG()
	
	local tw = self.LegendWidth
	if tw then
		local th = self.LegendHeight
		local y0 = b[19] - b[23]
		local x0 = b[18]
		local x1 = b[20]
		local tx0, tx1 = self:alignText(x0, x1, tw)
		d:setBGPen(gb, gox, goy)
		d:setFont(self.LegendFont)
		d:pushClipRect(x0, y0 - th, x1, y0 - 1)
		d:drawText(tx0, y0 - th, tx1, y0 - 1, self.Legend, b[17], gb)
		d:popClipRect()
	end
	
	-- thickness of outer borders:
	local t = rw + b[16]
	local d1 = max(b[22] - t, 0)
	local d2 = max(b[23] - t, 0)
	local d3 = max(b[24] - t, 0)
	local d4 = max(b[25] - t, 0)

	local i = e.Selected and 9 or 5
	local p1, p2, p3, p4 = unpack(b, i, i + 3)
	local bs = e.Selected and props["border-style:active"] or
		props["border-style"]
	if bs == "ridge" or bs == "groove" then
		local e1, e2, e3, e4 =
			floor(d1 / 2), floor(d2 / 2), floor(d3 / 2), floor(d4 / 2)
		drawBorderRect(d, b, e1, e2, e3, e4, p1, p2, p3, p4)
		drawBorderRect(d, b, d1 - e1, d2 - e2, d3 - e3, d4 - e4,
			p3, p4, p1, p2)
	else
		drawBorderRect(d, b, d1, d2, d3, d4, p1, p2, p3, p4)
	end

	-- rim:
	d:setBGPen(b[13])
	drawBorderRect(d, b,
		min(b[22], rw), min(b[23], rw), min(b[24], rw), min(b[25], rw))
	
	-- focus:	
	d:setBGPen(e.Focus and b[14] or gb, gox, goy)
	drawBorderRect(d, b, b[22], b[23], b[24], b[25])
end

function DefaultBorder:getRegion(br)
	local b = self.Border
	local x0, y0, x1, y1 = self:layout()
	br:setRect(x0 - b[1], y0 - b[2], x1 + b[3], y1 + b[4])
	br:subRect(x0, y0, x1, y1)
	local tw = self.LegendWidth
	if tw then
		local tx0, tx1 = self:alignText(x0, x1, tw)
		br:orRect(tx0, y0 - self.LegendHeight - b[2], tx1, y0 - 1)
	end
	return br
end

function DefaultBorder:alignText(x0, x1, tw)
	local w = x1 - x0 + 1
	local align = self.Parent.Properties["border-legend-align"]
	if not align or align == "center" then
		x0 = max(x0, x0 + floor((w - tw) / 2))
	elseif align == "right" then
		x0 = max(x0, x1 + 1 - tw)
	end
	return x0, min(x1, x0 + tw - 1)
end

return DefaultBorder
