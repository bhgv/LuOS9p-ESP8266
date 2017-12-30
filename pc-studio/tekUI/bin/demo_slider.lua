#!/usr/bin/env lua

local ui = require "tek.ui"
local Gauge = ui.Gauge
local Group = ui.Group
local ScrollBar = ui.ScrollBar
local Slider = ui.Slider
local Text = ui.Text
local L = ui.getLocale("tekui-demo", "schulze-mueller.de")

-------------------------------------------------------------------------------
--	Create demo window:
-------------------------------------------------------------------------------

local scrollbar1 = ui.ScrollBar:new
{
	Id = "slider-slider-1",
	Kind = "number",
	Width = "free",
	Min = 0,
	Max = 10
}

scrollbar1:addNotify("Value", ui.NOTIFY_ALWAYS, { ui.NOTIFY_ID, 
	"slider-text-1", "setValue", "Text", ui.NOTIFY_FORMAT, "%2.2f" })
scrollbar1:addNotify("Value", ui.NOTIFY_ALWAYS,
	{ ui.NOTIFY_ID, "slider-slider-2", "setValue", "Value", ui.NOTIFY_VALUE })
scrollbar1:addNotify("Value", ui.NOTIFY_ALWAYS,
	{ ui.NOTIFY_ID, "slider-gauge-1", "setValue", "Value", ui.NOTIFY_VALUE })

local scrollbar2 = ui.ScrollBar:new
{
	Id = "slider-slider-2",
	Kind = "number",
	Width = "free",
	Min = 0,
	Max = 10,
	Step = 1
}

scrollbar2:addNotify("Value", ui.NOTIFY_ALWAYS, { ui.NOTIFY_ID, 
	"slider-text-2", "setValue", "Text", ui.NOTIFY_FORMAT, "%.0f" })
scrollbar2:addNotify("Value", ui.NOTIFY_ALWAYS, 
	{ ui.NOTIFY_ID, "slider-slider-1", "setValue", "Value", ui.NOTIFY_VALUE })
scrollbar2:addNotify("Value", ui.NOTIFY_ALWAYS, 
	{ ui.NOTIFY_ID, "slider-gauge-1", "setValue", "Value", ui.NOTIFY_VALUE })

local scrollbar3 = ui.ScrollBar:new
{
	Id = "slider-slider-3",
	Kind = "number",
	Width = "free",
	Min = 10,
	Max = 20,
	Step = 1,
}

scrollbar3:addNotify("Value", ui.NOTIFY_ALWAYS, { ui.NOTIFY_ID, 
	"slider-text-3", "setValue", "Text", ui.NOTIFY_FORMAT, "%.0f" })
scrollbar3:addNotify("Value", ui.NOTIFY_ALWAYS,
	{ ui.NOTIFY_ID, "slider-slider-1", "setValue", "Range", ui.NOTIFY_VALUE })
scrollbar3:addNotify("Value", ui.NOTIFY_ALWAYS,
	{ ui.NOTIFY_ID, "slider-slider-2", "setValue", "Range", ui.NOTIFY_VALUE })

local slider1 = ui.Slider:new
{
	Id = "slider-1",
	Orientation = "vertical",
	Increment = 5,
	Max = 100,
}

slider1:addNotify("Value", ui.NOTIFY_ALWAYS,
	{ ui.NOTIFY_ID, "slider-2", "setValue", "Value", ui.NOTIFY_VALUE })

local slider2 = ui.Slider:new
{
	Id = "slider-2",
	Orientation = "vertical",
	Increment = 5,
	Max = 100,
}

slider2:addNotify("Value", ui.NOTIFY_ALWAYS,
	{ ui.NOTIFY_ID, "gauge-2", "setValue", "Value", ui.NOTIFY_VALUE })

local slider3 = ui.Slider:new
{
	Id = "slider-7",
	Increment = 5,
	Max = 100,
}

slider3:addNotify("Value", ui.NOTIFY_ALWAYS,
	{ ui.NOTIFY_ID, "slider-1", "setValue", "Value", ui.NOTIFY_VALUE })
slider3:addNotify("Value", ui.NOTIFY_ALWAYS,
	{ ui.NOTIFY_ID, "slider-6", "setValue", "Value", ui.NOTIFY_VALUE })

local slider4 = ui.Slider:new
{
	Id = "slider-5",
	Orientation = "vertical",
	Increment = 5,
	Max = 100,
}

slider4:addNotify("Value", ui.NOTIFY_ALWAYS,
	{ ui.NOTIFY_ID, "gauge-3", "setValue", "Value", ui.NOTIFY_VALUE })

local slider5 = ui.Slider:new
{
	Id = "slider-6",
	Orientation = "vertical",
	Increment = 5,
	Max = 100,
}

slider5:addNotify("Value", ui.NOTIFY_ALWAYS,
	{ ui.NOTIFY_ID, "slider-5", "setValue", "Value", ui.NOTIFY_VALUE })


local window = ui.Window:new
{
	Id = "slider-window",
	Title = L.SLIDER_TITLE,
	Status = "hide",
	Orientation = "vertical",
	HideOnEscape = true,
	SizeButton = true,
	Children =
	{
		Group:new
		{
			Legend = L.SLIDER_SLIDERS,
			Columns = 3,
			Width = "fill",
			Children =
			{
				Text:new
				{
					Text = L.SLIDER_CONTINUOUS,
					Width = "fill",
				},
				scrollbar1,
				Text:new
				{
					Id = "slider-text-1",
					Width = "fill",
					Text = "  0.00  ",
					KeepMinWidth = true,
				},
				
				Text:new
				{
					Text = L.SLIDER_INTEGER_STEP,
					Width = "fill",
				},
				scrollbar2,
				Text:new
				{
					Id = "slider-text-2",
					Width = "fill",
					Text = "  0  ",
					KeepMinWidth = true,
				},
				
				Text:new
				{
					Text = L.SLIDER_RANGE,
					Width = "fill",
				},
				scrollbar3,
				Text:new
				{
					Id = "slider-text-3",
					Width = "fill",
					Text = "  0  ",
					KeepMinWidth = true,
				}
			}
		},
		Group:new
		{
			Legend = L.SLIDER_GAUGE,
			Children =
			{
				Gauge:new
				{
					Min = 0,
					Max = 10,
					Id = "slider-gauge-1",
				}
			}
		},

		Group:new
		{
			Legend = L.SLIDER_CONNECTIONS,
			Children =
			{
				slider1,
				slider2,
				Group:new
				{
					Orientation = "vertical",
					VAlign = "center",
					Children =
					{
						slider3,
						Group:new
						{
							Children =
							{
								Gauge:new
								{
									Id = "gauge-2",
									Max = 100,
								},
								Gauge:new
								{
									Id = "gauge-3",
									Max = 100,
								}
							}
						}
					}
				},
				slider4,
				slider5
			}
		}
	}
}

-------------------------------------------------------------------------------
--	Started stand-alone or as part of the demo?
-------------------------------------------------------------------------------

if ui.ProgName:match("^demo_") then
	local app = ui.Application:new()
	ui.Application.connect(window)
	app:addMember(window)
	window:setValue("Status", "show")
	app:run()
else
	return
	{
		Window = window,
		Name = L.SLIDER_TITLE,
		Description = L.SLIDER_DESCRIPTION,
	}
end
