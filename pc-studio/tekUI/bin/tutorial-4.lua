#!/usr/bin/env lua

ui = require "tek.ui"

ui.Application:new
{
	AuthorStyleSheets = "tutorial",
	Children =
	{
		ui.Window:new
		{
			Title = "Tutorial 4",
			Children =
			{
				ui.Button:new
				{
					Text = "Hello",
					Width = "auto",
					onPress = function(self)
						ui.Button.onPress(self)
						local button = self:getById("output")
						if self.Pressed then
							button:setValue("Text", "world")
						else
							button:setValue("Text", "")
						end
					end
				},
				ui.Text:new
				{
					Legend = "Output",
					Id = "output",
					Height = "free",
					Style = "font: :100",
				}
			}
		}
	}
}:run()
