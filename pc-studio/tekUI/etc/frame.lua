-------------------------------------------------------------------------------
--
--	tek.ui.class.frame
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	OVERVIEW::
--		[[#ClassOverview]] :
--		[[#tek.class : Class]] /
--		[[#tek.class.object : Object]] /
--		[[#tek.ui.class.element : Element]] /
--		[[#tek.ui.class.area : Area]] /
--		Frame ${subclasses(Frame)}
--
--		This class implements an element's borders. The ''default'' border
--		class handles up to three sub borders:
--			* The ''main'' border is the innermost of the three sub borders.
--			It is used to render the primary border style, which can be
--			''inset'', ''outset'', ''ridge'', ''groove'', or
--			''solid''. This border has priority over the other two.
--			* The ''rim'' border separates the two other borders and
--			may give the composition a more contrastful look. This border
--			has the lowest priority.
--			* The ''focus'' border (in addition to the element's focus
--			highlighting style) can be used to visualize that the element is
--			currently receiving the input. This is the outermost of the three
--			sub borders. When the element is in unfocused state, this border
--			often appears in the same color as the surrounding group, making
--			it indistinguishable from the surrounding background.
--		Border classes do not have to implement all sub borders; these
--		properties are all handled by the ''default'' border class internally,
--		and more (and other) sub borders and properties may be defined and
--		implemented in the future (or in other border classes). As the Frame
--		class has no notion of sub borders, their respective widths are
--		subtracted from the Element's total border width, leaving only the
--		remaining width for the ''main'' border.
--
--	ATTRIBUTES::
--		- {{BorderRegion [G]}} ([[#tek.lib.region : Region]])
--			Region object holding the outline of the element's border
--		- {{Legend [IG]}} (string)
--			Border legend text
--
--	STYLE PROPERTIES::
--		''border-bottom-color'' || controls the ''default'' border class
--		''border-bottom-width'' || controls {{Frame.Border}}
--		''border-class'' || controls {{Frame.BorderClass}}
--		''border-color'' || controls the ''default'' border class
--		''border-focus-color'' || controls the ''default'' border class
--		''border-focus-width'' || controls the ''default'' border class
--		''border-left-color'' || controls the ''default'' border class
--		''border-left-width'' || controls {{Frame.Border}}
--		''border-legend-font'' || controls the ''default'' border class
--		''border-legend-align'' || controls the ''default'' border class
--		''border-right-color'' || controls the ''default'' border class
--		''border-right-width'' || controls {{Frame.Border}}
--		''border-rim-color'' || controls the ''default'' border class
--		''border-rim-width'' || controls the ''default'' border class
--		''border-style'' || controls the ''default'' border class
--		''border-top-color'' || controls the ''default'' border class
--		''border-top-width'' || controls {{Frame.Border}}
--		''border-width'' || controls {{Frame.Border}}
--
--	IMPLEMENTS::
--		- Frame:drawBorder() - Draws the element's border
--		- Frame:getBorder() - Queries the element's border
--
--	OVERRIDES::
--		- Element:cleanup()
--		- Area:damage()
--		- Area:draw()
--		- Area:getByXY()
--		- Area:getMargin()
--		- Area:layout()
--		- Class.new()
--		- Element:onSetStyle()
--		- Area:punch()
--		- Area:setState()
--		- Element:setup()
--
-------------------------------------------------------------------------------

local db = require "tek.lib.debug"
local ui = require "tek.ui".checkVersion(112)
local Area = ui.require("area", 46)
local Region = ui.loadLibrary("region", 10)
local tonumber = tonumber
local type = type

local Frame = Area.module("tek.ui.class.frame", "tek.ui.class.area")
Frame._VERSION = "Frame 24.2"

local FL_REDRAWBORDER = ui.FL_REDRAWBORDER
local FL_SETUP_LAYOUT = ui.FL_SETUP + ui.FL_LAYOUT

-------------------------------------------------------------------------------
--	new: overrides
-------------------------------------------------------------------------------

function Frame.new(class, self)
	self = self or { }
	self.BorderObject = false
	self.BorderRegion = false
	self.Legend = self.Legend or false
	self = Area.new(class, self)
	if self.Legend then
		self:addStyleClass("legend")
	end
	return self
end

-------------------------------------------------------------------------------
--	setup: overrides
-------------------------------------------------------------------------------

function Frame:setup(app, window)
	Area.setup(self, app, window)
	self:newBorderObject()
end

-------------------------------------------------------------------------------
--	newBorderObject: internal
-------------------------------------------------------------------------------

function Frame:newBorderObject()
	local props = self.Properties
	local b1 = tonumber(props["border-left-width"]) or 0
	local b2 = tonumber(props["border-top-width"]) or 0
	local b3 = tonumber(props["border-right-width"]) or 0
	local b4 = tonumber(props["border-bottom-width"]) or 0
	if b1 > 0 or b2 > 0 or b3 > 0 or b4 > 0 then
		self.BorderObject = ui.createHook("border",
			props["border-class"] or "default", self,
			{
				Border = { b1, b2, b3, b4 },
				Legend = self.Legend, 
				Style = self.Style 
			})
	else
		self.BorderObject = false
	end
end

-------------------------------------------------------------------------------
--	cleanup: overrides
-------------------------------------------------------------------------------

function Frame:cleanup()
	self.BorderRegion = false
	self.BorderObject = ui.destroyHook(self.BorderObject)
	Area.cleanup(self)
end

-------------------------------------------------------------------------------
--	b1, b2, b3, b4 = Frame:getBorder(): Returns an element's border widths in
--	the order left, top, right, bottom.
-------------------------------------------------------------------------------

function Frame:getBorder()
	if self.BorderObject then
		return self.BorderObject:getBorder()
	end
	return 0, 0, 0, 0
end

-------------------------------------------------------------------------------
--	damage: overrides
-------------------------------------------------------------------------------

function Frame:damage(r1, r2, r3, r4)
	Area.damage(self, r1, r2, r3, r4)
	if self.BorderRegion and
		self.BorderRegion:checkIntersect(r1, r2, r3, r4) then
		self:setFlags(FL_REDRAWBORDER)
	end
end

-------------------------------------------------------------------------------
--	layout: overrides. Additionally maintains a border region.
-------------------------------------------------------------------------------

function Frame:layout(r1, r2, r3, r4, markdamage)
	local changed, border_ok = Area.layout(self, r1, r2, r3, r4, markdamage)
	if changed and self:layoutBorder() and not border_ok and
		markdamage ~= false then
		self:setFlags(FL_REDRAWBORDER)
	end		
	return changed
end

-------------------------------------------------------------------------------
--	layoutBorder: internal
-------------------------------------------------------------------------------

function Frame:layoutBorder()
	local bo = self.BorderObject
	if bo then
		-- getRegion() implies layout(); also, reuse existing region:
		self.BorderRegion = bo:getRegion(self.BorderRegion or Region.new())
		return true
	end
end

-------------------------------------------------------------------------------
--	punch: overrides
-------------------------------------------------------------------------------

function Frame:punch(region)
	Area.punch(self, region)
	region:subRegion(self.BorderRegion)
end

-------------------------------------------------------------------------------
--	Frame:drawBorder(): Draws an element's border.
-------------------------------------------------------------------------------

function Frame:drawBorder()
	local b = self.BorderObject
	if b and self:checkFlags(ui.FL_SHOW) then
		b:draw(self.Window.Drawable)
	end
end

-------------------------------------------------------------------------------
--	draw: overrides
-------------------------------------------------------------------------------

function Frame:draw()
	local res = Area.draw(self)
	if self:checkClearFlags(FL_REDRAWBORDER) then
		self:drawBorder()
	end
	return res
end

-------------------------------------------------------------------------------
--	getByXY: overrides
-------------------------------------------------------------------------------

function Frame:getByXY(x, y)
	local r1, r2, r3, r4 = self:getRect()
	if r1 then
		local b1, b2, b3, b4 = self:getBorder()
		return x >= r1 - b1 and x <= r3 + b3 and y >= r2 - b2 and
			y <= r4 + b4 and self
	end
end

-------------------------------------------------------------------------------
--	getMargin: overrides
-------------------------------------------------------------------------------

function Frame:getMargin()
	local m1, m2, m3, m4 = Area.getMargin(self)
	local b1, b2, b3, b4 = self:getBorder()
	return m1 + b1, m2 + b2, m3 + b3, m4 + b4
end

-------------------------------------------------------------------------------
--	onSetStyle: overrides
-------------------------------------------------------------------------------

function Frame:onSetStyle()
	Area.onSetStyle(self)
	if self:checkFlags(FL_SETUP_LAYOUT) then
		self:newBorderObject()
		self:layoutBorder()
		self:setFlags(FL_REDRAWBORDER)
	end
end

-------------------------------------------------------------------------------
--	setState: overrides
-------------------------------------------------------------------------------

function Frame:setState(bg)
	Area.setState(self, bg)
	self:setFlags(FL_REDRAWBORDER)
end

-------------------------------------------------------------------------------
--	reconfigure: 
-------------------------------------------------------------------------------

function Frame:reconfigure()
	Area.reconfigure(self)
	self:newBorderObject()
	self.BorderRegion = false
end

return Frame
