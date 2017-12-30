#!/usr/bin/env lua

local ui = require "tek.ui"

local function setinner(self, idx, val)
	for _, key in ipairs { "None", "SameSize", "SameWidth", "SameHeight" } do
		local e = self:getById(key)
		for _, c in ipairs(e.Children) do
			c[idx] = val
			c:rethinkLayout(1)
		end
	end
end

local function setouter(self, idx, val)
	for _, key in ipairs { "None", "SameSize", "SameWidth", "SameHeight" } do
		local e = self:getById(key)
		e[idx] = val
		e:rethinkLayout(1)
	end
end

ui.application:new
{
	Children =
	{
		ui.window:new
		{
			HideOnEscape = true,
			Title = "Grid Alignment",
			Children =
			{
				ui.Group:new
				{
					Orientation = "vertical",
					Legend = "Outer Align",
					Children =
					{
						ui.Group:new
						{
							Orientation = "vertical",
							Style = "max-width: 0",
							Legend = "Horiz",
							Children =
							{
								ui.RadioButton:new { Text = "Left",
									onClick = function(self)
										setouter(self, "HAlign", "left")
									end
								},
								ui.RadioButton:new { Text = "Center", Selected = true,
									onClick = function(self)
										setouter(self, "HAlign", "center")
									end
								},
								ui.RadioButton:new { Text = "Right",
									onClick = function(self)
										setouter(self, "HAlign", "right")
									end
								}
							}
						},
						ui.Group:new
						{
							Orientation = "vertical",
							Legend = "Vert",
							Style = "max-width: 0",
							Children =
							{
								ui.RadioButton:new { Text = "Top",
									onClick = function(self)
										setouter(self, "VAlign", "top")
									end
								},
								ui.RadioButton:new { Text = "Center", Selected = true,
									onClick = function(self)
										setouter(self, "VAlign", "center")
									end
								},
								ui.RadioButton:new { Text = "Bottom",
									onClick = function(self)
										setouter(self, "VAlign", "bottom")
									end
								}
							}
						}
					}
				},
				ui.Group:new
				{
					Legend = "Grids",
					Columns = 2,
					HAlign = "right",
					VAlign = "bottom",
					Children =
					{
						ui.Group:new
						{
							Id = "SameSize",
							Legend = "SameSize",
							Columns = 2,
							SameSize = true,
							HAlign = "center",
							VAlign = "center",
							Children =
							{
								ui.text:new { Mode = "button", Class = "button", Text = "Big",
									HAlign = "center", VAlign = "center",
									Style = "font: ui-large; max-width: 0" },
								ui.text:new { Mode = "button", Class = "button", Text = "Normal",
									HAlign = "center", VAlign = "center",
									Style = "font: ui-main; max-width: 0" },
								ui.text:new { Mode = "button", Class = "button", Text = "Small",
									HAlign = "center", VAlign = "center",
									Style = "font: ui-small; max-width: 0" },
								ui.text:new { Mode = "button", Class = "button", Text = "Huge",
									HAlign = "center", VAlign = "center",
									Style = "font: ui-huge; max-width: 0" },
							}
						},
						ui.Group:new
						{
							Id = "SameWidth",
							Legend = "SameWidth",
							Columns = 2,
							SameSize = "width",
							HAlign = "center",
							VAlign = "center",
							Children =
							{
								ui.text:new { Mode = "button", Class = "button", Text = "Big",
									HAlign = "center", VAlign = "center",
									Style = "font: ui-large; max-width: 0" },
								ui.text:new { Mode = "button", Class = "button", Text = "Normal",
									HAlign = "center", VAlign = "center",
									Style = "font: ui-main; max-width: 0" },
								ui.text:new { Mode = "button", Class = "button", Text = "Small",
									HAlign = "center", VAlign = "center",
									Style = "font: ui-small; max-width: 0" },
								ui.text:new { Mode = "button", Class = "button", Text = "Huge",
									HAlign = "center", VAlign = "center",
									Style = "font: ui-huge; max-width: 0" },
							}
						},
						ui.Group:new
						{
							Id = "SameHeight",
							Legend = "SameHeight",
							Columns = 2,
							SameSize = "height",
							HAlign = "center",
							VAlign = "center",
							Children =
							{
								ui.text:new { Mode = "button", Class = "button", Text = "Big",
									HAlign = "center", VAlign = "center",
									Style = "font: ui-large; max-width: 0" },
								ui.text:new { Mode = "button", Class = "button", Text = "Normal",
									HAlign = "center", VAlign = "center",
									Style = "font: ui-main; max-width: 0" },
								ui.text:new { Mode = "button", Class = "button", Text = "Small",
									HAlign = "center", VAlign = "center",
									Style = "font: ui-small; max-width: 0" },
								ui.text:new { Mode = "button", Class = "button", Text = "Huge",
									HAlign = "center", VAlign = "center",
									Style = "font: ui-huge; max-width: 0" },
							}
						},
						ui.Group:new
						{
							Id = "None",
							Legend = "None",
							Columns = 2,
							HAlign = "center",
							VAlign = "center",
							Children =
							{
								ui.text:new { Mode = "button", Class = "button", Text = "Big",
									HAlign = "center", VAlign = "center",
									Style = "font: ui-large; max-width: 0" },
								ui.text:new { Mode = "button", Class = "button", Text = "Normal",
									HAlign = "center", VAlign = "center",
									Style = "font: ui-main; max-width: 0" },
								ui.text:new { Mode = "button", Class = "button", Text = "Small",
									HAlign = "center", VAlign = "center",
									Style = "font: ui-small; max-width: 0" },
								ui.text:new { Mode = "button", Class = "button", Text = "Huge",
									HAlign = "center", VAlign = "center",
									Style = "font: ui-huge; max-width: 0" },
							}
						}
					}
				},
				ui.Group:new
				{
					Orientation = "vertical",
					Legend = "Inner Align",
					Children =
					{
						ui.Group:new
						{
							Orientation = "vertical",
							Legend = "Horiz",
							Style = "max-width: 0",
							Children =
							{
								ui.RadioButton:new { Text = "Left",
									onClick = function(self)
										setinner(self, "HAlign", "left")
									end
								},
								ui.RadioButton:new { Text = "Center", Selected = true,
									onClick = function(self)
										setinner(self, "HAlign", "center")
									end
								},
								ui.RadioButton:new { Text = "Right",
									onClick = function(self)
										setinner(self, "HAlign", "right")
									end
								}
							}
						},
						ui.Group:new
						{
							Orientation = "vertical",
							Legend = "Vert",
							Style = "max-width: 0",
							Children =
							{
								ui.RadioButton:new { Text = "Top",
									onClick = function(self)
										setinner(self, "VAlign", "top")
									end
								},
								ui.RadioButton:new { Text = "Center", Selected = true,
									onClick = function(self)
										setinner(self, "VAlign", "center")
									end
								},
								ui.RadioButton:new { Text = "Bottom",
									onClick = function(self)
										setinner(self, "VAlign", "bottom")
									end
								}
							}
						}
					}
				}
			}
		}
	}
}:run()
