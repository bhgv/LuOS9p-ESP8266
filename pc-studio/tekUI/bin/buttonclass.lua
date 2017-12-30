#!/usr/bin/env lua

ui = require "tek.ui"

-------------------------------------------------------------------------------
--	Create Button class:
-------------------------------------------------------------------------------

Button = ui.Text:newClass { _NAME = "_button" }

function Button.new(class, self)
	self = self or { }
	self.Class = "button"
	self.Mode = self.Mode or "button"
	return ui.Text.new(class, self)
end

-------------------------------------------------------------------------------
--	Create application:
-------------------------------------------------------------------------------

app = ui.Application:new
{
	ApplicationId = "tekui-demo",
	Domain = "schulze-mueller.de",
}

-------------------------------------------------------------------------------
--	Get the catalog of locale strings:
-------------------------------------------------------------------------------

L = app:getLocale()

-------------------------------------------------------------------------------
--	Create window:
-------------------------------------------------------------------------------

win = ui.Window:new { Title = L.HELLO }

-------------------------------------------------------------------------------
--	Create button:
-------------------------------------------------------------------------------

button = Button:new { Text = L.HELLO_WORLD }

button:addNotify("Pressed", false, {
	ui.NOTIFY_SELF,
	ui.NOTIFY_FUNCTION,
	function(self)
		print(L.HELLO_WORLD)
	end
})

-------------------------------------------------------------------------------
--	Link window to application and button to window:
-------------------------------------------------------------------------------

app:addMember(win)
win:addMember(button)

-------------------------------------------------------------------------------
--	Run the application:
-------------------------------------------------------------------------------

app:run()
