-------------------------------------------------------------------------------
--
--	tek.ui.class.numeric
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
--		Numeric ${subclasses(Numeric)}
--
--		This class implements the management of numerical
--		values. Without further specialization it has hardly any real-life
--		use and may be considered an abstract class. See also
--		[[#tek.ui.class.gauge : Gauge]] and [[#tek.ui.class.slider : Slider]]
--		for some of its child classes.
--
--	ATTRIBUTES::
--		- {{Default [IG]}} (number)
--			The default for {{Value}}, which can be revoked using the
--			Numeric:reset() method.
--		- {{Increment [ISG]}} (number)
--			Default increase/decrease step value [Default: 1]
--		- {{Max [ISG]}} (number)
--			Maximum acceptable {{Value}}. Setting this value
--			invokes the Numeric:onSetMax() method.
--		- {{Min [ISG]}} (number)
--			Minimum acceptable {{Value}}. Setting this value
--			invokes the Numeric:onSetMin() method.
--		- {{Value [ISG]}} (number)
--			The current value represented by this class. Setting this
--			value causes the Numeric:onSetValue() method to be invoked.
--
--	IMPLEMENTS::
--		- Numeric:decrease() - Decreases {{Value}}
--		- Numeric:increase() - Increments {{Value}}
--		- Numeric:onSetMax() - Handler for the {{Max}} attribute
--		- Numeric:onSetMin() - Handler for the {{Min}} attribute
--		- Numeric:onSetValue() - Handler for the {{Value}} attribute
--		- Numeric:reset() - Resets {{Value}} to the {{Default}} value
--
--	OVERRIDES::
--		- Object.addClassNotifications()
--		- Element:cleanup()
--		- Class.new()
--		- Element:setup()
--
-------------------------------------------------------------------------------

local db = require "tek.lib.debug"
local ui = require "tek.ui".checkVersion(112)
local Widget = ui.require("widget", 25)

local floor = math.floor
local max = math.max
local min = math.min

local Numeric = Widget.module("tek.ui.class.numeric", "tek.ui.class.widget")
Numeric._VERSION = "Numeric 5.2"

-------------------------------------------------------------------------------
--	addClassNotifications: overrides
-------------------------------------------------------------------------------

function Numeric.addClassNotifications(proto)
	Numeric.addNotify(proto, "Value", ui.NOTIFY_ALWAYS, 
		{ ui.NOTIFY_SELF, "onSetValue" })
	Numeric.addNotify(proto, "Min", ui.NOTIFY_ALWAYS,
		{ ui.NOTIFY_SELF, "onSetMin" })
	Numeric.addNotify(proto, "Max", ui.NOTIFY_ALWAYS,
		{ ui.NOTIFY_SELF, "onSetMax" })
	return Widget.addClassNotifications(proto)
end

Numeric.ClassNotifications = 
	Numeric.addClassNotifications { Notifications = { } }

-------------------------------------------------------------------------------
--	new: overrides
-------------------------------------------------------------------------------

function Numeric.new(class, self)
	self = self or { }
	self.Max = self.Max or 100
	self.Min = self.Min or 1
	self.Default = max(self.Min, min(self.Max, self.Default or self.Min))
	self.Value = max(self.Min, min(self.Max, self.Value or self.Default))
	self.Increment = self.Increment or 1
	return Widget.new(class, self)
end

-------------------------------------------------------------------------------
--	increase([delta]): Increment {{Value}} by the specified {{delta}}.
--	If {{delta}} is omitted, the {{Increment}} attribute is used in its place.
-------------------------------------------------------------------------------

function Numeric:increase(d)
	self:setValue("Value", self.Value + (d or self.Increment))
end

-------------------------------------------------------------------------------
--	decrease([delta]): Decrease {{Value}} by the specified {{delta}}.
--	If {{delta}} is omitted, the {{Increment}} attribute is used in its place.
-------------------------------------------------------------------------------

function Numeric:decrease(d)
	self:setValue("Value", self.Value - (d or self.Increment))
end

-------------------------------------------------------------------------------
--	reset(): Reset {{Value}} to is {{Default}} value.
-------------------------------------------------------------------------------

function Numeric:reset()
	self:setValue("Value", self.Default)
end

-------------------------------------------------------------------------------
--	onSetValue(): This handler is invoked when the Numeric's {{Value}}
--	attribute has changed.
-------------------------------------------------------------------------------

function Numeric:onSetValue()
	self.Value = max(self.Min, min(self.Max, self.Value))
end

-------------------------------------------------------------------------------
--	onSetMin(): This handler is invoked when the Numeric's {{Min}}
--	attribute has changed.
-------------------------------------------------------------------------------

function Numeric:onSetMin()
	self.Min = min(self.Min, self.Max)
	self:setValue("Value", self.Value)
end

-------------------------------------------------------------------------------
--	onSetMax(): This handler is invoked when the Numeric's {{Max}}
--	attribute has changed.
-------------------------------------------------------------------------------

function Numeric:onSetMax()
	self.Max = max(self.Min, self.Max)
	local d = self.Value - self.Max
	if d > 0 then
		self.Value = self.Value - d
	end
	self:setValue("Value", self.Value)
end

return Numeric
