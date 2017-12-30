-------------------------------------------------------------------------------
--
--	tek.ui.class.handle
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
--		Handle ${subclasses(Handle)}
--
--		Implements a handle that can be dragged along the axis of the Group's
--		orientation. It reassigns Weights to its flanking elements.
--
--	OVERRIDES::
--		- Area:askMinMax()
--		- Area:checkFocus()
--		- Area:draw()
--		- Class.new()
--		- Area:passMsg()
--		- Area:show()
--
-------------------------------------------------------------------------------

local ui = require "tek.ui".checkVersion(112)
local Widget = ui.require("widget", 25)

local floor = math.floor
local insert = table.insert
local ipairs = ipairs
local max = math.max
local min = math.min

local Handle = Widget.module("tek.ui.class.handle", "tek.ui.class.widget")
Handle._VERSION = "Handle 7.2"

-------------------------------------------------------------------------------
-- Class implementation:
-------------------------------------------------------------------------------

function Handle.new(class, self)
	self = self or { }
	self.AutoPosition = false
	self.Mode = self.Mode or "button"
	self.Move0 = false
	self.MoveMinMax = { }
	self.Orientation = false
	self.SizeList = false
	return Widget.new(class, self)
end

-------------------------------------------------------------------------------
--	setup: overrides
-------------------------------------------------------------------------------

function Handle:setup(app, win)
	local o = self:getGroup().Orientation == "horizontal"
	self.Orientation = o
	if o then
		self.Width = "auto"
		self.MaxHeight = "none"
	else
		self.MaxWidth = "none"
		self.Height = "auto"
	end
	Widget.setup(self, app, win)
end

-------------------------------------------------------------------------------
--	startMove: internal
-------------------------------------------------------------------------------

function Handle:startMove(x, y)

	local g = self:getGroup()
	local i1, i3
	if self.Orientation then
		i1, i3 = 1, 3
		self.Move0 = x
	else
		i1, i3 = 2, 4
		self.Move0 = y
	end

	local free0, free1 = 0, 0 -- freedom
	-- local max0, max1 = 0, 0
	local nfuc = 0 -- free+unweighted children
	local tw, fw = 0, 0 -- totweight, weight per unweighted child

	for _, e in ipairs(g.Children) do
		local df = 0 -- delta free
		local mf = 0 -- max free
		local mm = { e.MinMax:get() }
		if mm[i3] > mm[i1] then
			local er = { e:getRect() }
			local emb = { e:getMargin() }
			if e.Weight then
				tw = tw + e.Weight
			else
				local s = er[i3] - er[i1] + 1
				if s < mm[i3] then
					nfuc = nfuc + 1
					fw = fw + 0x10000
				end
			end
			df = er[i3] - er[i1] + 1 + emb[i1] +
				emb[i3] - mm[i1]
			mf = mm[i3] - (er[i3] - er[i1] + 1 + emb[i1] + emb[i3])
		end

		free1 = free1 + df
		-- max1 = max1 + mf
		if e == self then
			free0 = free1
			-- max0 = max1
		end
	end
	free1 = free1 - free0
	-- max1 = max1 - max0
	-- free0 = min(max1, free0)
	-- free1 = min(max0, free1)

	if tw < 0x10000 then
		if fw == 0 then
			fw, tw = 0, 0x10000
		else
			fw, tw = floor((0x10000 - tw) * 0x100 / (fw / 0x100)), 0x10000
		end
	else
		fw, tw = 0, tw
	end

	self.SizeList = { { }, { } } -- left, right
	local li = self.SizeList[1]

	local n = 0
	local w = 0x10000
	for _, e in ipairs(g.Children) do
		local mm = { e:getMinMax() }
		local nw
		if mm[i3] > mm[i1] then
			if not e.Weight then
				n = n + 1
				if n == nfuc then
					nw = w -- rest
				else
					nw = fw -- weight of an unweighted child
				end
			else
				nw = e.Weight
			end
			w = w - nw
			insert(li, { element = e, weight = nw })
		end
		if e == self then
			li = self.SizeList[2] -- second list
		end
	end

	self.MoveMinMax[1] = -free0
	self.MoveMinMax[2] = free1

	return self
end

-------------------------------------------------------------------------------
--	doMove: internal
-------------------------------------------------------------------------------

function Handle:doMove(x, y)

	local g = self:getGroup()
	local xy = (self.Orientation and x or y) - 
		self.Move0

	if xy < self.MoveMinMax[1] then
		xy = self.MoveMinMax[1]
	elseif xy > self.MoveMinMax[2] then
		xy = self.MoveMinMax[2]
	end
	local tot = self.MoveMinMax[2] - self.MoveMinMax[1]
	local totw = xy * 0x10000 / tot

	local totw2 = totw

	if xy < 0 then
		-- left:
		for i = #self.SizeList[1], 1, -1 do
			local c = self.SizeList[1][i]
			local e = c.element
			local nw = max(c.weight + totw, 0)
			totw = totw + (c.weight - nw)
			e.Weight = nw
		end
		-- right:
		for i = 1, #self.SizeList[2] do
			local c = self.SizeList[2][i]
			local e = c.element
			local nw = min(c.weight - totw2, 0x10000)
			totw2 = totw2 - (c.weight - nw)
			e.Weight = nw
		end
	elseif xy > 0 then
		-- left:
		for i = #self.SizeList[1], 1, -1 do
			local c = self.SizeList[1][i]
			local e = c.element
			local nw = min(c.weight + totw, 0x10000)
			totw = totw - (nw - c.weight)
			e.Weight = nw
		end
		-- right:
		for i = 1, #self.SizeList[2] do
			local c = self.SizeList[2][i]
			local e = c.element
			local nw = max(c.weight - totw2, 0)
			totw2 = totw2 + (nw - c.weight)
			e.Weight = nw
		end
	end
	g:rethinkLayout(1, true)
end

-------------------------------------------------------------------------------
--	passMsg: overrides
-------------------------------------------------------------------------------

function Handle:passMsg(msg)
	local mx, my = self:getMsgFields(msg, "mousexy")
	if msg[2] == ui.MSG_MOUSEBUTTON then
		if msg[3] == 1 then -- leftdown:
			if self.Window.HoverElement == self and not self.Disabled then
				if self:startMove(mx, my) then
					self.Window:setMovingElement(self)
				end
			end
		elseif msg[3] == 2 then -- leftup:
 			if self.Window.MovingElement == self then
 				self.SizeList = false
				self.Window:setMovingElement()
			end
			self.Move0 = false
		end
	elseif msg[2] == ui.MSG_MOUSEMOVE then
		if self.Window.MovingElement == self then
			self:doMove(mx, my)
			-- do not pass message to other elements while dragging:
			return false
		end
	end
	return Widget.passMsg(self, msg)
end

-------------------------------------------------------------------------------
--	checkFocus: overrides
-------------------------------------------------------------------------------

function Handle:checkFocus()
	return false
end

return Handle
