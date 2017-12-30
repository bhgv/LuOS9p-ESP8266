-------------------------------------------------------------------------------
--
--	tek.ui.class.popitem
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
--		PopItem ${subclasses(PopItem)}
--
--		This class provides an anchorage for popups. This also works
--		recursively, i.e. elements of the PopItem class may contain other
--		PopItems as their children. The most notable child class of the
--		PopItem is the [[#tek.ui.class.menuitem : MenuItem]].
--
--	ATTRIBUTES::
--		- {{Children [I]}} (table)
--			Array of child objects - will be connected to the application
--			while the popup is open.
--		- {{Shortcut [IG]}} (string)
--			Keyboard shortcut for the object; unlike
--			[[#tek.ui.class.widget : Widget]].KeyCode, this shortcut is
--			also enabled while the object is invisible. By convention, only
--			combinations with a qualifier should be used here, e.g.
--			{{"Alt+C"}}, {{"Shift+Ctrl+Q"}}. Qualifiers are separated by
--			{{"+"}} and must precede the key. Valid qualifiers are:
--				- {{"Alt"}}, {{"LAlt"}}, {{"RAlt"}}
--				- {{"Shift"}}, {{"LShift"}}, {{"RShift"}}
--				- {{"Ctrl"}}, {{"LCtrl"}}, {{"RCtrl"}}
--				- {{"IgnoreCase"}} - pseudo-qualifier; ignores the Shift key
--				- {{"IgnoreAltShift"}} - pseudo-qualifier, ignores the Shift
--				and Alt keys
--			
--			Alias names for keys are
--				- {{"F1"}} ... {{"F12"}} (function keys),
--				- {{"Left"}}, {{"Right"}}, {{"Up"}}, {{"Down"}} (cursor keys)
--				- {{"BckSpc"}}, {{"Tab"}}, {{"Esc"}}, {{"Insert"}},
--				{{"Overwrite"}}, {{"PageUp"}}, {{"PageDown"}}, {{"Pos1"}}, 
--				{{"End"}}, {{"Print"}}, {{"Scroll"}}, and {{"Pause"}}}}.
--
--	OVERRIDES::
--		- Object.addClassNotifications()
--		- Element:cleanup()
--		- Element:getAttr()
--		- Class.new()
--		- Widget:onClick()
--		- Area:passMsg()
--		- Element:setup()
--		- Area:show()
--
-------------------------------------------------------------------------------

local db = require "tek.lib.debug"
local ui = require "tek.ui".checkVersion(112)
local PopupWindow = ui.require("popupwindow", 5)
local Text = ui.require("text", 28)
ui.require("widget", 26)
local floor = math.floor
local unpack = unpack or table.unpack

local PopItem = Text.module("tek.ui.class.popitem", "tek.ui.class.text")
PopItem._VERSION = "PopItem 26.2"

-------------------------------------------------------------------------------
--	Constants and class data:
-------------------------------------------------------------------------------

local DEF_POPUPFADEINDELAY = 6
local DEF_POPUPFADEOUTDELAY = 10

local FL_POPITEM = ui.FL_POPITEM
local FL_ACTIVATERMB = ui.FL_ACTIVATERMB

local NOTIFY_ACTIVE = { ui.NOTIFY_SELF, "onActivate" }

-------------------------------------------------------------------------------
--	addClassNotifications: overrides
-------------------------------------------------------------------------------

function PopItem.addClassNotifications(proto)
	PopItem.addNotify(proto, "Hilite", ui.NOTIFY_ALWAYS,
		{ ui.NOTIFY_SELF, "doSubMenu" })
	PopItem.addNotify(proto, "Selected", true,
		{ ui.NOTIFY_SELF, "selectPopup" })
	PopItem.addNotify(proto, "Selected", false,
		{ ui.NOTIFY_SELF, "unselectPopup" })
	return Text.addClassNotifications(proto)
end

PopItem.ClassNotifications = 
	PopItem.addClassNotifications { Notifications = { } }

-------------------------------------------------------------------------------
--	Class implementation:
-------------------------------------------------------------------------------

function PopItem.new(class, self)
	self = self or { }
	self.Image = self.Image or false
	self.ImageRect = self.ImageRect or false
	self.PopupBase = false
	self.PopupWindow = false
	self.DelayedBeginPopup = false
	self.DelayedEndPopup = false
	if self.KeyCode == nil then
		self.KeyCode = true
	end
	self.ShiftX = 0
	self.ShiftY = 0
	if self.Children then
		self.Mode = self.Mode or "toggle"
		self.FocusNotification = { self, "unselectPopup" }
	else
		self.Children = false
		self.Mode = self.Mode or "button"
	end
	self.Shortcut = self.Shortcut or false
	return Text.new(class, self)
end

-------------------------------------------------------------------------------
--	connect: overrides
-------------------------------------------------------------------------------

function PopItem:connect(parent)
	if not self.PopupBase then
		-- this is a root item of a popup tree:
		self:addStyleClass("popup-root")
-- 		self:addStyleClass("button")
	end
	return Text.connect(self, parent)
end

-------------------------------------------------------------------------------
--	setup: overrides
-------------------------------------------------------------------------------

function PopItem:setup(app, window)
	Text.setup(self, app, window)
	if window:getClass() ~= PopupWindow then
		self:connectPopItems(app, window)
	end
end

-------------------------------------------------------------------------------
--	cleanup: overrides
-------------------------------------------------------------------------------

function PopItem:cleanup()
	local app, window = self.Application, self.Window
	if self.Window:getClass() ~= PopupWindow then
		self:disconnectPopItems(self.Window)
	end
	Text.cleanup(self)
	-- restore application and window, as they are needed in
	-- popitems' notification handlers even when they are not visible:
	self.Application, self.Window = app, window
end

-------------------------------------------------------------------------------
--	hide: overrides
-------------------------------------------------------------------------------

function PopItem:hide()
	self:unselectPopup()
	Text.hide(self)
end

-------------------------------------------------------------------------------
--	askMinMax: overrides
-------------------------------------------------------------------------------

function PopItem:askMinMax(m1, m2, m3, m4)
	local n1, n2, n3, n4 = Text.askMinMax(self, m1, m2, m3, m4)
	if self.Image then
		local p1, p2, p3, p4 = self:getPadding()
		local ma1, ma2, ma3, ma4 = self:getMargin()
		local iw = n1 - m1 - p3 - p1 - ma3 - ma1 + 1
		local ih = n2 - m2 - p4 - p2 - ma4 - ma2 + 1
		iw, ih = self.Application.Display:fitMinAspect(iw, ih, 1, 1, 0)
		n1 = n1 + iw
		n3 = n3 + ih
	end
	return n1, n2, n3, n4
end

-------------------------------------------------------------------------------
--	layout: overrides
-------------------------------------------------------------------------------

function PopItem:layout(x0, y0, x1, y1, markdamage)
	if Text.layout(self, x0, y0, x1, y1, markdamage) then
		if self.Image then
			local r1, r2, r3, r4 = self:getRect()
			local p1, p2, p3, p4 = self:getPadding()
			local iw = r3 - r1 - p3 - p1 + 1
			local ih = r4 - r2 - p4 - p2 + 1
			iw, ih = self.Application.Display:fitMinAspect(iw, ih, 1, 1, 0)
			-- use half the padding that was granted for the right edge:
			local x = r3 - floor(p3 / 2) - iw
			local y = r2 + p2
			local i = self.ImageRect
			i[1], i[2], i[3], i[4] = x, y, x + iw - 1, y + ih - 1
		end
		return true
	end
end

-------------------------------------------------------------------------------
--	draw: overrides
-------------------------------------------------------------------------------

function PopItem:draw()
	local d = self.Window.Drawable
	self.ShiftX, self.ShiftY = d:setShift()
	if Text.draw(self) then
		local i = self.Image
		if i then
			local x0, y0, x1, y1 = unpack(self.ImageRect)
			i:draw(d, x0, y0, x1, y1, self.FGPen)
		end
		return true
	end
end

-------------------------------------------------------------------------------
--	calcPopup:
-------------------------------------------------------------------------------

function PopItem:calcPopup()
	local _, _, x, y = self.Window.Drawable:getAttrs()
	local w
	local r1, r2, r3, r4 = self:getRect()
	local sx, sy = self.ShiftX, self.ShiftY
	if self.PopupBase then
		x =	x + r3 + sx
		y = y + r2 + sy
	else
		x =	x + r1 + sx
		y = y + r4 + sy
		w = r3 - r1 + 1
	end
	return x, y, w
end

-------------------------------------------------------------------------------
--	beginPopup: overrides
-------------------------------------------------------------------------------

function PopItem:beginPopup(baseitem)
	if baseitem then
		Text.beginPopup(self, baseitem)
		self.PopupBase = baseitem
		return
	end
	
	local children = self.Children
	if not children then
		db.warn("element has no children")
		return
	end

	local winx, winy, winw, winh = self:calcPopup()

	if self.Window.ActivePopup then
		db.warn("Killed active popup")
		self.Window.ActivePopup:endPopup()
	end

	-- prepare children for being used in a popup window:
	for i = 1, #children do
		local c = children[i]
		c:beginPopup(self.PopupBase or self)
		c:setFlags(FL_POPITEM)
	end

	self.PopupWindow = PopupWindow:new
	{
		-- window in which the popup cascade is rooted:
		PopupRootWindow = self.Window.PopupRootWindow or self.Window,
		-- item in which this popup window is rooted:
		PopupBase = self.PopupBase or self,
		Children = self.Children,
		Orientation = "vertical",
		Left = winx,
		Top = winy,
		Width = winw,
		Height = winh,
		MaxWidth = winw,
		MaxHeight = winh,
		Borderless = true,
		PopupWindow = true,
	}

	local app = self.Application

	-- connect children recursively:
	app.connect(self.PopupWindow)

	self.Window.ActivePopup = self

	app:addMember(self.PopupWindow)

	self.PopupWindow:setValue("Status", "show")

	self.Window:addNotify("Status", "hide", self.FocusNotification)
	self.Window:addNotify("WindowFocus", ui.NOTIFY_ALWAYS, self.FocusNotification)

end

-------------------------------------------------------------------------------
--	endPopup:
-------------------------------------------------------------------------------

function PopItem:endPopup()
	if not self.Children then
		db.warn("element has no children")
		return
	end
	local base = self.PopupBase or self
	self:setValue("Focus", false)

	base:setValue("Focus", true)
	
	self:setState()
	self.Window:remNotify("WindowFocus", ui.NOTIFY_ALWAYS,
		self.FocusNotification)
	self.Window:remNotify("Status", "hide", self.FocusNotification)
	
	if self.PopupWindow then
		self.PopupWindow:setValue("Status", "hide")
		self.Application:remMember(self.PopupWindow)
	end
	self.Window.ActivePopup = false
	self.PopupWindow = false
	self:setValue("Selected", false)
end

-------------------------------------------------------------------------------
--	unselectPopup:
-------------------------------------------------------------------------------

function PopItem:unselectPopup()
	if self.Children and self.PopupWindow then
		self:endPopup()
		self.Window:setActiveElement()
	end
end

function PopItem:passMsg(msg)
	if msg[2] == ui.MSG_MOUSEBUTTON then
		local armb = self:checkFlags(FL_ACTIVATERMB)
		if msg[3] == 1 or (armb and msg[3] == 4) then -- leftdown:
			if self.PopupWindow and self.Window.ActiveElement ~= self and
				not self.PopupBase and self.Window.HoverElement == self then
				self:endPopup()
				-- swallow event, don't let ourselves get reactivated:
				return false
			end
		elseif msg[3] == 2 or (armb and msg[3] == 8) then -- leftup:
			if self.PopupWindow and self.Window.HoverElement ~= self and
				not self.Disabled then
				self:endPopup()
			end
		end
	end
	return Text.passMsg(self, msg)
end


function PopItem:doSubMenu()
	if self.Children then
		-- check if not the baseitem:
		if self.PopupBase then
			self.Window.DelayedBeginPopup = false
			local val = self.Hilite
			if val == true then
				if not self.PopupWindow then
					db.trace("Begin beginPopup delay")
					self.Window.BeginPopupTicks = DEF_POPUPFADEINDELAY
					self.Window.DelayedBeginPopup = self
				elseif self.Window.DelayedEndPopup == self then
					self.Window.DelayedEndPopup = false
				end
			elseif val == false and self.PopupWindow then
				db.trace("Begin endPopup delay")
				self.Window.BeginPopupTicks = DEF_POPUPFADEOUTDELAY
				self.Window.DelayedEndPopup = self
			end
		end
	end
end

-------------------------------------------------------------------------------
--	selectPopup:
-------------------------------------------------------------------------------

function PopItem:selectPopup()
	if self.Children then
		if not self.PopupWindow then
			self:beginPopup()
		end
		if self.PopupBase then
			self.Selected = false
			self:setFlags(ui.FL_REDRAW)
		end
	end
end

-------------------------------------------------------------------------------
--	onClick: overrides
-------------------------------------------------------------------------------

function PopItem:onClick()
	if self.PopupBase then
		-- unselect base item, causing the tree to collapse:
		self.PopupBase:setValue("Selected", false)
	end
end

-------------------------------------------------------------------------------
--	connectPopItems:
-------------------------------------------------------------------------------

function PopItem:connectPopItems(app, window)
	if self:instanceOf(PopItem) then
		db.info("adding %s", self:getClassName())
		local c = self:getChildren("init")
		if c then
			for i = 1, #c do
				-- c[i]:addStyleClass("popup-child")
				PopItem.connectPopItems(c[i], app, window)
			end
		else
			if self.Shortcut then
				window:addKeyShortcut(self.Shortcut, self)
			end
		end
	elseif self:getAttr("popup-collapse") then
		-- special treatment so that clicking the element collapses the popup:
		self:addNotify("Active", ui.NOTIFY_ALWAYS, ui.NOTIFY_ACTIVE)
	end
	self:setFlags(FL_POPITEM)
end

-------------------------------------------------------------------------------
--	disconnectPopItems:
-------------------------------------------------------------------------------

function PopItem:disconnectPopItems(window)
	self:checkClearFlags(FL_POPITEM)
	if self:instanceOf(PopItem) then
		db.info("removing popitem %s", self:getClassName())
		local c = self:getChildren("init")
		if c then
			for i = 1, #c do
				PopItem.disconnectPopItems(c[i], window)
			end
		else
			if self.Shortcut then
				window:remKeyShortcut(self.Shortcut, self)
			end
		end
	elseif self:getAttr("popup-collapse") then
		self:remNotify("Active", ui.NOTIFY_ALWAYS, ui.NOTIFY_ACTIVE)
	end
end

-------------------------------------------------------------------------------
--	getAttr: overrides
-------------------------------------------------------------------------------

function PopItem:getAttr(attr, ...)
	if attr == "menuitem-size" then
		-- Do we have a text record for a shortcut? If so, return its size
		return self.TextRecords[2] and self:getTextSize()
	end
	return Text.getAttr(self, attr, ...)
end

-------------------------------------------------------------------------------
--	getChildren: overrides
-------------------------------------------------------------------------------

function PopItem:getChildren(mode)
	return mode == "init" and self.Children
end

return PopItem
