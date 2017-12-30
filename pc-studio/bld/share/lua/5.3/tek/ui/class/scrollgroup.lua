-------------------------------------------------------------------------------
--
--	tek.ui.class.scrollgroup
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
--		ScrollGroup ${subclasses(ScrollGroup)}
--
--		This class implements a group containing a scrollable
--		container and accompanying elements such as horizontal and vertical
--		[[#tek.ui.class.scrollbar : ScrollBars]].
--
--	ATTRIBUTES::
--		- {{AcceptFocus [IG]}} (boolean)
--			This attribute is passed to the scrollbar elements inside the
--			scroll group. See [[#tek.ui.class.scrollbar : ScrollBar]]
--		- {{Child [IG]}} ([[#tek.ui.class.canvas : Canvas]])
--			Specifies the Canvas which encapsulates the scrollable
--			area and children.
--		- {{HSliderMode [IG]}} (string)
--			Specifies when the horizontal
--			[[#tek.ui.class.scrollbar : ScrollBar]] should be visible:
--				- {{"on"}} - The horizontal scrollbar is displayed
--				- {{"off"}} - The horizontal scrollbar is not displayed
--				- {{"auto"}} - The horizontal scrollbar is displayed when
--				the Lister is wider than the currently visible area.
--			Note: The use of the {{"auto"}} mode is currently (v8.0)
--			discouraged.
--		- {{VSliderMode [IG]}} (string)
--			Specifies when the vertical
--			[[#tek.ui.class.scrollbar : ScrollBar]] should be visible:
--				- {{"on"}} - The vertical scrollbar is displayed
--				- {{"off"}} - The vertical scrollbar is not displayed
--				- {{"auto"}} - The vertical scrollbar is displayed when
--				the Lister is taller than the currently visible area.
--			Note: The use of the {{"auto"}} mode is currently (v8.0)
--			discouraged.
--
--	OVERRIDES::
--		- Element:cleanup()
--		- Area:layout()
--		- Class.new()
--		- Element:setup()
--
-------------------------------------------------------------------------------

local ui = require "tek.ui".checkVersion(112)
local Group = ui.require("group", 31)
local Region = ui.loadLibrary("region", 10)
local ScrollBar = ui.require("scrollbar", 13)

local assert = assert
local floor = math.floor
local insert = table.insert
local intersect = Region.intersect
local max = math.max
local min = math.min
local remove = table.remove

local ScrollGroup = Group.module("tek.ui.class.scrollgroup", "tek.ui.class.group")
ScrollGroup._VERSION = "ScrollGroup 19.3"

local FL_DRAW = ui.FL_SETUP + ui.FL_SHOW + ui.FL_LAYOUT
local FL_DONOTBLIT = ui.FL_DONOTBLIT

-------------------------------------------------------------------------------
--	new:
-------------------------------------------------------------------------------

function ScrollGroup.new(class, self)
	self = self or { }
	if self.AcceptFocus == nil then
		self.AcceptFocus = true
	end
	self.BlitList = { }
	self.Child = self.Child or false
	self.HMax = -1
	self.HRange = -1
	self.HValue = -1
	self.HSliderEnabled = false
	self.HSliderGroup = self.HSliderGroup or false
	self.HSliderMode = self.HSliderMode or "off"
	self.HSliderNotify = { self, "onSetSliderLeft", ui.NOTIFY_VALUE }
	self.NotifyHeight = { self, "onSetCanvasHeight", ui.NOTIFY_VALUE }
	self.NotifyLeft = { self, "onSetCanvasLeft", ui.NOTIFY_VALUE }
	self.NotifyTop = { self, "onSetCanvasTop", ui.NOTIFY_VALUE }
	self.NotifyWidth = { self, "onSetCanvasWidth", ui.NOTIFY_VALUE }
	self.Orientation = "horizontal"
	self.Increment = self.Increment or 10
	self.HIncrement = self.HIncrement or self.Increment
	self.VIncrement = self.VIncrement or self.Increment
	self.VMax = -1
	self.VRange = -1
	self.VValue = -1
	self.VSliderEnabled = false
	self.VSliderGroup = self.VSliderGroup or false
	self.VSliderMode = self.VSliderMode or "off"
	self.VSliderNotify = { self, "onSetSliderTop", ui.NOTIFY_VALUE }
	
	local hslider, vslider

	if self.HSliderMode ~= "off" and not self.HSliderGroup then
		hslider = ScrollBar:new
		{
			Orientation = "horizontal",
			Min = 0,
			Increment = self.HIncrement,
			Step = 1,
			AcceptFocus = self.AcceptFocus
		}
		self.HSliderGroup = hslider
		self.HSliderEnabled = true
	end

	if self.VSliderMode ~= "off" and not self.VSliderGroup then
		vslider = ScrollBar:new
		{
			Orientation = "vertical",
			Min = 0,
			Increment = self.VIncrement,
			Step = 1,
			AcceptFocus = self.AcceptFocus
		}
		self.VSliderGroup = vslider
		self.VSliderEnabled = true
	end
	
	self.Children = { self.Child, vslider }
	self.HGroup = self
	if self.HSliderMode ~= "off" then
		self.Orientation = "vertical"
		self.HGroup = Group:new { Children = self.Children }
		self.Children = { self.HGroup, hslider }
	end
	
	return Group.new(class, self)
end

-------------------------------------------------------------------------------
--	askMinMax: overrides
-------------------------------------------------------------------------------

function ScrollGroup:askMinMax(m1, m2, m3, m4)
	m1, m2, m3, m4 = Group.askMinMax(self, m1, m2, m3, m4)
	local cb1, cb2, cb3, cb4 = self.Child:getMargin()
	local b1, b2, b3, b4 = self:getMargin()
	if self.HSliderMode == "auto" and self.Child:getAttr("MinWidth") == 0 then
		local n1 = self.Child:askMinMax(0, 0, 0, 0)
		self.Child.MinWidth = n1 - cb1 - cb3 - b1 - b3
	end
	if self.VSliderMode == "auto" and self.Child:getAttr("MinHeight") == 0 then
		self.Child.MinHeight = m2 - cb2 - cb4 - b2 - b4
	end
	return m1, m2, m3, m4
end

-------------------------------------------------------------------------------
--	setup: overrides
-------------------------------------------------------------------------------

function ScrollGroup:setup(app, window)
	Group.setup(self, app, window)
	local c = self.Child
	c:addNotify("CanvasLeft", ui.NOTIFY_ALWAYS, self.NotifyLeft)
	c:addNotify("CanvasTop", ui.NOTIFY_ALWAYS, self.NotifyTop)
	c:addNotify("CanvasWidth", ui.NOTIFY_ALWAYS, self.NotifyWidth)
	c:addNotify("CanvasHeight", ui.NOTIFY_ALWAYS, self.NotifyHeight)
	if self.HSliderGroup then
		self.HSliderGroup.Slider:addNotify("Value", ui.NOTIFY_ALWAYS,
			self.HSliderNotify)
	end
	if self.VSliderGroup then
		self.VSliderGroup.Slider:addNotify("Value", ui.NOTIFY_ALWAYS,
			self.VSliderNotify)
	end
end

-------------------------------------------------------------------------------
--	cleanup: overrides
-------------------------------------------------------------------------------

function ScrollGroup:cleanup()
	if self.VSliderGroup then
		self.VSliderGroup.Slider:remNotify("Value", ui.NOTIFY_ALWAYS,
			self.VSliderNotify)
	end
	if self.HSliderGroup then
		self.HSliderGroup.Slider:remNotify("Value", ui.NOTIFY_ALWAYS,
			self.HSliderNotify)
	end
	local c = self.Child
	c:remNotify("CanvasHeight", ui.NOTIFY_ALWAYS, self.NotifyHeight)
	c:remNotify("CanvasWidth", ui.NOTIFY_ALWAYS, self.NotifyWidth)
	c:remNotify("CanvasTop", ui.NOTIFY_ALWAYS, self.NotifyTop)
	c:remNotify("CanvasLeft", ui.NOTIFY_ALWAYS, self.NotifyLeft)
	Group.cleanup(self)
end

-------------------------------------------------------------------------------
--	enableHSlider:
-------------------------------------------------------------------------------

function ScrollGroup:enableHSlider(onoff)
	local enabled = self.HSliderEnabled
	if onoff and not enabled then
		self:addMember(self.HSliderGroup, 2)
		enabled = true
	elseif not onoff and enabled then
		self:remMember(self.HSliderGroup, 2)
		enabled = false
	end
	self.HSliderEnabled = enabled
	return enabled
end

-------------------------------------------------------------------------------
--	enableVSlider:
-------------------------------------------------------------------------------

function ScrollGroup:enableVSlider(onoff)
	local enabled = self.VSliderEnabled
	if onoff and not enabled then
		self.HGroup:addMember(self.VSliderGroup, 2)
		enabled = true
	elseif not onoff and enabled then
		self.HGroup:remMember(self.VSliderGroup, 2)
		enabled = false
	end
	self.VSliderEnabled = enabled
	return enabled
end

-------------------------------------------------------------------------------
--	onSetCanvasWidth:
-------------------------------------------------------------------------------

function ScrollGroup:onSetCanvasWidth(w)
	local c = self.Child
	local r1, _, r3 = c:getRect()
	if r1 then
		local sw = r3 - r1 + 1
		self.Child:setValue("CanvasWidth", w)
		self:enableHSlider(self.HSliderMode == "on"
			or self.HSliderMode == "auto" and (sw < w))
		local g = self.HSliderGroup
		if self.HRange ~= w then
			self.HRange = w
			if g then
				g.Slider:setValue("Range", w)
			end
		end
		local hmax = max(0, w - sw)
		if self.HMax ~= hmax then
			self.HMax = hmax
			if g then
				g.Slider:setValue("Max", hmax)
			end
		end
	end
end

-------------------------------------------------------------------------------
--	onSetCanvasHeight:
-------------------------------------------------------------------------------

function ScrollGroup:onSetCanvasHeight(h)
	local c = self.Child
	local _, r2, _, r4 = c:getRect()
	if r2 then
		local sh = r4 - r2 + 1
		self.Child:setValue("CanvasHeight", h)
		self:enableVSlider(self.VSliderMode == "on"
			or self.VSliderMode == "auto" and (sh < h))
		local g = self.VSliderGroup
		if self.VRange ~= h then
			self.VRange = h
			if g then
				g.Slider:setValue("Range", h)
			end
		end
		local vmax = max(0, h - sh)
		if self.VMax ~= vmax then
			self.VMax = vmax
			if g then
				g.Slider:setValue("Max", vmax)
			end
		end
	end
end

-------------------------------------------------------------------------------
--	onSetCanvasLeft:
-------------------------------------------------------------------------------

function ScrollGroup:onSetCanvasLeft(x)
	local c = self.Child
	local r1, _, r3 = c:getRect()
	if r1 then
		local ox = c.OldCanvasLeft
		ox = ox or c.CanvasLeft
		x = max(0, min(c.CanvasWidth - (r3 - r1 + 1), floor(x)))
		local dx = ox - x
		c:setValue("CanvasLeft", x, false)
		c.OldCanvasLeft = x
		c:setValue("CanvasLeft", x)
		if self.HSliderGroup then
			self.HSliderGroup.Slider:setValue("Value", x)
		end
		self.HValue = x
		if dx ~= 0 and not c:checkFlags(FL_DONOTBLIT) then
			insert(self.BlitList, { dx, 0 })
		end
	end
end

-------------------------------------------------------------------------------
--	onSetCanvasTop:
-------------------------------------------------------------------------------

function ScrollGroup:onSetCanvasTop(y)
	local c = self.Child
	local _, r2, _, r4 = c:getRect()
	if r2 then
		local oy = c.OldCanvasTop
		oy = oy or c.CanvasTop
		y = max(0, min(c.CanvasHeight - (r4 - r2 + 1), floor(y)))
		local dy = oy - y
		c:setValue("CanvasTop", y, false)
		c.OldCanvasTop = y
		c:setValue("CanvasTop", y)
		if self.VSliderGroup then
			self.VSliderGroup.Slider:setValue("Value", y)
		end
		self.VValue = y
		if dy ~= 0 and not c:checkFlags(FL_DONOTBLIT) then
			insert(self.BlitList, { 0, dy })
			self:setFlags(ui.FL_REDRAW)
		end
	end
end

-------------------------------------------------------------------------------
--	exposeArea:
-------------------------------------------------------------------------------

function ScrollGroup:exposeArea(r1, r2, r3, r4)
	self.Child:damage(r1, r2, r3, r4)
end

-------------------------------------------------------------------------------
--	blitRect:
-------------------------------------------------------------------------------

function ScrollGroup:blitRect(...)
	self.Window.Drawable:blitRect(...)
end

-------------------------------------------------------------------------------
--	layout: overrides
-------------------------------------------------------------------------------

function ScrollGroup:layout(r1, r2, r3, r4, markdamage)
	local res = Group.layout(self, r1, r2, r3, r4, markdamage)
	local c = self.Child
	self:onSetCanvasWidth(c.CanvasWidth)
	self:onSetCanvasHeight(c.CanvasHeight)
	self:onSetCanvasTop(c.CanvasTop)
	self:onSetCanvasLeft(c.CanvasLeft)
	return res
end

-------------------------------------------------------------------------------
--	draw: overrides
-------------------------------------------------------------------------------

function ScrollGroup:draw()

	-- handle scrolling:
	local cs = self.Window.CanvasStack
	insert(cs, self.Child)

	-- determine cumulative copyarea shift:
	local dx, dy = 0, 0
	while #self.BlitList > 0 do
		local c = remove(self.BlitList, 1)
		dx = dx + c[1]
		dy = dy + c[2]
	end
	
	local canvas = self.Child
	-- we have observed a child element not being ready for drawing:
	if (dx ~= 0 or dy ~= 0) and canvas:checkFlags(FL_DRAW) then
		
		-- determine own and parent canvas:
		local parent = cs[#cs - 1] or cs[1]

		-- calc total canvas shift for self and parent:
		local ax, ay = 0, 0
		local bx, by = 0, 0
		for i = 1, #cs - 1 do
			bx, by = ax, ay
			local r1, r2 = cs[i]:getRect()
			ax = ax + r1 - cs[i].CanvasLeft
			ay = ay + r2 - cs[i].CanvasTop
		end
		
		-- get intersection between self and parent:
		local a1, a2, a3, a4 = canvas:getRect()
		local b1, b2, b3, b4 = parent:getRect()
		a1, a2, a3, a4 = intersect(a1 + ax, a2 + ay, a3 + ax, a4 + ay,
			b1 + bx, b2 + by, b3 + bx, b4 + by)
		if a1 then

			-- intersect with top rect:
			a1, a2, a3, a4 = intersect(a1, a2, a3, a4, cs[1]:getRect())
			if a1 then

				local d = self.Window.Drawable

				-- make relative:
				local sx, sy = d:setShift()
				a1 = a1 - sx
				a2 = a2 - sy
				a3 = a3 - sx
				a4 = a4 - sy

				-- region that needs to get refreshed (relative):
				local dr = Region.new(a1, a2, a3, a4)

				local x0, y0, x1, y1 = canvas:getRect()
				x0, y0, x1, y1 = intersect(x0, y0, x1, y1,
					a1 + dx, a2 + dy, a3 + dx, a4 + dy)
				if x0 then

					dr:subRect(x0, y0, x1, y1)

					d:pushClipRect(a1, a2, a3, a4)

					-- copy area, collecting exposures from obscured regions:

					local t = { }

					self:blitRect(a1, a2, a3, a4, a1 + dx, a2 + dy, t)

					-- exposures resulting from obscured areas (make relative):
					for i = 1, #t, 4 do
						self:exposeArea(t[i] - sx, t[i+1] - sy,
							t[i+2] - sx, t[i+3] - sy)
					end

					-- exposures resulting from areas shifting into canvas:
					dr:forEach(self.exposeArea, self)

					d:popClipRect()

				else
					-- refresh all:
					self:exposeArea(a1, a2, a3, a4)
				end
			end
		end
	end

	-- refresh group contents (including damages caused by scrolling):

	local res = Group.draw(self)

	remove(cs)
	
	return res
end

-------------------------------------------------------------------------------
--	onSetSliderTop:
-------------------------------------------------------------------------------

function ScrollGroup:onSetSliderTop(val)
	self:onSetCanvasTop(val)
end

-------------------------------------------------------------------------------
--	onSetSliderLeft:
-------------------------------------------------------------------------------

function ScrollGroup:onSetSliderLeft(val)
	self:onSetCanvasLeft(val)
end

return ScrollGroup
