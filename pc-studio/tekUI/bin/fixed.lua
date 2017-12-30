#!/usr/bin/env lua

local ui = require "tek.ui"

ui.Application:new
{
	Children =
	{
		ui.Window:new
		{
			Title = "Fixed Layout",
			HideOnEscape = true,
			Layout = "fixed",
			Width = 400,
			Height = 400,
			Children =
			{
				ui.Button:new
				{
					Text = "One", 
					Style = "fixed: 10 10 100 100"
				},
				ui.Button:new
				{
					Text = "Two", 
					Style = "fixed: 30 120 140 190"
				},
				ui.Button:new
				{
					Text = "Three",
					Style = "fixed: 150 70 330 130"
				},
				ui.Button:new
				{
					Text = "Four", 
					Style = "fixed: 250 140 390 170"
				},
				ui.PageGroup:new
				{
					PageCaptions = { "Foo", "Bar", "Baz" },
					Style = "fixed: 0 200 399 399",
					Children =
					{
						ui.Group:new
						{
							Layout = "fixed",
							Children =
							{
								ui.Button:new
								{
									Text = "Foo", 
									Style = "fixed: 300 250 380 350"
								},
								ui.Button:new
								{
									Text = "Foo", 
									Style = "fixed: 10 280 60 320"
								}
							}
						},
						ui.Group:new
						{
							Layout = "fixed",
							Children =
							{
								ui.Button:new
								{
									Text = "Bar", 
									Style = "fixed: 120 260 250 320"
								},
								ui.Button:new
								{
									Text = "Bar", 
									Style = "fixed: 340 280 380 340"
								}
							}
						},
						ui.Group:new
						{
							Layout = "fixed",
							Children =
							{
								ui.Button:new
								{
									Text = "Baz", 
									Style = "fixed: 10 250 100 320"
								},
								ui.Button:new
								{
									Text = "Baz", 
									Style = "fixed: 200 240 250 290"
								}
							}
						}
					}
				}
			}
		}
	}
}:run()
