#!/usr/bin/env lua

ui = require "tek.ui"

ui.Application:new
{
	Children =
	{
		ui.Window:new
		{
			Title = "Hello",
			HideOnEscape = true,
			Children =
			{
				ui.Button:new
				{
					Text = "_Hello, World!",
					onClick = function(self)
						print "Hello, World!"
					end
				}
			}
		}
	}
}:run()
