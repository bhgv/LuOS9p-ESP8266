#!/usr/bin/env lua

local ui = require "tek.ui"
local Region = require "tek.lib.region"
local DefaultLayout = ui.loadClass("layout", "default")

-------------------------------------------------------------------------------

local RandomLayout = DefaultLayout:newClass()

function RandomLayout.new(class, self)
	self = self or { }
	self.Seed = false
	self = DefaultLayout.new(class, self)
	self:newLayout()
	return self
end

function RandomLayout:newLayout()
	self.Seed = os.time()
end

function RandomLayout:layout(group, r1, r2, r3, r4, markdamage)
	
	local children = group.Children
	local freeregion = group.FreeRegion
	
	local region = Region.new()
	local x, y, x1, y1 = group:getRect()
	local w = x1 - x + 1
	local h = y1 - y + 1
	
	math.randomseed(self.Seed)
	
	for i = 1, #children do
		local c = children[i]
		local minw, minh = c.MinMax:get()
		local x0, y0, x1, y1
		repeat
			x0 = math.random(0, w - 1 - minw)
			y0 = math.random(0, h - 1 - minh)
			x1 = math.random(x0 + minw, math.min(w - 1, x0 + minw))
			y1 = math.random(y0 + minh, math.min(h - 1, y0 + minh))
		until not region:checkIntersect(x0, y0, x1, y1)
		
		region:orRect(x0, y0, x1, y1)
		
		-- enter recursion:
		c:layout(x0 + x, y0 + y, x1 + x, y1 + y, markdamage)
		
		-- punch a hole for the element into the background:
		c:punch(freeregion)
	end
end

-------------------------------------------------------------------------------

local Layouter = RandomLayout:new { }

local Order = 1

local Texts = { "Click", "Me", "in", "the", "Correct", "Order" }

local app = ui.Application:new
{
	ProgramName = "Layout Hook Demo",
	Author = "Timm S. Müller",
}

local win = ui.Window:new 
{
	HideOnEscape = true,
	Orientation = "vertical",
}

local group = ui.Group:new
{
	Width = 400,
	Height = 400,
	Legend = "Custom Layout Group",
	Layout = Layouter
}

app:addMember(win)
win:addMember(group)

for i = 1, #Texts do
	group:addMember(ui.Button:new { Text = Texts[i],
		onClick = function(self)
			if i == Order then
				if Order == #Texts then
					Order = 1
					Layouter:newLayout()
					group:rethinkLayout(1)
				else
					Order = Order + 1
				end
			else
				Order = 1
			end
		end
	})
end

win:addMember(ui.Text:new { Width = "fill", Text = "Click Buttons in the Correct Order" })

app:run()
