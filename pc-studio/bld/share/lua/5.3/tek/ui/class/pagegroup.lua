-------------------------------------------------------------------------------
--
--	tek.ui.class.pagegroup
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
--		PageGroup ${subclasses(PageGroup)}
--
--		Implements a group whose children are layouted in individual
--		pages.
--
--	ATTRIBUTES::
--		- {{PageCaptions [IG]}} (table)
--			An array of strings containing captions for each page in
--			the group. If '''false''', no page captions will be displayed.
--			[Default: '''false''']
--		- {{PageNumber [ISG]}} (number)
--			Number of the page that is initially selected. [Default: 1]
--			Setting this attribute invokes the PageGroup:onSetPageNumber()
--			method.
--
--	IMPLEMENTS::
--		- PageGroup:disablePage() - Enables a Page
--		- PageGroup:onSetPageNumber() - Handler for {{PageNumber}}
--
--	OVERRIDES::
--		- Element:cleanup()
--		- Class.new()
--		- Element:setup()
--
-------------------------------------------------------------------------------

local db = require "tek.lib.debug"
local ui = require "tek.ui".checkVersion(112)

local Frame = ui.require("frame", 21)
local Widget = ui.require("widget", 25)
local Group = ui.require("group", 31)
local Region = ui.loadLibrary("region", 10)
local Text = ui.require("text", 28)

local assert = assert
local insert = table.insert
local max = math.max
local min = math.min
local tonumber = tonumber
local tostring = tostring
local type = type

local PageGroup = Group.module("tek.ui.class.pagegroup", "tek.ui.class.group")
PageGroup._VERSION = "PageGroup 19.7"

-------------------------------------------------------------------------------
--	PageContainerGroup:
-------------------------------------------------------------------------------

local PageContainerGroup = Group:newClass { _NAME = "_page-container" }

function PageContainerGroup.new(class, self)
	self.PageElement = self.PageElement or false
	self.PageNumber = self.PageNumber or 1
	return Group.new(class, self)
end

-------------------------------------------------------------------------------
--	show: overrides
-------------------------------------------------------------------------------

function PageContainerGroup:show()
	Widget.show(self)
	self.PageElement:show()
end

-------------------------------------------------------------------------------
--	hide: overrides
-------------------------------------------------------------------------------

function PageContainerGroup:hide()
	self.PageElement:hide()
	Widget.hide(self)
end

-------------------------------------------------------------------------------
--	damage: overrides
-------------------------------------------------------------------------------

function PageContainerGroup:damage(r1, r2, r3, r4)
	Widget.damage(self, r1, r2, r3, r4)
	local f = self.FreeRegion
	if f and f:checkIntersect(r1, r2, r3, r4) then
		self:setFlags(ui.FL_REDRAW)
	end
	self.PageElement:damage(r1, r2, r3, r4)
end

-------------------------------------------------------------------------------
--	getByXY: overrides
-------------------------------------------------------------------------------

function PageContainerGroup:getByXY(x, y)
	return self.PageElement:getByXY(x, y)
end

-------------------------------------------------------------------------------
--	askMinMax: overrides
-------------------------------------------------------------------------------

function PageContainerGroup:askMinMax(m1, m2, m3, m4)
	local c = self.Children
	self.Children = { self.PageElement }
	m1, m2, m3, m4 = Group.askMinMax(self, m1, m2, m3, m4)
	self.Children = c
	return m1, m2, m3, m4
end

-------------------------------------------------------------------------------
--	punch: overrides
-------------------------------------------------------------------------------

function PageContainerGroup:punch(region)
	Group.punch(self, region)
	self.PageElement:punch(region)
end

-------------------------------------------------------------------------------
--	layout: overrides
-------------------------------------------------------------------------------

function PageContainerGroup:layout(r1, r2, r3, r4, markdamage)
	local c = self.Children
	self.Children = { self.PageElement }
	local res = Group.layout(self, r1, r2, r3, r4, markdamage)
	self.Children = c
	return res
end

-------------------------------------------------------------------------------
--	onSetDisable: overrides
-------------------------------------------------------------------------------

function PageContainerGroup:onDisable()
	return self.PageElement:setValue("Disabled", self.Disabled)
end

-------------------------------------------------------------------------------
--	passMsg: overrides
-------------------------------------------------------------------------------

function PageContainerGroup:passMsg(msg)
	return self.PageElement:passMsg(msg)
end

-------------------------------------------------------------------------------
--	getParent: overrides
-------------------------------------------------------------------------------

function PageContainerGroup:getParent()
	return self.Parent
end

-------------------------------------------------------------------------------
--	getGroup: overrides
-------------------------------------------------------------------------------

function PageContainerGroup:getGroup(parent)
	return parent and self.Parent or self
end

-------------------------------------------------------------------------------
--	getNext: overrides
-------------------------------------------------------------------------------

function PageContainerGroup:getNext(mode)
	return self:getParent():getNext(mode)
end

