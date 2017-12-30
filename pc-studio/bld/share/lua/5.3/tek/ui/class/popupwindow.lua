-------------------------------------------------------------------------------
--
--	tek.ui.class.popupwindow
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
--		[[#tek.ui.class.group : Group]] /
--		[[#tek.ui.class.group : Window]] /
--		PopupWindow ${subclasses(PopupWindow)}
--
--		This class specializes a Window for the use by a
--		[[#tek.ui.class.popitem : PopItem]].
--
--	OVERRIDES::
--		- Class.new()
--		- Area:passMsg()
--		- Area:show()
--
-------------------------------------------------------------------------------

local ui = require "tek.ui".checkVersion(112)
local Window = ui.require("window", 33)

local max = math.max

local PopupWindow = Window.module("tek.ui.class.popupwindow", "tek.ui.class.window")
PopupWindow._VERSION = "PopupWindow 5.3"

-------------------------------------------------------------------------------
--	PopupWindow class:
-------------------------------------------------------------------------------

function PopupWindow.new(class, self)
	self.PopupBase = self.PopupBase or false
	self.BeginPopupTicks = 0
	self.DelayedBeginPopup = false
	self.DelayedEndPopup = false
	self.Width = self.Width or "auto"
	self.Height = self.Height or "auto"
	self.FullScreen = false
	return Window.new(class, self)
end

-------------------------------------------------------------------------------
--	show: overrides
-------------------------------------------------------------------------------

function PopupWindow:setup(app, window)
	Window.setup(self, app, window)
	-- determine width of menuitems in group:
	local maxw = 0
	local c = self:getChildren()
	for i = 1, #c do
		local e = c[i]
		local w = e:getAttr("menuitem-size")
		if w then
			maxw = max(maxw, w + 10) -- TODO: hardcoded padding
		end
	end
	for i = 1, #c do
		local e = c[i]
		-- align shortcut text (if present):
		local w = e:getAttr("menuitem-size")
		if w then
			e.TextRecords[2][5] = maxw
		end
	end
-- 	self.Window:addInputHandler(ui.MSG_INTERVAL, self, self.updateInterval)
end

-------------------------------------------------------------------------------
--	show: overrides
-------------------------------------------------------------------------------

function PopupWindow:show()
	Window.show(self)
	self.Window:addInputHandler(ui.MSG_INTERVAL, self, self.updateInterval)
end

-------------------------------------------------------------------------------
--	hide: overrides
-------------------------------------------------------------------------------

function PopupWindow:hide()
	self.Window:remInputHandler(ui.MSG_INTERVAL, self, self.updateInterval)
	Window.hide(self)
end

-------------------------------------------------------------------------------
--	passMsg: overrides
-------------------------------------------------------------------------------

function PopupWindow:passMsg(msg)
	if msg[2] == ui.MSG_MOUSEOVER then
		if msg[3] == 0 then
			if self.DelayedEndPopup then
				self:setHiliteElement(self.DelayedEndPopup)
				self.DelayedEndPopup = false
			end
		end
		-- do not pass control back to window msg handler:
		return false
	end
	return Window.passMsg(self, msg)
end

-------------------------------------------------------------------------------
--	updateInterval:
-------------------------------------------------------------------------------

function PopupWindow:updateInterval(msg)
	self.BeginPopupTicks = self.BeginPopupTicks - 1
	if self.BeginPopupTicks < 0 then
		if self.DelayedBeginPopup then
			if not self.DelayedBeginPopup.PopupWindow then
				self.DelayedBeginPopup:beginPopup()
			end
			self.DelayedBeginPopup = false
		elseif self.DelayedEndPopup then
			if self.DelayedEndPopup.PopupWindow then
				self.DelayedEndPopup:endPopup()
			end
			self.DelayedEndPopup = false
		end
	end
	return msg
end

return PopupWindow
