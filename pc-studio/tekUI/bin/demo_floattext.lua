#!/usr/bin/env lua

local ui = require "tek.ui"
local L = ui.getLocale("tekui-demo", "schulze-mueller.de")

-------------------------------------------------------------------------------
--	Create demo window:
-------------------------------------------------------------------------------

local window = ui.Window:new
{
	Orientation = "vertical",
	Id = "floattext-window",
	Title = L.FLOATTEXT_TITLE,
	Status = "hide",
	Orientation = "vertical",
	HideOnEscape = true,
	SizeButton = true,
	Children =
	{
		ui.Group:new
		{
			Children =
			{
				ui.Group:new
				{
					Orientation = "vertical",
					Children =
					{
						ui.Text:new
						{
							Text = L.FLOATTEXT_PREFORMATTED,
						},
						ui.Canvas:new
						{
							Weight = 0x6000,
							CanvasWidth = 500,
							CanvasHeight = 500,
							Child = ui.FloatText:new
							{
								Style = "font: ui-fixed; background-color: dark; color: bright; margin: 10",
								Preformatted = true,
								Text = [[

                        Ecce homo
              Friedrich Nietzsche
                      (1844-1900)

  Ja! Ich weiß, woher ich stamme!
    Ungesättigt gleich der Flamme
     Glühe und verzehr' ich mich.
 Licht wird alles, was ich fasse,
      Kohle alles, was ich lasse:
        Flamme bin ich sicherlich
]]
							}
						}
					}
				},
				ui.Handle:new { },
				ui.Group:new
				{
					Orientation = "vertical",
					Children =
					{
						ui.Text:new
						{
							Text = L.FLOATTEXT_DYNAMIC_LAYOUT,
						},
						ui.ScrollGroup:new
						{
							HSliderMode = "off",
							VSliderMode = "on",
							Child = ui.Canvas:new
							{
								AutoWidth = true,
								Child = ui.FloatText:new
								{
									Style = "margin: 10",
									Text = [[
										Under der linden
										Walther von der Vogelweide (1170-1230)

										Under der linden
										an der heide,
										dâ unser zweier bette was,
										dâ mugent ir vinden
										schône beide
										gebrochen bluomen unde gras.
										vor dem walde in einem tal,
										tandaradei,
										schône sanc diu nahtegal.

										Ich kam gegangen
										zuo der ouwe:
										dô was mîn friedel komen ê.
										dâ wart ich enpfangen
										hêre frouwe,
										daz ich bin sælic iemer mê.
										kuster mich? wol tûsentstunt:
										tandaradei,
										seht wie rôt mir ist der munt.

										Dô hete er gemachet
										alsô rîche
										von bluomen eine bettestat.
										des wirt noch gelachet
										inneclîche,
										kumt iemen an daz selbe pfat.
										bî den rôsen er wol mac,
										tandaradei,
										merken wâ mirz houbet lac.

										Daz er bî mir læge,
										wesse ez iemen
										(nu enwelle got!), sô schamte ich mich.
										wes er mit mir pflæge,
										niemer niemen
										bevinde daz, wan er unt ich,
										und ein kleinez vogellîn:
										tandaradei,
										daz mac wol getriuwe sîn.]]
								}
							}
						}
					}
				}
			}
		},
		ui.Text:new
		{
			Text = L.FLOATTEXT_YOU_CAN_DRAG_THE_BAR,
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
		Name = L.FLOATTEXT_TITLE,
		Description = L.FLOATTEXT_DESCRIPTION
	}
end
