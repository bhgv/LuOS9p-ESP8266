#!/usr/bin/env lua

ui = require "tek.ui"

ui.Application:new
{
	AuthorStyleSheets = "tutorial",
	Children =
	{
		ui.Window:new
		{
			Title = "Tutorial 3",
			Children =
			{
				ui.Button:new
				{
					Text = "Hello",
					Width = "auto",
				},
				ui.Button:new
				{
					Text = "world",
					MaxHeight = "none",
				},
			}
		}
	}

}:run()
