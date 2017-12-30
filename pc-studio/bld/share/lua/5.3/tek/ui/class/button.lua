-------------------------------------------------------------------------------
--
--	tek.ui.class.button
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
--		[[#tek.ui.class.widget : Widget]] /
--		[[#tek.ui.class.text : Text]] /
--		Button ${subclasses(Button)}
--
--		The Button class implements a Text element with a ''button mode''
--		(behavior) and ''button class'' (appearance). In addition to that,
--		it enables the initialization of a possible keyboard shortcut from
--		a special initiatory character (by default an underscore) preceding
--		a letter in the element's {{Text}} attribute.
--
--	NOTES::
--		This class adds redundancy, because it differs from the
--		[[#tek.ui.class.text : Text]] class only in that it initializes a few
--		defaults differently in its {{new()}} method. To avoid this overhead,
--		use the Text class directly, or create a ''Button factory'' like this:
--				function newButton(text)
--				  return ui.Text:new { Mode = "button", Class = "button",
--				    Text = text, KeyCode = true }
--				end
--
-------------------------------------------------------------------------------

local ui = require "tek.ui".checkVersion(112)
local Text = ui.require("text", 28)

local Button = Text.module("tek.ui.class.button", "tek.ui.class.text")
Button._VERSION = "Button 2.1"

-------------------------------------------------------------------------------
--	Class implementation:
-------------------------------------------------------------------------------

function Button.new(class, self)
	self = self or { }
	if self.KeyCode == nil then
		self.KeyCode = true
	end
	self.Mode = self.Mode or "button"
	self = Text.new(class, self)
	self:addStyleClass("button")
	return self
end

return Button
