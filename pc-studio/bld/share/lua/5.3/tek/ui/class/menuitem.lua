-------------------------------------------------------------------------------
--
--	tek.ui.class.menuitem
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
--		[[#tek.ui.class.popitem : PopItem]] /
--		MenuItem ${subclasses(MenuItem)}
--
--		This class implements basic, recursive items for window and popup
--		menus with a typical menu look; in particular, it displays the
--		[[#tek.ui.class.popitem : PopItem]]'s {{Shortcut}} string and an
--		arrow to indicate the presence of a sub menu.
--
--	OVERRIDES::
--		- Area:askMinMax()
--		- Area:draw()
--		- Area:layout()
--		- Class.new()
--		- Area:show()
--
-------------------------------------------------------------------------------

local db = require "tek.lib.debug"
local ui = require "tek.ui".checkVersion(112)
local PopItem = ui.require("popitem", 22)

local max = math.max
local floor = math.floor

local MenuItem = PopItem.module("tek.ui.class.menuitem", "tek.ui.class.popitem")
MenuItem._VERSION = "MenuItem 11.1"

-------------------------------------------------------------------------------
--	Constants and class data:
-------------------------------------------------------------------------------

local ArrowImage = ui.Image:new
{
	{ 0x7000,0x4000, 0x7000,0xc000, 0xb000,0x8000 }, false, false, true,
	{ { 0x1000, 3, { 1, 2, 3 }, "menu-detail" } },
}

-------------------------------------------------------------------------------
--	MenuItem class:
-------------------------------------------------------------------------------

function MenuItem.new(class, self)
	self = self or { }
	if self.Children then
		self.Mode = "toggle"
	else
		self.Mode = "button"
	end
	-- prevent superclass from filling in text records:
	self.TextRecords = self.TextRecords or { }
	self = PopItem.new(class, self)
	self:addStyleClass("menuitem")
	return self
end

function MenuItem:setup(app, win)
	if self.Children then
		if self.PopupBase then
			self.Image = ArrowImage
			self.ImageRect = { 0, 0, 0, 0 }
		end
	end
	PopItem.setup(self, app, win)
	self:setFlags(ui.FL_CURSORFOCUS)
	local font = self.Application.Display:openFont(self.Properties["font"])
	self:setTextRecord(1, self.Text, font, "left")
	if self.Shortcut and not self.Children then
		self:setTextRecord(2, self.Shortcut, font, "left")
	end
end

function MenuItem:doSubMenu()
	-- subitems are handled in baseclass:
	PopItem.doSubMenu(self)
	-- handle baseitem:
	if self.Window then
		local popup = self.Window.ActivePopup
		if popup then
			-- hilite over baseitem while another open popup in menubar:
			if self.Hilite == true and popup ~= self then
				db.info("have another popup open")
				self:beginPopup()
				self:setValue("Selected", true)
			end
		end
	end
end

function MenuItem:beginPopup(baseitem)
	if self.Window and self.Window.ActivePopup and
		self.Window.ActivePopup ~= self then
		-- close already open menu in same group:
		self.Window.ActivePopup:endPopup()
	end
	-- subitems are handled in baseclass:
	PopItem.beginPopup(self, baseitem)
end

function MenuItem:endPopup()
	-- subitems are handled in baseclass:
	PopItem.endPopup(self)
	self:setValue("Focus", false)
end

return MenuItem
