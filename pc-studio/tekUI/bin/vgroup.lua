#!/usr/bin/env lua

local ui = require "tek.ui"

ui.Application:new
{
	AuthorStyles = [[
		.huge { 
			width: free; 
			height: free; 
			font: ui-huge; 
		}
	]],

	Children =
	{
		ui.Window:new
		{
			Title = "Groups Demo",
			HideOnEscape = true,
			Children =
			{
				ui.ScrollGroup:new
				{
					Legend = "Virtual Group",
					HSliderMode = "on",
					VSliderMode = "on",
					Child = ui.Canvas:new
					{
						AutoPosition = true,
						MaxWidth = 500,
						MaxHeight = 500,
						CanvasWidth = 500,
						CanvasHeight = 500,
						Child = ui.Group:new
						{
							Columns = 2,
							Children =
							{
								ui.Button:new { Text = "Foo", Class = "huge" },
								ui.Button:new { Text = "Bar", Class = "huge" },
								ui.Button:new { Text = "Baz", Class = "huge" },
								ui.ScrollGroup:new
								{
									Legend = "Virtual Group",
									Width = 500,
									Height = 500,
									MinWidth = 0,
									MinHeight = 0,
									HSliderMode = "on",
									VSliderMode = "on",
									Child = ui.Canvas:new
									{
										AutoPosition = true,
										CanvasWidth = 500,
										CanvasHeight = 500,
										Child = ui.Group:new
										{
											Columns = 2,
											Children =
											{
												ui.Button:new { Text = "Red", Class = "huge" },
												ui.Button:new { Text = "Green", Class = "huge" },
												ui.Button:new { Text = "Blue", Class = "huge" },
												ui.ScrollGroup:new
												{
													Legend = "Virtual Group",
													Width = 500,
													Height = 500,
													MinWidth = 0,
													MinHeight = 0,
													HSliderMode = "on",
													VSliderMode = "on",
													Child = ui.Canvas:new
													{
														AutoPosition = true,
														CanvasWidth = 500,
														CanvasHeight = 500,
														Child = ui.Group:new
														{
															Columns = 2,
															Children =
															{
																ui.Button:new { Text = "One", Class = "huge" },
																ui.Button:new { Text = "Two", Class = "huge" },
																ui.Button:new { Text = "Three", Class = "huge" },
																ui.Button:new { Text = "Four", Class = "huge" },
															}
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
}:run()