-------------------------------------------------------------------------------
--	getPrev: overrides
-------------------------------------------------------------------------------

function PageContainerGroup:getPrev(mode)
	return self:getParent():getPrev(mode)
end

-------------------------------------------------------------------------------
--	getChildren: overrides
-------------------------------------------------------------------------------

function PageContainerGroup:getChildren(mode)
	return mode == "init" and self.Children or { self.PageElement }
end

-------------------------------------------------------------------------------
--	changeTab:
-------------------------------------------------------------------------------

function PageContainerGroup:changeTab(pagebuttons, tabnr)
	if self.Children[tabnr] then
		if pagebuttons and pagebuttons[self.PageNumber + 1] then
			pagebuttons[self.PageNumber + 1]:setValue("Selected", false)
		end
		self.PageNumber = tabnr
		self.PageElement:hide()
		self.PageElement = self.Children[tabnr]
		local d = self.Window.Drawable
		if d then
			self.PageElement:show(d)
			self:getParent():rethinkLayout(2, true)
		else
			db.error("pagegroup not connected to display")
		end
	end
end

-------------------------------------------------------------------------------
--	addClassNotifications: overrides
-------------------------------------------------------------------------------

function PageGroup.addClassNotifications(proto)
	PageGroup.addNotify(proto, "PageNumber", ui.NOTIFY_ALWAYS,
		{ ui.NOTIFY_SELF, "onSetPageNumber", ui.NOTIFY_VALUE,
		ui.NOTIFY_OLDVALUE })
	return Group.addClassNotifications(proto)
end

PageGroup.ClassNotifications = 
	PageGroup.addClassNotifications { Notifications = { } }

-------------------------------------------------------------------------------
--	new: overrides
-------------------------------------------------------------------------------

function PageGroup.new(class, self)
	self = self or { }
	self.Orientation = "vertical"
	self.PageCaptions = self.PageCaptions or false
	self.TabButtons = false
	
	local layout = self.Layout
	self.Layout = false
	local children = self.Children or { }
	self.Children = false
	local pagenumber = self.PageNumber or 1
	self.PageNumber = pagenumber
	pagenumber = type(pagenumber) == "number" and pagenumber or 1
	pagenumber = max(1, min(pagenumber, #children))

	self = Group.new(class, self)
	
	local pagegroup = PageContainerGroup:new {
		PageElement = children[pagenumber] or ui.Area:new { },
		PageNumber = pagenumber,
		Width = self.Width,
		Height = self.Height,
		Layout = layout,
		Children = children
	}
	
	local pagebuttons = false
	if self.PageCaptions then
		pagegroup.Class = "page-container"
		pagebuttons = { 
			ui.Frame:new {
				Class = "page-button-fill",
				Style = "border-left-width: 0",
				MinWidth = 3,
				MaxWidth = 3,
				Width = 3,
				Height = "fill"
			}
		}
		if #children == 0 then
			insert(pagebuttons, ui.Text:new {
				Class = "page-button",
				Mode = "inert",
				Width = "auto"
			})
		else
			for i = 1, #children do
				local pc = self.PageCaptions[i]
				if type(pc) == "table" then
					-- is element already, ok
				else
					local text = pc or tostring(i)
					pc = ui.Text:new
					{
						Class = "page-button",
						Mode = "touch",
						Width = "auto",
						Text = text,
						KeyCode = true,
						onPress = function(pagebutton)
							if pagebutton.Pressed then
								self:setValue("PageNumber", i)
							end
						end
					}
				end
				insert(pagebuttons, pc)
			end
		end
		insert(pagebuttons, ui.Frame:new {
			Class = "page-button-fill",
			Height = "fill"
		})
		self:addMember(Group:new {
			Class = "page-button-group",
			Width = "fill",
			Height = "auto",
			Children = pagebuttons
		})
		self.TabButtons = pagebuttons
		if pagebuttons[pagenumber + 1] then
			pagebuttons[pagenumber + 1]:setValue("Selected", true)
		end
	end

	self:addMember(pagegroup)

	return self
end

-------------------------------------------------------------------------------
--	onSetPageNumber(): This method is invoked when the element's
--	{{PageNumber}} attribute has changed.
-------------------------------------------------------------------------------

function PageGroup:onSetPageNumber()
	local n = tonumber(self.PageNumber)
	local b = self.TabButtons
	if b then
		if b[n + 1] then
			b[n + 1]:setValue("Selected", true)
		end
		self.Children[2]:changeTab(self.TabButtons, n)
	else
		self.Children[1]:changeTab(self.TabButtons, n)
	end
end

-------------------------------------------------------------------------------
--	disablePage(pagenum, onoff): This function allows to disable or re-enable
--	a page button identified by {{pagenum}}.
-------------------------------------------------------------------------------

function PageGroup:disablePage(num, onoff)
	local numc = #self.Children[2].Children
	if num >= 1 and num <= numc then
		self.TabButtons[num + 1]:setValue("Disabled", onoff or false)
	end
end

return PageGroup
