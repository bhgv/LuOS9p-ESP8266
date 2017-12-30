-------------------------------------------------------------------------------
--
--	tek.ui.class.poplist
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
--		PopList ${subclasses(PopList)}
--
--		This class is a specialization of a PopItem allowing the user
--		to choose an item from a list.
--
--	ATTRIBUTES::
--		- {{ListObject [IG]}} ([[#tek.class.list : List]])
--			List object
--		- {{SelectedLine [ISG]}} (number)
--			Number of the selected entry, or 0 if none is selected. Changing
--			this attribute invokes the PopList:onSelectLine() method.
--
--	IMPLEMENTS::
--		- PopList:onSelectLine() - Handler for the {{SelectedLine}} attribute
--		- PopList:setList() - Sets a new list object
--
--	OVERRIDES::
--		- Area:askMinMax()
--		- Area:cleanup()
--		- Area:draw()
--		- Class.new()
--		- Area:setup()
--
-------------------------------------------------------------------------------

local db = require "tek.lib.debug"
local List = require "tek.class.list"
local ui = require "tek.ui".checkVersion(112)

local Canvas = ui.require("canvas", 36)
local Widget = ui.require("widget", 25)
local Lister = ui.require("lister", 30)
local PopItem = ui.require("popitem", 22)
local ScrollGroup = ui.require("scrollgroup", 17)
local Text = ui.require("text", 28)

local assert = assert
local insert = table.insert
local max = math.max

local PopList = PopItem.module("tek.ui.class.poplist", "tek.ui.class.popitem")
PopList._VERSION = "PopList 13.6"

local FL_KEEPMINWIDTH = ui.FL_KEEPMINWIDTH

-------------------------------------------------------------------------------
--	Constants and class data:
-------------------------------------------------------------------------------

local ArrowImage = ui.Image:new
{
	{ 0x2000,0xa000, 0xe000,0xa000, 0x8000,0x4000 }, false, false, true,
	{ { 0x1000, 3, { 1, 2, 3 }, "menu-detail" } },
}

-------------------------------------------------------------------------------
--	PopLister:
-------------------------------------------------------------------------------

local PopLister = Lister:newClass()

function PopLister.new(class, self)
	self.Clicked = false
	return Lister.new(class, self)
end

function PopLister:passMsg(msg)
	local over, mx, my = self.Parent:getMouseOver(msg)
	if msg[2] == ui.MSG_MOUSEBUTTON then
		if msg[3] == 1 then
			self.Clicked = over
		elseif msg[3] == 2 then
			self.Clicked = false
		end
	end
	if msg[2] == ui.MSG_MOUSEMOVE or
		(msg[2] == ui.MSG_MOUSEBUTTON and msg[3] == 1) then
		if over then
			if not self.Clicked then
				local lnr = self:findLine(my)
				if lnr then
					if lnr ~= self.SelectedLine then
						self:setValue("CursorLine", lnr)
						self:setValue("SelectedLine", lnr, true)
					end
				end
			end
		end
	elseif msg[2] == ui.MSG_MOUSEBUTTON and msg[3] == 2 then
		if over then
			if not self.Active then
				-- need to emulate click:
				self:setValue("Active", true)
			end
			-- let it collapse:
			self:setValue("Active", false)
		end
	end
	return msg
end

function PopLister:handleInput(msg)
	if msg[2] == ui.MSG_KEYDOWN then
		if msg[3] == 13 and self.LastKey ~= 13 then
			self.Window:clickElement(self)
			self:setValue("SelectedLine", self.CursorLine, true)
			return false -- absorb
		end
	end
	return Lister.handleInput(self, msg)
end

function PopLister:onActivate()
	Lister.onActivate(self)
	local active = self.Active
	if active == false and self.Window then
		local lnr = self.CursorLine
		local entry = self:getItem(lnr)
		if entry then
			local popitem = self.Window.PopupBase
			popitem:setValue("SelectedLine", lnr, true)
			popitem:setValue("Text", entry[1][1])
		end
		-- needed to unregister input-handler:
		self:setValue("Focus", false)
		self.Window:finishPopup()
	end
end

function PopLister:askMinMax(m1, m2, m3, m4)
	m1 = m1 + self:getAttr("MinWidth")
	m2 = m2 + self.CanvasHeight
	m3 = ui.HUGE
	m4 = m4 + self.CanvasHeight
	return Widget.askMinMax(self, m1, m2, m3, m4)
end

-------------------------------------------------------------------------------
--	PopList:
-------------------------------------------------------------------------------

function PopList.addClassNotifications(proto)
	PopList.addNotify(proto, "SelectedLine", 
		ui.NOTIFY_ALWAYS, { ui.NOTIFY_SELF, "onSelectLine" })
	return PopItem.addClassNotifications(proto)
end

PopList.ClassNotifications = PopList.addClassNotifications { Notifications = { } }

function PopList.new(class, self)
	self = self or { }
	self.ListObject = self.ListObject or List:new()
	self.Lister = PopLister:new { ListObject = self.ListObject,
		Class = self.Class, Style = self.Style }
	self.Children =
	{
		ScrollGroup:new
		{
			VSliderMode = self.ListHeight and "auto" or "off",
			Child = Canvas:new
			{
				Height = self.ListHeight,
				Class = "poplist-canvas",
				KeepMinWidth = true,
				KeepMinHeight = not self.ListHeight,
				AutoWidth = true,
				AutoHeight = true,
				Child = self.Lister,
			}
		}
	}
	
	self.ImageRect = { 0, 0, 0, 0 }
	self.Image = self.Image or ArrowImage
	self.Width = self.Width or "fill"
	self.SelectedLine = self.SelectedLine or 0
	self.ListHeight = self.ListHeight or false
	
	return PopItem.new(class, self)
end

function PopList:show()
	PopList.onSelectLine(self)
	PopItem.show(self)
end

function PopList:askMinMax(m1, m2, m3, m4)
	local lo = self.ListObject
	if lo and not self:checkFlags(FL_KEEPMINWIDTH) then
		local tr = { }
		local props = self.Lister.Properties
		local font = self.Application.Display:openFont(props["font"])
		for lnr = 1, lo:getN() do
			local entry = lo:getItem(lnr)
			local t = self:newTextRecord(entry[1][1], font, props["text-align"],
				props["vertical-align"], 0, 0, 0, 0)
			insert(tr, t)
		end
		local lw = self:getTextSize(tr) -- max width of items in list
		local w, h = self:getTextSize() -- width/height of our own text
		w = max(w, lw) - w -- minus own width, as it gets added in super class
		m1 = m1 + w -- + iw
		m3 = m3 + w -- + iw
	end
	return PopItem.askMinMax(self, m1, m2, m3, m4)
end

function PopList:beginPopup()
	PopItem.beginPopup(self)
-- 	self.Lister:setValue("SelectedLine", self.SelectedLine, false)
	self.Lister.Clicked = false
	self.Lister:setValue("Focus", true)
end

-------------------------------------------------------------------------------
--	onSelectLine(): This method is invoked when the {{SelectedLine}}
--	attribute has changed.
-------------------------------------------------------------------------------

function PopList:onSelectLine()
	local lnr = self.SelectedLine
	local entry = self.Lister:getItem(lnr)
	if entry then
		self:setValue("Text", entry[1][1])
	end
end

-------------------------------------------------------------------------------
--	setList(listobject): Sets a new [[#tek.class.list : List]]
--	object.
-------------------------------------------------------------------------------

function PopList:setList(listobject)
	assert(not listobject or listobject:instanceOf(List))
	self.ListObject = listobject
	self.Lister:setList(listobject)
end

-------------------------------------------------------------------------------
--	decodeProperties: overrides
-------------------------------------------------------------------------------

function PopList:decodeProperties(p)
	self.Lister:decodeProperties(p)
	PopItem.decodeProperties(self, p)
end

return PopList
