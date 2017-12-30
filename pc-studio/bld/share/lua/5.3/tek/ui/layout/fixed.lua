-------------------------------------------------------------------------------
--
--	tek.ui.layout.fixed
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	OVERVIEW::
--		[[#ClassOverview]] :
--		[[#tek.class : Class]] /
--		[[#tek.ui.class.layout : Layout]] /
--		FixedLayout
--
--		This class implements a [[#tek.ui.class.group : Group]]'s ''fixed''
--		layouting strategy. Using the fixed layouter, the size and position of
--		individual elements in a group is predetermined by the contents of
--		their ''fixed'' style property, with the coordinates of the rectangle
--		in the order left, top, right, bottom, e.g.:
--
--				ui.Window:new {
--				  Layout = "fixed", Width = 400, Height = 400,
--				  Children = {
--				    ui.Button:new {
--				      Style = "fixed: 10 280 60 320"
--				    }
--				  }
--				}
--
--	OVERRIDES::
--		- Layout:askMinMax()
--		- Layout:layout()
--
-------------------------------------------------------------------------------

local ui = require "tek.ui".checkVersion(112)
local Layout = require "tek.ui.class.layout"
local tonumber = tonumber

local FixedLayout = Layout.module("tek.ui.layout.fixed", "tek.ui.class.layout")
FixedLayout._VERSION = "Fixed Layout 2.2"

function FixedLayout:layout(group, r1, r2, r3, r4, markdamage)
	local children = group.Children
	local freeregion = group.FreeRegion
	for i = 1, #children do
		local c = children[i]
		local fixed = c.Properties["fixed"] or "0 0 0 0"
		local f1, f2, f3, f4 = fixed:match("(%d+) (%d+) (%d+) (%d+)")
		-- enter recursion:
		c:layout(tonumber(f1) or 0, tonumber(f2) or 0, tonumber(f3) or 0,
			tonumber(f4) or 0, markdamage)
		-- punch a hole for the element into the background:
		c:punch(freeregion)
	end
end

function FixedLayout:askMinMax(group, m1, m2, m3, m4)
	local children = group.Children
	for i = 1, #children do
		local c = children[i]
		c:askMinMax(m1, m2, m3, m4)
	end
	return m1 or 0, m2 or 0, m3 or 0, m4 or 0
end

return FixedLayout
