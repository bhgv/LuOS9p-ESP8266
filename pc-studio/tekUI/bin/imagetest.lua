#!/usr/bin/env lua

local ui = require "tek.ui"

local RadioImage1 = ui.getStockImage("radiobutton")
local RadioImage2 = ui.getStockImage("radiobutton", 2)
local BitMapImage1 = ui.loadImage(ui.ProgDir .. "/graphics/world.ppm")
local BitMapImage2 = ui.loadImage(ui.ProgDir .. "/graphics/locale.ppm")

-------------------------------------------------------------------------------
--	Main:
-------------------------------------------------------------------------------

ui.Application:new 
{
	Children = 
	{
		ui.Window:new 
		{
			HideOnEscape = true,
			Children = 
			{
				ui.ImageWidget:new 
				{ 
					Image = RadioImage1, 
					MinWidth = 20,
					MinHeight = 20,
					Mode = "button",
					Style = "padding: 10",
					ImageAspectX = 2,
					ImageAspectY = 3,
					onPress = function(self)
						ui.ImageWidget.onPress(self)
						self:setValue("Image", self.Pressed and RadioImage2 or RadioImage1)
					end
				},
				ui.ImageWidget:new 
				{ 
					Mode = "button",
					Image = BitMapImage1,
					Style = "padding: 10",
					onPress = function(self)
						ui.ImageWidget.onPress(self)
						self:setValue("Image", self.Pressed and BitMapImage2 or BitMapImage1)
					end
				}
			}
		}
	}
}:run()
