#!/usr/bin/env lua

local ui = require "tek.ui"

local Frame1 = ui.Text:new
{
	Text = "Element 1\nFixed Size",
	Class = "button",
	Style = [[
		font: ui-huge;
		background-color: #cc0000;
		color: #fff;
		width: 350;
		height: 350;
		margin: 10;
	 ]]
}

local Frame2 = ui.Text:new
{
	Text = "Element 2\nFlexible Height",
	Class = "button",
	Style = [[
		font: ui-huge;
		background-color: #00aa00;
		color: #fff;
		width: 350;
		max-height: none;
		margin: 10;
	]]
}

local Frame3 = ui.Text:new
{
	Text = "Element 3\nFlexible Width",
	Class = "button",
	Style = [[
		font: ui-huge;
		background-color: #0000aa;
		color: #fff;
		max-width: none;
		height: 350;
		margin: 10;
	]]
}

local app = ui.Application:new
{
	Children =
	{
		ui.Window:new
		{
			Title = "Canvas and Scrollgroup",
			HideOnEscape = true,
			Orientation = "vertical",
			Children =
			{
				ui.Group:new
				{
					SameSize = true,
					Children =
					{
						ui.Button:new
						{
							Text = "Element _1",
							onClick = function(self)
								self:getById("the-canvas"):setValue("Child", Frame1)
							end
						},
						ui.Button:new
						{
							Text = "Element _2",
							onClick = function(self)
								self:getById("the-canvas"):setValue("Child", Frame2)
							end
						},
						ui.Button:new
						{
							Text = "Element _3",
							onClick = function(self)
								self:getById("the-canvas"):setValue("Child", Frame3)
							end
						},
						ui.Button:new
						{
							Text = "_Clear",
							onClick = function(self)
								self:getById("the-canvas"):setValue("Child", false)
							end
						}
					}
				},
				ui.ScrollGroup:new
				{
					HSliderMode = "auto",
					VSliderMode = "auto",
					Child = ui.Canvas:new
					{
-- 						Style = "background-image: url(bin/graphics/locale.ppm)",
						Style = "background-color: #678",
						UseChildBG = false,
						AutoWidth = true,
						AutoHeight = true,
						Id = "the-canvas",
					}
				}
			}
		}
	}

}

app:run()
app:hide()
app:cleanup()
