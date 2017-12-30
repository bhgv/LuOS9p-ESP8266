#!/usr/bin/env lua

ui = require "tek.ui"

ui.Application:new
{
	AuthorStyleSheets = "tutorial",
	Children =
	{
		ui.Window:new
		{
			Title = "Tutorial 2",
			Children =
			{
				ui.Button:new
				{
					Text = "Hello world",
					onClick = function(self)
						print "Hello world"
					end
				}
			}
		}
	}
}:run()
