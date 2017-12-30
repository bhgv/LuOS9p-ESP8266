-------------------------------------------------------------------------------
--
--	tek.ui.class.listview
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
--		ListView ${subclasses(ListView)}
--
--		This class implements a [[#tek.ui.class.group : Group]] containing
--		a [[#tek.ui.class.scrollgroup : ScrollGroup]] and optionally a
--		group of column headers; its main purpose is to automate the somewhat
--		complicated setup of multi-column lists with headers, but it can be
--		used for single-column lists and lists without column headers as well.
--
--	ATTRIBUTES::
--		- {{Headers [I]}} (table)
--			An array of strings containing the captions of column headers.
--			[Default: unspecified]
--		- {{HSliderMode [I]}} (string)
--			This attribute is passed on the
--			[[#tek.ui.class.scrollgroup : ScrollGroup]] - see there.
--		- {{VSliderMode [I]}} (string)
--			This attribute is passed on the
--			[[#tek.ui.class.scrollgroup : ScrollGroup]] - see there.
--
-------------------------------------------------------------------------------

local List = require "tek.class.list"
local ui = require "tek.ui".checkVersion(112)

local Canvas = ui.require("canvas", 30)
local Widget = ui.require("widget", 25)
local Group = ui.require("group", 31)
local Lister = ui.require("lister", 30)
local ScrollBar = ui.require("scrollbar", 13)
local ScrollGroup = ui.require("scrollgroup", 17)

local Text = ui.Text
local unpack = unpack

local ListView = Group.module("tek.ui.class.listview", "tek.ui.class.group")
ListView._VERSION = "ListView 6.4"

-------------------------------------------------------------------------------
--	HeadItem:
-------------------------------------------------------------------------------

local HeadItem = Text:newClass { _NAME = "_listview-headitem" }

function HeadItem.new(class, self)
	self = self or { }
	self.Mode = "inert"
	self.Width = "auto"
	return Text.new(class, self)
end

function HeadItem:askMinMax(m1, m2, m3, m4)
	local w, h = self:getTextSize()
	return Widget.askMinMax(self, m1 + w, m2 + h, m3 + w, m4 + h)
end

-------------------------------------------------------------------------------

local LVScrollGroup = ScrollGroup:newClass { _NAME = "_lvscrollgroup" }

function LVScrollGroup:onSetCanvasHeight(h)
	ScrollGroup.onSetCanvasHeight(self, h)
	local c = self.Child
	local _, r2, _, r4 = c:getRect()
	if r2 then
		local sh = r4 - r2 + 1
		local en = self.VSliderGroupMode == "on" or
			self.VSliderGroupMode == "auto" and (sh < h)
		if en ~= self.VSliderGroupEnabled then
			if en then
				self.ExternalGroup:addMember(self.VSliderGroup, 2)
			else
				self.ExternalGroup:remMember(self.VSliderGroup, 2)
			end
			self.VSliderGroupEnabled = en
		end
	end
end

local LVCanvas = Canvas:newClass { _NAME = "_lvcanvas" }

function LVCanvas:passMsg(msg)
	if msg[2] == ui.MSG_MOUSEBUTTON then
		if msg[3] == 64 or msg[3] == 128 then
			-- check if mousewheel over ourselves:
			local mx, my = self:getMsgFields(msg, "mousexy")
			if self:checkArea(mx, my) then
				return self.Child:passMsg(msg)
			end
		end
	end
	return Canvas.passMsg(self, msg)
end

-------------------------------------------------------------------------------
--	ListView:
-------------------------------------------------------------------------------

function ListView.new(class, self)
	self = self or { }

	self.Child = self.Child or Lister:new()
	self.HeaderGroup = self.HeaderGroup or false
	self.Headers = self.Headers or false
	self.HSliderMode = self.HSliderMode or "on"
	self.VSliderGroup = false
	self.VSliderMode = self.VSliderMode or "on"

	if self.Headers and not self.HeaderGroup then
		local c = { }
		local headers = self.Headers
		for i = 1, #headers do
			c[i] = HeadItem:new { Text = headers[i] }
		end
		self.HeaderGroup = Group:new { Width = "fill", Children = c }
	end

	if self.HeaderGroup then
		self.VSliderGroup = ScrollBar:new { Orientation = "vertical", Min = 0 }
		self.Child.HeaderGroup = self.HeaderGroup
		self.Children =
		{
			ScrollGroup:new
			{
				VSliderMode = "off",
				HSliderMode = self.HSliderMode,
				Child = LVCanvas:new
				{
					AutoHeight = true,
					AutoWidth = true,
					Child = Group:new
					{
						Orientation = "vertical",
						Children =
						{
							self.HeaderGroup,
							LVScrollGroup:new
							{
								-- our own sliders are always off:
								VSliderMode = "off",
								HSliderMode = "off",

								-- data to service the external slider:
								VSliderGroupMode = self.VSliderMode,
								VSliderGroup = self.VSliderGroup,
								VSliderGroupEnabled = true,
								ExternalGroup = self,

								Child = Canvas:new
								{
									Child = self.Child
								}
							}
						}
					}
				}
			},
			self.VSliderGroup
		}
		-- point element that determines lister alignment to outer canvas:
		self.Child.AlignElement = self.Children[1].Child
	else
		self.Children =
		{
			ScrollGroup:new
			{
				VSliderMode = self.VSliderMode,
				HSliderMode = self.HSliderMode,
				Child = Canvas:new
				{
					MinHeight = self.MinHeight,
					AutoWidth = true,
					Child = self.Child
				}
			}
		}
	end

	return Group.new(class, self)
end

return ListView
