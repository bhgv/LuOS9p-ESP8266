#!/usr/bin/env lua

local ui = require "tek.ui"
local List = require "tek.class.list"
local Group = ui.Group
local MenuItem = ui.MenuItem
local PopItem = ui.PopItem
local Spacer = ui.Spacer

local L = ui.getLocale("tekui-demo", "schulze-mueller.de")

-------------------------------------------------------------------------------
--	Create demo window:
-------------------------------------------------------------------------------

local window = ui.Window:new
{
	Orientation = "vertical",
	Id = "popups-window",
	Title = L.POPUPS_TITLE,
	Status = "hide",
	HideOnEscape = true,
	SizeButton = true,
	Children =
	{
		Orientation = "vertical",
		Group:new
		{
			Class = "menubar",
			Children =
			{
				MenuItem:new
				{
					Text = "File",
					Children =
					{
						MenuItem:new { Text = "New" },
						Spacer:new { },
						MenuItem:new { Text = "Open..." },
						MenuItem:new { Text = "Open Recent" },
						MenuItem:new { Text = "Open With",
							Children =
							{
								MenuItem:new { Text = "Lua" },
								MenuItem:new { Text = "Kate" },
								MenuItem:new { Text = "UltraEdit" },
								MenuItem:new { Text = "Other..." },
							}
						},
						MenuItem:new
						{
							Text = "Nesting",
							Children =
							{
								MenuItem:new
								{
									Text = "Any",
									Children =
									{
										MenuItem:new
										{
											Text = "Recursion",
											Children =
											{
												MenuItem:new
												{
													Text = "Depth" ,
													Children =
													{
														MenuItem:new
														{
															Text = "Will" ,
															Children =
															{
																MenuItem:new
																{
																	Text = "Do."
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
						},
						Spacer:new { },
						MenuItem:new { Text = "Save..." },
						MenuItem:new { Text = "Save as" },
						Spacer:new { },
						MenuItem:new { Text = "_Reload" },
						MenuItem:new { Text = "Print" },
						Spacer:new { },
						MenuItem:new { Text = "Close" },
						MenuItem:new { Text = "Close all", Shortcut = "Shift+Ctrl+Q" },
						Spacer:new { },
						MenuItem:new
						{
							Text = "_Quit",
							Shortcut = "Ctrl+Q",
						}
					}
				},
				MenuItem:new
				{
					Text = "Edit",
					Children =
					{
						MenuItem:new { Text = "Undo" },
						MenuItem:new { Text = "Redo", Disabled = true },
						Spacer:new { },
						MenuItem:new { Text = "Cut" },
						MenuItem:new { Text = "Copy" },
						MenuItem:new { Text = "Paste" },
						Spacer:new { },
						MenuItem:new { Text = "Select all" },
						MenuItem:new { Text = "Deselect", Disabled = true },
						Spacer:new { },
						ui.CheckMark:new { Text = "Checkmark", Class = "menuitem" },
						ui.CheckMark:new { Text = "Fomp", Class = "menuitem" },
						ui.CheckMark:new { Text = "Disabled", Class = "menuitem", Disabled = true },
						Spacer:new { },
						ui.RadioButton:new { Text = "One", Class = "menuitem" },
						ui.RadioButton:new { Text = "Two", Class = "menuitem" },
						ui.RadioButton:new { Text = "Three", Disabled = true, Class = "menuitem" },
					}
				}
			}
		},
		Group:new
		{
			Children =
			{
				Group:new
				{
					Orientation = "vertical",
					Children =
					{
						Group:new
						{
							Children =
							{
								PopItem:new
								{
									Text = "_Normal Popups",
									-- these children are not connected initially:
									Children =
									{
										PopItem:new
										{
											Text = "_Button Style",
											Children =
											{
												PopItem:new
												{
													Text = "_English",
													Children =
													{
														PopItem:new { Text = "_One" },
														PopItem:new { Text = "_Two" },
														PopItem:new { Text = "Th_ree" },
														PopItem:new { Text = "_Four" },
													}
												},
												PopItem:new
												{
													Text = "Es_pañol",
													Children =
													{
														PopItem:new { Text = "_Uno" },
														PopItem:new { Text = "_Dos" },
														PopItem:new { Text = "_Tres", Disabled = true },
														PopItem:new { Text = "_Cuatro" },
													}
												}
											}
										},
										MenuItem:new
										{
											Text = "_Menu Style",
											Children =
											{
												MenuItem:new
												{
													Text = "_Français",
													Children =
													{
														MenuItem:new { Text = "_Un" },
														MenuItem:new { Text = "_Deux" },
														MenuItem:new { Text = "_Trois" },
														MenuItem:new { Text = "_Quatre", Disabled = true },
													}
												},
												MenuItem:new
												{
													Text = "_Deutsch",
													Children =
													{
														MenuItem:new { Text = "_Eins" },
														MenuItem:new { Text = "_Zwei" },
														MenuItem:new { Text = "_Drei" },
														MenuItem:new { Text = "_Vier" },
													}
												},
												MenuItem:new
												{
													Text = "Binary",
													Children =
													{
														MenuItem:new { Text = "001" },
														MenuItem:new { Text = "010" },
														MenuItem:new { Text = "011" },
														MenuItem:new { Text = "100" },
													}
												}
											}
										}
									}
								},
								PopItem:new
								{
									Text = "_Special Popups",
									Children =
									{
										ui.Tunnel:new { }
									}
								},
								ui.PopList:new
								{
									Id = "euro-combo",
									Text = "_Combo Box",
									Width = "free",
									ListObject = List:new
									{
										Items =
										{
											{ { "Combo Box" } },
											{ { "Uno - Un - Uno" } },
											{ { "Dos - Deux - Due" } },
											{ { "Tres - Trois - Tre" } },
											{ { "Cuatro - Quatre - Quattro" } },
											{ { "Cinco - Cinq - Cinque" } },
											{ { "Seis - Six - Sei" } },
											{ { "Siete - Sept - Sette" } },
											{ { "Ocho - Huit - Otto" } },
											{ { "Nueve - Neuf - Nove" } },
										}
									},
									onSelect = function(self)
										ui.PopList.onSelect(self)
										local item = self.ListObject:getItem(self.SelectedLine)
										if item then
-- 											self:getById("japan-combo"):setValue("SelectedLine", self.SelectedLine)
											self:getById("popup-show"):setValue("Text", item[1][1])
										end
									end,
								},
-- 								ui.PopList:new
-- 								{
-- 									Id = "japan-combo",
-- 									Text = "日本語",
-- 									-- Class = "japanese",
-- 									Style = "font:kochi mincho",
-- 									MinWidth = 80,
-- 									ListObject = List:new
-- 									{
-- 										Items =
-- 										{
-- 											{ { "日本語" } },
-- 											{ { "一" } },
-- 											{ { "二" } },
-- 											{ { "三" } },
-- 											{ { "四" } },
-- 											{ { "五" } },
-- 											{ { "六" } },
-- 											{ { "七" } },
-- 											{ { "八" } },
-- 											{ { "九" } },
-- 										}
-- 									}
-- 								}
							}
						},
						ui.ScrollGroup:new
						{
							VSliderMode = "on",
							HSliderMode = "on",
							Child = ui.Canvas:new
							{
								CanvasHeight = 400,
								AutoWidth = true,
								AutoPosition = true,
								Child = ui.Group:new
								{
									Orientation = "vertical",
									Children =
									{
										ui.Text:new
										{
											Height = "free",
											Text = "",
											Style = "font: ui-huge:48",
											Id = "popup-show",
										},
										ui.Group:new
										{
											Children =
											{
												ui.PopList:new
												{
													SelectedLine = 1,
													ListObject = List:new
													{
														Items =
														{
															{ { "a Popup in" } },
															{ { "a shifted" } },
															{ { "Scrollgroup" } },
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
		Name = L.POPUPS_TITLE,
		Description = L.POPUPS_DESCRIPTION
	}
end
