-------------------------------------------------------------------------------
--
--	tek.ui.class.scrollbar
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
--		ScrollBar ${subclasses(ScrollBar)}
--
--		Implements a group containing a
--		[[#tek.ui.class.slider : Slider]] and arrow buttons.
--
--	ATTRIBUTES::
--		- {{AcceptFocus [IG]}} (boolean)
--			If '''false''', the elements inside the scrollbar (slider, arrows)
--			abstain from receiving the input focus, which means that they can
--			only be operated with the mouse. Default: '''true'''
--		- {{Max [ISG]}} (number)
--			The maximum value the slider can accept. Setting this value
--			invokes the ScrollBar:onSetMax() method.
--		- {{Min [ISG]}} (number)
--			The minimum value the slider can accept. Setting this value
--			invokes the ScrollBar:onSetMin() method.
--		- {{Orientation [IG]}} (string)
--			The orientation of the scrollbar, which can be "horizontal"
--			or "vertical"
--		- {{Range [ISG]}} (number)
--			The range of the slider, i.e. the size it represents. Setting
--			this value invokes the ScrollBar:onSetRange() method.
--		- {{Step [IG]}} (boolean or number)
--			See [[#tek.ui.class.slider]].
--		- {{Value [ISG]}} (number)
--			The value of the slider. Setting this value invokes the
--			ScrollBar:onSetValue() method.
--
--	IMPLEMENTS::
--		- ScrollBar:onSetMax() - Handler for the {{Max}} attribute
--		- ScrollBar:onSetMin() - Handler for the {{Min}} attribute
--		- ScrollBar:onSetRange() - Handler for the {{Range}} attribute
--		- ScrollBar:onSetValue() - Handler for the {{Value}} attribute
--
--	OVERRIDES::
--		- Object.addClassNotifications()
--		- Element:cleanup()
--		- Class.new()
--		- Element:setup()
--
-------------------------------------------------------------------------------

local ui = require "tek.ui".checkVersion(112)
local Group = ui.require("group", 31)
local ImageWidget = ui.require("imagewidget", 13)
local Slider = ui.require("slider", 24)

local max = math.max
local min = math.min

local ScrollBar = Group.module("tek.ui.class.scrollbar", "tek.ui.class.group")
ScrollBar._VERSION = "ScrollBar 15.4"

-------------------------------------------------------------------------------
--	Constants & Class data:
-------------------------------------------------------------------------------

local ArrowUpImage = ui.getStockImage("arrowup")
local ArrowDownImage = ui.getStockImage("arrowdown")
local ArrowLeftImage = ui.getStockImage("arrowleft")
local ArrowRightImage = ui.getStockImage("arrowright")

-------------------------------------------------------------------------------
--	ArrowButton:
-------------------------------------------------------------------------------

local ArrowButton = ImageWidget:newClass { _NAME = "_scrollbar-arrow" }

function ArrowButton.new(class, self)
	self.Width = "auto"
	self.Height = "auto"
	self.Mode = "button"
	self.Direction = self.Direction or 1
	return ImageWidget.new(class, self)
end

function ArrowButton:checkFocus()
	if self.ScrollBar.AcceptFocus then
		return ImageWidget.checkFocus(self)
	end
	return false
end

function ArrowButton:onClick()
	if self.Direction > 0 then
		self.Slider:increase()
	else
		self.Slider:decrease()
	end
end

function ArrowButton:onHold()
	ImageWidget.onHold(self)
	if self.Hold then
		self:onClick()
	end
end

-------------------------------------------------------------------------------
--	Slider:
-------------------------------------------------------------------------------

local SBSlider = Slider:newClass { _NAME = "_scrollbar-slider" }

function SBSlider:checkFocus()
	if self.ScrollBar.AcceptFocus then
		return Slider.checkFocus(self)
	end
	return false
end

function SBSlider:onSetValue()
	Slider.onSetValue(self)
	self.ScrollBar:setValue("Value", self.Value)
end

function SBSlider:updateSlider()
	Slider.updateSlider(self)
	local disabled = self.Min == self.Max
	local sb = self.ScrollBar.Children
	if disabled ~= sb[1].Disabled then
		sb[1]:setValue("Disabled", disabled)
		sb[2]:setValue("Disabled", disabled)
		sb[3]:setValue("Disabled", disabled)
	end
end

function SBSlider:onSetRange()
	Slider.onSetRange(self)
	self.ScrollBar:setValue("Range", self.Range)
end

function SBSlider:onSetMin()
	Slider.onSetMin(self)
	self.ScrollBar:setValue("Min", self.Min)
end

function SBSlider:onSetMax()
	Slider.onSetMax(self)
	self.ScrollBar:setValue("Max", self.Max)
end

-------------------------------------------------------------------------------
--	addClassNotifications: overrides
-------------------------------------------------------------------------------

function ScrollBar.addClassNotifications(proto)
	ScrollBar.addNotify(proto, "Value", ScrollBar.NOTIFY_ALWAYS,
		{ ScrollBar.NOTIFY_SELF, "onSetValue" })
	ScrollBar.addNotify(proto, "Min", ScrollBar.NOTIFY_ALWAYS,
		{ ScrollBar.NOTIFY_SELF, "onSetMin" })
	ScrollBar.addNotify(proto, "Max", ScrollBar.NOTIFY_ALWAYS,
		{ ScrollBar.NOTIFY_SELF, "onSetMax" })
	ScrollBar.addNotify(proto, "Range", ScrollBar.NOTIFY_ALWAYS,
		{ ScrollBar.NOTIFY_SELF, "onSetRange" })
	return Group.addClassNotifications(proto)
end

ScrollBar.ClassNotifications = 
ScrollBar.addClassNotifications { Notifications = { } }

-------------------------------------------------------------------------------
--	new: overrides
-------------------------------------------------------------------------------

function ScrollBar.new(class, self)
	self = self or { }
	self.Min = self.Min or 1
	self.Max = self.Max or 1
	self.Default = max(self.Min, min(self.Max, self.Default or self.Min))
	self.Value = max(self.Min, min(self.Max, self.Value or self.Default))
	self.Range = max(self.Max, self.Range or self.Max)
	self.Increment = self.Increment or 1
	self.Step = self.Step or false
	self.Child = self.Child or false
	self.ArrowOrientation = self.ArrowOrientation or self.Orientation
	self.Kind = self.Kind or "scrollbar"
	if self.AcceptFocus == nil then
		self.AcceptFocus = true
	end
	
	self.Slider = self.Slider or SBSlider:new
	{
		Child = self.Child,
		ScrollBar = false, -- initialized later, see below
		Min = self.Min,
		Max = self.Max,
		Value = self.Value,
		Range = self.Range,
		Increment = self.Increment,
		Orientation = self.Orientation,
		Step = self.Step,
		Kind = self.Kind,
		Height = "fill",
		Width = "fill",
	}
	
	local img1, img2
	local class1, class2 = "scrollbar-arrowleft", "scrollbar-arrowright"
	local increase = self.Increment

	if self.Orientation == "vertical" then
		if self.ArrowOrientation == "horizontal" then
			img1, img2 = ArrowLeftImage, ArrowRightImage
			increase = -self.Increment
		else
			img1, img2 = ArrowUpImage, ArrowDownImage
			class1, class2 = "scrollbar-arrowup", "scrollbar-arrowdown"
		end
	else
		if self.ArrowOrientation == "vertical" then
			img1, img2 = ArrowUpImage, ArrowDownImage
			increase = -self.Increment
			class1, class2 = "scrollbar-arrowup", "scrollbar-arrowdown"
		else
			img1, img2 = ArrowLeftImage, ArrowRightImage
		end
	end

	self.ArrowButton1 = ArrowButton:new
	{
		Image = img1,
		Class = "button " .. class1,
		Slider = self.Slider,
		ScrollBar = self,
		Direction = -increase
	}
	self.ArrowButton2 = ArrowButton:new
	{
		Image = img2,
		Class = "button " .. class2,
		Slider = self.Slider,
		ScrollBar = self,
		Direction = increase
	}

	if self.ArrowOrientation ~= self.Orientation then
		self.Children =
		{
			self.Slider,
			Group:new
			{
				Width = "fill",
				Height = "fill",
				Orientation = "horizontal" and "vertical" or self.Orientation,
				Children =
				{
					self.ArrowButton1,
					self.ArrowButton2
				}
			}
		}
	else
		self.Children =
		{
			self.Slider,
			self.ArrowButton1,
			self.ArrowButton2
		}
	end
	
	self = Group.new(class, self)
	
	self.Slider.ScrollBar = self
	
	return self
end

-------------------------------------------------------------------------------
--	setup: overrides
-------------------------------------------------------------------------------

function ScrollBar:setup(app, window)
	if self.Orientation == "vertical" then
		self.Width = "auto"
		self.MaxHeight = "none"
		self.Height = self.Height or "fill"
	else
		self.MaxWidth = "none"
		self.Height = "auto"
		self.Width = self.Width or "fill"
	end
	Group.setup(self, app, window)
end

-------------------------------------------------------------------------------
--	onSetMin(): This handler is invoked when the ScrollBar's {{Min}}
--	attribute has changed. See also Numeric:onSetMin().
-------------------------------------------------------------------------------

function ScrollBar:onSetMin()
	self.Slider:setValue("Min", self.Min)
	self.Min = self.Slider.Min
end

-------------------------------------------------------------------------------
--	onSetMax(): This handler is invoked when the ScrollBar's {{Max}}
--	attribute has changed. See also Numeric:onSetMax().
-------------------------------------------------------------------------------

function ScrollBar:onSetMax()
	self.Slider:setValue("Max", self.Max)
	self.Max = self.Slider.Max
end

-------------------------------------------------------------------------------
--	onSetValue(): This handler is invoked when the ScrollBar's {{Value}}
--	attribute has changed. See also Numeric:onSetValue().
-------------------------------------------------------------------------------

function ScrollBar:onSetValue()
	self.Slider:setValue("Value", self.Value)
	self.Value = self.Slider.Value
end

-------------------------------------------------------------------------------
--	onSetRange(): This handler is invoked when the ScrollBar's {{Range}}
--	attribute has changed. See also Slider:onSetRange().
-------------------------------------------------------------------------------

function ScrollBar:onSetRange()
	self.Slider:setValue("Range", self.Range)
	self.Range = self.Slider.Range
end

return ScrollBar
