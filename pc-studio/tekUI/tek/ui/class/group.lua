-------------------------------------------------------------------------------
--
--	tek.ui.class.group
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
--		Group ${subclasses(Group)}
--
--		This class implements a container for child elements and
--		various layouting options.
--
--	ATTRIBUTES::
--		- {{Children [IG]}} (table)
--			A table of the object's children
--		- {{Columns [IG]}} (number)
--			Grid width, in number of elements [Default: 1, not a grid]
--		- {{FreeRegion [G]}} ([[#tek.lib.region : Region]])
--			Region inside the group that is not covered by child elements
--		- {{Layout [IG]}} (string or [[#tek.ui.class.layout : Layout]])
--			The name of a layouter class (or a Layouter object) used for
--			layouting the element's children. Default: {{"default"}}
--		- {{Orientation [IG]}} (string)
--			Orientation of the group; can be
--				- {{"horizontal"}} - The elements are layouted horizontally
--				- {{"vertical"}} - The elements are layouted vertically
--			Default: {{"horizontal"}}
--		- {{Rows [IG]}} (number)
--			Grid height, in number of elements. [Default: 1, not a grid]
--		- {{SameSize [IG]}} (boolean, {{"width"}}, {{"height"}})
--			'''true''' indicates that the same width and height should
--			be reserved for all elements in the group; the keywords
--			{{"width"}} and {{"height"}} specify that only the same width or
--			height should be reserved, respectively. Default: '''false'''
--
--	IMPLEMENTS::
--		- {{Group:addMember()}} - See Family:addMember()
--		- {{Group:remMember()}} - See Family:remMember()
--
--	OVERRIDES::
--		- Area:askMinMax()
--		- Element:cleanup()
--		- Area:damage()
--		- Element:decodeProperties()
--		- Area:draw()
--		- Area:erase()
--		- Area:getBGElement()
--		- Area:getByXY()
--		- Area:getChildren()
--		- Area:getGroup()
--		- Area:hide()
--		- Area:layout()
--		- Class.new()
--		- Area:passMsg()
--		- Area:punch()
--		- Element:setup()
--		- Area:show()
--
-------------------------------------------------------------------------------

local ui = require "tek.ui".checkVersion(112)
local Family = ui.require("family", 2)
local Widget = ui.require("widget", 29)
local Region = ui.loadLibrary("region", 10)
local assert = assert
local intersect = Region.intersect
local type = type

local Group = Widget.module("tek.ui.class.group", "tek.ui.class.widget")
Group._VERSION = "Group 35.2"

-------------------------------------------------------------------------------
--	constants:
-------------------------------------------------------------------------------

local FL_REDRAW = ui.FL_REDRAW
local FL_LAYOUT = ui.FL_LAYOUT
local FL_SHOW = ui.FL_SHOW
local FL_CHANGED = ui.FL_CHANGED
local FL_UPDATE = ui.FL_UPDATE
local FL_SETUP = ui.FL_SETUP
local FL_RECVINPUT = ui.FL_RECVINPUT
local FL_RECVMOUSEMOVE = ui.FL_RECVMOUSEMOVE
local FL_TRACKDAMAGE = ui.FL_TRACKDAMAGE

local MSGFLAGS = FL_LAYOUT + FL_SETUP + FL_SHOW + FL_RECVINPUT
local MSGFLAGS_MM = MSGFLAGS + FL_RECVMOUSEMOVE

local MSG_MOUSEMOVE = ui.MSG_MOUSEMOVE

-------------------------------------------------------------------------------
--	class implementation:
-------------------------------------------------------------------------------

function Group.new(class, self)
	self = self or { }
	self.Children = self.Children or { }
	self.Columns = self.Columns or false
	self.FreeRegion = false
	local layout = self.Layout or "default"
	if type(layout) == "string" then
		self.Layout = ui.loadClass("layout", layout):new { }
	end
	self.Orientation = self.Orientation or "horizontal"
	self.Rows = self.Rows or false
	self.SameSize = self.SameSize or false
	return Widget.new(class, self)
end

-------------------------------------------------------------------------------
--	forEachChild(method, ...)
-------------------------------------------------------------------------------

function Group:forEachChild(method, ...)
	local c = self:getChildren("init")
	for i = 1, #c do
		c[i][method](c[i], ...)
	end
end

-------------------------------------------------------------------------------
--	decodeProperties: overrides
-------------------------------------------------------------------------------

function Group:decodeProperties(p)
	Widget.decodeProperties(self, p)
	self:forEachChild("decodeProperties", p)
end

-------------------------------------------------------------------------------
--	setup: overrides
-------------------------------------------------------------------------------

function Group:setup(app, window)
	Widget.setup(self, app, window)
	self:setFlags(FL_CHANGED + FL_RECVINPUT + FL_RECVMOUSEMOVE)
	self:forEachChild("setup", app, window)
end

-------------------------------------------------------------------------------
--	cleanup: overrides
-------------------------------------------------------------------------------

function Group:cleanup()
	Widget.cleanup(self)
	self:checkClearFlags(0, FL_RECVINPUT + FL_RECVMOUSEMOVE)
	self:forEachChild("cleanup")
end

-------------------------------------------------------------------------------
--	show: overrides
-------------------------------------------------------------------------------

function Group:show()
	Widget.show(self)
	self:forEachChild("show")
end

-------------------------------------------------------------------------------
--	hide: overrides
-------------------------------------------------------------------------------

function Group:hide()
	Widget.hide(self)
	self.FreeRegion = false
	self:forEachChild("hide")
end

-------------------------------------------------------------------------------
--	connect: overrides
-------------------------------------------------------------------------------

function Group:connect(parent)
	assert(parent)
	local c = self:getChildren("init")
	if not c then
		return false
	end
	for i = 1, #c do
		if not c[i]:connect(self) then
			return false
		end
	end
	return Widget.connect(self, parent)
end

-------------------------------------------------------------------------------
--	addMember: add a child member (see Family:addMember())
-------------------------------------------------------------------------------

function Group:addMember(child, pos)
	child:connect(self)
	local app = self.Application
	if app then
		app:decodeProperties(child)
		child:setup(app, self.Window)
		if self:checkFlags(FL_SHOW) then
			child:show()
		end
	end
	if Family.addMember(self, child, pos) then
		self:rethinkLayout(2, true)
		return child
	end
	child:hide()
	child:cleanup()
end

-------------------------------------------------------------------------------
--	removed = remMember: remove a child member (see Family:remMember())
-------------------------------------------------------------------------------

function Group:remMember(child)
	local window = self.Window
	local show = child:checkFlags(FL_SHOW) and window
	if show and child == window.FocusElement then
		window:setFocusElement()
	end
	local found = Family.remMember(self, child)
	if child.Weight then
		local c = self.Children
		for i = 1, #c do
			c[i].Weight = false
		end
	end
	if show then
		child:hide()
	end
	if child:checkFlags(FL_SETUP) then
		child:cleanup()
	end
	self:rethinkLayout(1, true)
	return found
end

-------------------------------------------------------------------------------
--	damage: overrides
-------------------------------------------------------------------------------

local function orIntersect(d, x0, y0, x1, y1, r1, r2, r3, r4)
	x0, y0, x1, y1 = intersect(x0, y0, x1, y1, r1, r2, r3, r4)
	if x0 then
		d:orRect(x0, y0, x1, y1)
	end
end

function Group:damage(r1, r2, r3, r4)
	Widget.damage(self, r1, r2, r3, r4)
	if self:checkFlags(FL_LAYOUT) then
		local fr = self.FreeRegion
		if fr and fr:checkIntersect(r1, r2, r3, r4) then
			if self:checkFlags(FL_TRACKDAMAGE) then
				-- mark damage where it overlaps with freeregion:
				local dr = self.DamageRegion
				if not dr then
					dr = Region.new()
					self.DamageRegion = dr
				end
				fr:forEach(orIntersect, dr, r1, r2, r3, r4)
			end
			self:setFlags(FL_REDRAW)
		end
		local c = self.Children
		for i = 1, #c do
			c[i]:damage(r1, r2, r3, r4)
		end
	end
end

-------------------------------------------------------------------------------
--	erase: overrides
-------------------------------------------------------------------------------

function Group:erase()
	local d = self.Window.Drawable
	local f = self.FreeRegion
	local dr = self.DamageRegion
	d:setBGPen(self:getBG())
	if dr then
		-- repaint where damageregion and freeregion overlap:
		dr:andRegion(f)
		dr:forEach(d.fillRect, d)
	else
		-- repaint freeregion:
		f:forEach(d.fillRect, d)
	end
end

-------------------------------------------------------------------------------
--	draw: overrides
-------------------------------------------------------------------------------

function Group:draw()
	local res = Widget.draw(self)
	local c = self.Children
	for i = 1, #c do
		if c[i]:checkClearFlags(FL_UPDATE) and not c[i].Invisible then
			c[i]:draw()
		end
	end
	return res
end

-------------------------------------------------------------------------------
--	getByXY: overrides
-------------------------------------------------------------------------------

function Group:getByXY(x, y)
	local r1, r2, r3, r4 = self:getRect()
	if r1 and x >= r1 and x <= r3 and y >= r2 and y <= r4 then
		local c = self.Children
		for i = 1, #c do
			local ret = c[i]:getByXY(x, y)
			if ret then
				return ret
			end
		end
	end
	return false
end

-------------------------------------------------------------------------------
--	askMinMax: overrides
-------------------------------------------------------------------------------

function Group:askMinMax(m1, m2, m3, m4)
	m1, m2, m3, m4 = self.Layout:askMinMax(self, m1, m2, m3, m4)
	return Widget.askMinMax(self, m1, m2, m3, m4)
end

-------------------------------------------------------------------------------
--	layout: overrides; note that layouting takes place unconditionally here
-------------------------------------------------------------------------------

function Group:layout(r1, r2, r3, r4, markdamage)
	local res = Widget.layout(self, r1, r2, r3, r4, markdamage)
	-- layout contents, update freeregion:
	local fr = (self.FreeRegion or Region.new()):setRect(r1, r2, r3, r4)
	self.FreeRegion = fr
	self.Layout:layout(self, r1, r2, r3, r4, markdamage)
	fr:subRegion(self.BorderRegion)
	if res then
		if self.Properties["background-attachment"] == "fixed" then
			-- fully repaint groups with fixed texture when resized:
			self.DamageRegion = false
		end
		self:setFlags(FL_REDRAW)
	end
	return res
end

-------------------------------------------------------------------------------
--	punch: overrides
-------------------------------------------------------------------------------

function Group:punch(region)
	local m1, m2, m3, m4 = self:getMargin()
	local r1, r2, r3, r4 = self:getRect()
	region:subRect(r1 - m1, r2 - m2, r3 + m3, r4 + m4)
end

-------------------------------------------------------------------------------
--	passMsg: overrides
-------------------------------------------------------------------------------

function Group:passMsg(msg)
-- 	if msg[2] == MOUSEBUTTON and msg[3] == 1 then -- leftdown
-- 		if Widget.getByXY(self, msg[4], msg[5]) then
-- 			if not self:onActivateGroup() then
-- 				return false
-- 			end
-- 		end
-- 	end
	local flags = msg[2] == MSG_MOUSEMOVE and MSGFLAGS_MM or MSGFLAGS
	local c = self.Children
	for i = 1, #c do
		if c[i]:checkFlags(flags) then
			msg = c[i]:passMsg(msg)
			if not msg then
				return false
			end
		end
	end
	return msg
end

-------------------------------------------------------------------------------
--	getGroup: overrides
-------------------------------------------------------------------------------

function Group:getGroup(parent)
	if parent then
		return Widget.getGroup(self, parent)
	end
	return self
end

-------------------------------------------------------------------------------
--	getChildren: overrides
-------------------------------------------------------------------------------

function Group:getChildren()
	return self.Children
end

-------------------------------------------------------------------------------
--	getBGElement: overrides
-------------------------------------------------------------------------------

function Group:getBGElement()
	return self
end

-------------------------------------------------------------------------------
--	reconfigure()
-------------------------------------------------------------------------------

function Group:reconfigure()
	Widget.reconfigure(self)
	self:forEachChild("reconfigure")
end

return Group
