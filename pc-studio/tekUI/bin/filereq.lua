#!/usr/bin/env lua

local ui = require "tek.ui"

local APP_ID = "tekui-demo"
local DOMAIN = "schulze-mueller.de"

local L = ui.getLocale(APP_ID, DOMAIN)

app = ui.Application:new
{
	ApplicationId = APP_ID,
	Domain = DOMAIN,
	Children =
	{
		ui.Window:new
		{
			Title = L.FILE_REQUEST,
			Orientation = "vertical",
			HideOnEscape = true,
			Children =
			{
				ui.Group:new
				{
					Legend = L.LEGEND_SELECTION_TYPE,
					Orientation = "vertical",
					Children =
					{
						ui.RadioButton:new
						{
							Id = "selectmode-all",
							Text = L.SHOW_FILES_AND_DIRS,
							Selected = true,
						},
						ui.RadioButton:new
						{
							Text = L.SHOW_ONLY_DIRS,
						},
						ui.CheckMark:new
						{
							Id = "multiselect",
							Text = L.MULTISELECT,
						},
					},
				},
				ui.Group:new
				{
					Legend = L.LEGEND_SELECT_PATHS,
					Columns = 2,
					Children =
					{
						ui.Text:new
						{
							Text = L.PATH,
							Class = "caption",
							Style = "text-align: right",
							Width = "auto",
							KeyCode = true,
						},
						ui.Group:new
						{
							Style = "valign: center",
							Children =
							{
								ui.Input:new
								{
									Id = "pathfield",
									KeyCode = ui.extractKeyCode(L.PATH),
								},
								ui.ImageWidget:new
								{
									Mode = "button",
									Class = "button",
									Width = "fill",
									MinHeight = 17,
									ImageAspectX = 5,
									ImageAspectY = 7,
									Height = "fill",
									Image = ui.getStockImage("file"),
									onClick = function(self)
										local app = self.Application
										app:addCoroutine(function()
											local pathfield = app:getById("pathfield")
											local filefield = app:getById("filefield")
											local statusfield = app:getById("statusfield")
											local status, path, select = app:requestFile
											{
												BasePath = app:getById("basefield"):getText(),
												Path = pathfield:getText(),
												SelectMode = app:getById("multiselect").Selected and
													"multi" or "single",
												DisplayMode = app:getById("selectmode-all").Selected and
													"all" or "onlydirs"
											}
											statusfield:setValue("Text", status)
											if status == "selected" then
												pathfield:setValue("Text", path)
												app:getById("filefield"):setValue("Text",
													table.concat(select, ", "))
											end
										end)
									end,
								}
							}
						},
						
						ui.Text:new
						{
							Text = L.BASEPATH,
							Class = "caption",
							Style = "text-align: right",
							Width = "auto",
							KeyCode = true,
						},
						ui.Group:new
						{
							Style = "valign: center",
							Children =
							{
								ui.Input:new
								{
									Id = "basefield",
									KeyCode = ui.extractKeyCode(L.BASEPATH),
								},
								ui.ImageWidget:new
								{
									Mode = "button",
									Class = "button",
									Width = "fill",
									MinHeight = 17,
									ImageAspectX = 5,
									ImageAspectY = 7,
									Height = "fill",
									Image = ui.getStockImage("file"),
									onClick = function(self)
										local app = self.Application
										app:addCoroutine(function()
											local basefield = app:getById("basefield")
											local status, path, select = app:requestFile
											{
												SelectText = L.SELECT,
												DisplayMode = "onlydirs",
												Path = basefield:getText(),
											}
											if status == "selected" then
												basefield:setValue("Text", path)
											end
										end)
									end,
								}
							}
						},
						ui.Text:new
						{
							Class = "caption",
							Width = "auto",
						},
						ui.Text:new
						{
							Text = L.BASEPATH_ANNOTATION,
							Style = "text-align: left";
							Class = "caption",
						},
						
					}
				},

				ui.Group:new
				{
					Legend = L.LEGEND_SELECT_RESULT,
					Columns = 2,
					Children =
					{
						ui.Text:new
						{
							Text = L.STATUS,
							Class = "caption",
							Width = "auto",
						},
						ui.Text:new
						{
							Id = "statusfield",
							Style = "text-align: left",
						},
						ui.Text:new
						{
							Text = L.SELECTED,
							Class = "caption",
							Style = "text-align: right",
							Width = "auto",
							KeyCode = true,
						},
						ui.Input:new
						{
							Id = "filefield",
						},
					}
				},
			}
		}
	}
}:run()
