-------------------------------------------------------------------------------
--
--	tek.ui.class.spacer
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	OVERVIEW::
--		[[#ClassOverview]] :
--		[[#tek.class : Class]] /
--		[[#tek.class.object : Object]] /
--		[[#tek.ui.class.element : Element]] /
--		[[#tek.ui.class.area : Area]] /
--		[[#tek.ui.class.frame : Frame]] /
--		Spacer ${subclasses(Spacer)}
--
--		This class implements a separator that helps to arrange elements in
--		a group.
--
--	OVERRIDES::
--		- Element:setup()
--
-------------------------------------------------------------------------------

local ui = require "tek.ui".checkVersion(112)
local Frame = ui.require("frame", 21)

local Spacer = Frame.module("tek.ui.class.spacer", "tek.ui.class.frame")
Spacer._VERSION = "Spacer 2.3"

-------------------------------------------------------------------------------
--	setup: overrides
-------------------------------------------------------------------------------

function Spacer:setup(app, win)
	if self:getGroup().Orientation == "horizontal" then
		self.Width = "auto"
		self.Height = "fill"
	else
		self.Height = "auto"
		self.Width = "fill"
	end
	Frame.setup(self, app, win)
end

return Spacer
