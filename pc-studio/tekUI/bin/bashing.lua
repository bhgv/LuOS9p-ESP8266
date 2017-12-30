#!/usr/bin/env lua

local ui = require "tek.ui"

ui.Application:new
{
	Children =
	{
		ui.Window:new
		{
			HideOnEscape = true,
			Columns = 2,
			Children =
			{
				ui.Tunnel:new { MaxWidth = 400, MaxHeight = 200, VAlign = "bottom" },
				ui.Tunnel:new { MaxWidth = 200, MaxHeight = 400 },
				ui.Tunnel:new { MaxWidth = 200, MaxHeight = 400, HAlign = "right" },
				ui.Tunnel:new { MaxWidth = 400, MaxHeight = 200 },
			},
		},
	},
}:run()
