-------------------------------------------------------------------------------
--
--	tek.ui.class.widget
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
--		Widget ${subclasses(Widget)}
--
--		This class implements interactions with the user.
--
--	ATTRIBUTES::
--		- {{Active [SG]}} (boolean)
--			The Widget's activation state. While '''true''', the position of
--			the pointing device is being verified (which is also reflected by
--			the {{Hilite}} attribute, see below). When the {{Active}} state
--			variable changes, the Widget's behavior depends on its {{Mode}}
--			attribute (see below):
--				* in ''button'' mode, the {{Selected}} attribute is set to
--				the value of the {{Hilite}} attribute. When the {{Selected}}
--				state changes, the {{Pressed}} attribute is set to the value
--				of the {{Active}} attribute.
--				* in ''toggle'' mode, the {{Selected}} attribute is inverted
--				logically, and the {{Pressed}} attribute is set to '''true'''.
--				* in ''touch'' mode, the {{Selected}} and {{Pressed}}
--				attributes are set to '''true''', if the Widget was not
--				selected already.
--			Changing this attribute invokes the Widget:onActivate() method.
--		- {{DblClick [SG]}} (boolean)
--			Signifies that the element is or has been double-clicked; it is
--			set to '''true''' when the element was double-clicked and is still
--			being held, and '''false''' when the second press has been
--			released. Changes to this attribute cause the invocation of the
--			Widget:onDblClick() method.
--		- {{FGPen [IG]}} (color specification)
--			A color specification for rendering the foreground details of the
--			element. This attribute is controllable via the ''color'' style
--			property.
--		- {{Hold [SG]}} (boolean)
--			Signifies that the element is being pressed for an extended period.
--			While being held, the value is repeatedly set to '''true''' in
--			intervals of {{n/50}} seconds, with {{n}} taken from the
--			[[#tek.ui.class.window : Window]]'s {{HoldTickRepeat}} attribute.
--			When the element is released, this attribute is set to '''false'''.
--			Changes to this attribute cause the invocation of the
--			Widget:onHold() method.
--		- {{InitialFocus [I]}} (boolean)
--			Specifies that the element should receive the focus initially.
--			If '''true''', the element will set its {{Focus}} attribute to
--			'''true''' upon invocation of the [[#Area:show : show]] method.
--			The boolean will be translated to the flag {{FL_INITIALFOCUS}},
--			and is meaningless after initialization.
--		- {{KeyCode [IG]}} (string or boolean)
--			If set, a keyboard equivalent for activating the element. See
--			[[#tek.ui.class.popitem : PopItem]] for a discussion of denoting
--			keyboard qualifiers. The [[#tek.ui.class.text : Text]] class allows
--			setting this attribute to '''true''', in which case the element's
--			{{Text}} will be examined during setup for an initiatory character
--			(by default an underscore), and if found, the {{KeyCode}} attribute
--			will be replaced by the character following this marker.
--		- {{Mode [ISG]}} (string)
--			The element's interaction mode:
--				* {{"inert"}}: The element does not react on input
--				* {{"touch"}}: The element does not rebound and keeps its
--				{{Selected}} state; it cannot be unselected by the user and
--				always submits '''true''' for the {{Pressed}} and {{Selected}}
--				attributes.
--				* {{"toggle"}}: The element does not rebound immediately
--				and keeps its {{Selected}} state until the next activation.
--				* {{"button"}}: The element rebounds when the pointing device
--				over it is being released, or when it is no longer hovering it.
--			See also the {{Active}} attribute.
--		- {{Pressed [SG]}} (boolean)
--			Signifies that a button was pressed or released. Changes to this
--			state variable invoke Widget:onPress().
--
--	STYLE PROPERTIES::
--		''color'' || controls the {{Widget.FGPen}} attribute
--		''effect-class'' || name of an class for rendering an overlay effect
--		''effect-color'' || controls the ''ripple'' effect hook
--		''effect-color2'' || controls the ''ripple'' effect hook
--		''effect-kind'' || controls the ''ripple'' effect hook
--		''effect-maxnum'' || controls the ''ripple'' effect hook
--		''effect-maxnum2'' || controls the ''ripple'' effect hook
--		''effect-orientation'' || controls the ''ripple'' effect hook
--		''effect-padding'' || controls the ''ripple'' effect hook
--		''effect-ratio'' || controls the ''ripple'' effect hook
--		''effect-ratio2'' || controls the ''ripple'' effect hook
--
--		A possible name for the ''effect-class'' property is {{"ripple"}}.
--		As its name suggests, it can paint various ripple effects (e.g. for
--		slider knobs and bar handles). Effect hooks are loaded from
--		{{tek.ui.hook}} and may define their own style properties.
--
--	STYLE PSEUDO CLASSES::
--		''active'' || for elements in active state
--		''disabled'' || for elements in disabled state
--		''focus'' || for elements that have the input focus
--		''hover'' || for elements that are being hovered by the mouse
--
--	IMPLEMENTS::
--		- Widget:activate() - Activate this or neighbour element
--		- Widget:onActivate() - Handler for {{Active}}
--		- Widget:onClick() - Gets called when the element has been clicked
--		- Widget:onDblClick() - Handler for {{DblClick}}
--		- Widget:onDisable() - Handler for {{Disabled}}
--		- Widget:onFocus() - Handler for {{Focus}}
--		- Widget:onHilite() - Handler for {{Hilite}}
--		- Widget:onHold() - Handler for {{Hold}}
--		- Widget:onPress() - Handler for {{Pressed}}
--		- Widget:onSelect() - Handler for {{Selected}}
--
--	OVERRIDES::
--		- Object.addClassNotifications()
--		- Area:checkFocus()
--		- Area:checkHilite()
--		- Element:cleanup()
--		- Element:getPseudoClass()
--		- Object.init()
--		- Area:layout()
--		- Area:passMsg()
--		- Area:setState()
--		- Element:setup()
--		- Area:show()
--
-------------------------------------------------------------------------------

local ui = require "tek.ui".checkVersion(112)
local Frame = ui.require("frame", 17)

local bor = ui.bor

local Widget = Frame.module("tek.ui.class.widget", "tek.ui.class.frame")
Widget._VERSION = "Widget 31.1"

-------------------------------------------------------------------------------
--	constants & class data:
-------------------------------------------------------------------------------

local MSG_MOUSEMOVE = ui.MSG_MOUSEMOVE
local MSG_MOUSEBUTTON = ui.MSG_MOUSEBUTTON

local FL_REDRAW = ui.FL_REDRAW
local FL_REDRAWBORDER = ui.FL_REDRAWBORDER
local FL_RECVINPUT = ui.FL_RECVINPUT
local FL_RECVMOUSEMOVE = ui.FL_RECVMOUSEMOVE
local FL_AUTOPOSITION = ui.FL_AUTOPOSITION
local FL_ACTIVATERMB = ui.FL_ACTIVATERMB
local FL_INITIALFOCUS = ui.FL_INITIALFOCUS
local FL_POPITEM = ui.FL_POPITEM

-------------------------------------------------------------------------------
--	addClassNotifications: overrides
-------------------------------------------------------------------------------

function Widget.addClassNotifications(proto)
	Widget.addNotify(proto, "DblClick", ui.NOTIFY_ALWAYS,
		{ ui.NOTIFY_SELF, "onDblClick" })
	Widget.addNotify(proto, "Disabled", ui.NOTIFY_ALWAYS,
		{ ui.NOTIFY_SELF, "onDisable" })
	Widget.addNotify(proto, "Hilite", ui.NOTIFY_ALWAYS,
		{ ui.NOTIFY_SELF, "onHilite" })
	Widget.addNotify(proto, "Selected", ui.NOTIFY_ALWAYS,
		{ ui.NOTIFY_SELF, "onSelect" })
	Widget.addNotify(proto, "Hold", ui.NOTIFY_ALWAYS,
		{ ui.NOTIFY_SELF, "onHold" })
	Widget.addNotify(proto, "Active", ui.NOTIFY_ALWAYS,
		{ ui.NOTIFY_SELF, "onActivate" })
	Widget.addNotify(proto, "Pressed", ui.NOTIFY_ALWAYS,
		{ ui.NOTIFY_SELF, "onPress" })
	Widget.addNotify(proto, "Focus", ui.NOTIFY_ALWAYS,
		{ ui.NOTIFY_SELF, "onFocus" })
	return Frame.addClassNotifications(proto)
end

Widget.ClassNotifications = 
	Widget.addClassNotifications { Notifications = { } }

-------------------------------------------------------------------------------
--	init: overrides
-------------------------------------------------------------------------------

function Widget.new(class, self)
	self = self or { }
	local flags = 0
	if self.ActivateOnRMB then
		flags = bor(flags, FL_ACTIVATERMB)
	end
	if self.InitialFocus then
		flags = bor(flags, FL_INITIALFOCUS)
	end
	if self.NoFocus then
		flags = bor(flags, ui.FL_NOFOCUS)
	end
	self.Flags = bor(self.Flags or 0, flags)
	self.Active = false
	self.DblClick = false
	self.EffectHook = false
	self.FGPen = false
	self.Hold = false
	self.KeyCode = self.KeyCode or false
	self.Mode = self.Mode or "inert"
	self.Pressed = false
	return Frame.new(class, self)
end

-------------------------------------------------------------------------------
--	setup: overrides
-------------------------------------------------------------------------------

function Widget:setup(app, window)
	Frame.setup(self, app, window)
	-- create effect hook:
	self.EffectHook = ui.createHook("hook", self.Properties["effect-class"],
		self, { Style = self.Style })
	local interactive = self.Mode ~= "inert"
	if interactive then
		local keycode = self.KeyCode
		if keycode then
			self.Window:addKeyShortcut(keycode, self)
		end
		self:setFlags(FL_RECVMOUSEMOVE)
	end
	self:setFlags(FL_RECVINPUT)
end

-------------------------------------------------------------------------------
--	cleanup: overrides
-------------------------------------------------------------------------------

function Widget:cleanup()
	self:checkClearFlags(0, FL_RECVINPUT + FL_RECVMOUSEMOVE)
	if self:checkFlags(ui.FL_SETUP) then
		self.EffectHook = ui.destroyHook(self.EffectHook)
		self.Window:remKeyShortcut(self.KeyCode, self)
	end
	Frame.cleanup(self)
end

-------------------------------------------------------------------------------
--	show: overrides
-------------------------------------------------------------------------------

function Widget:show()
	Frame.show(self)
	if self.Mode ~= "inert" and self:checkFlags(FL_INITIALFOCUS) then
		self:setValue("Focus", true)
	end
end

-------------------------------------------------------------------------------
--	layout: overrides
-------------------------------------------------------------------------------

function Widget:layout(x0, y0, x1, y1, markdamage)
	if Frame.layout(self, x0, y0, x1, y1, markdamage) then
		if self.EffectHook then
			self.EffectHook:layout(self:getRect())
		end
		return true
	end
end

-------------------------------------------------------------------------------
--	draw: overrides
-------------------------------------------------------------------------------

function Widget:draw()
	if Frame.draw(self) then
		local e = self.EffectHook
		if e then
			e:draw()
		end
		return true
	end
end

-------------------------------------------------------------------------------
--	Widget:onActivate(): This method is invoked when the {{Active}}
--	attribute has changed.
-------------------------------------------------------------------------------

function Widget:onActivate()

	local active = self.Active
	local win = self.Window
	local mode = self.Mode
	local selected = self.Selected

	-- released over a popup which was entered with the button held?
	local collapse = self:checkFlags(ui.FL_POPITEM) and win and
		win.PopupRootWindow

	if win then
		if mode == "toggle" then
			if active or collapse then
				self:setValue("Selected", not selected)
			end
		elseif mode == "touch" then
			if (active and not selected) or collapse then
				self:setValue("Selected", true)
				self:setValue("Pressed", true, true)
				self:setValue("Pressed", false)
			end
		elseif mode == "button" then
			self:setValue("Selected", active and self.Hilite)
			if (not selected ~= not active) or collapse then
				if active then
					win:setDblClickElement(self)
				end
				self:setValue("Pressed", active, true)
				if collapse then
					self:setValue("Pressed", false)
					self:setValue("Selected", false)
				end
			end
		end
		win = self.Window
	end
	
	if collapse and win then
		win:finishPopup()
	end

	self:setState()
end

-------------------------------------------------------------------------------
--	Widget:onDisable(): This method is invoked when the {{Disabled}}
--	attribute has changed. The {{Disabled}} attribute is defined in the
--	[[#tek.ui.class.area : Area]] class.
-------------------------------------------------------------------------------

function Widget:onDisable()
	local win = self.Window
	if self.Disabled and self.Focus and win then
		win:setFocusElement()
	end
	self:setFlags(FL_REDRAW)
	self:setState()
end

-------------------------------------------------------------------------------
--	Widget:onSelect(): This method is invoked when the {{Selected}}
--	attribute has changed. The {{Selected}} attribute is defined in the
--	[[#tek.ui.class.area : Area]] class.
-------------------------------------------------------------------------------

function Widget:onSelect()

	--	HACK for better touchpad support -- unfortunately an element is
	--	deselected also when the mouse is leaving the window, so this is
	--	not entirely satisfactory.

	-- if not selected then
	-- 	if self.Active then
	-- 		db.warn("Element deselected, forcing inactive")
	-- 		self.Window:setActiveElement()
	-- 	end
	-- end

	self:setFlags(FL_REDRAWBORDER)
	self:setState()
end

-------------------------------------------------------------------------------
--	Widget:onHilite(): This handler is called when the {{Hilite}}
--	attribute has changed. The {{Hilite}} attribute is defined in the
--	[[#tek.ui.class.area : Area]] class.
-------------------------------------------------------------------------------

function Widget:onHilite()
	if self.Mode == "button" then
		self:setValue("Selected", self.Active and self.Hilite)
	end
	self:setState()
end

-------------------------------------------------------------------------------
--	Widget:onPress(): This handler is called when the {{Pressed}}
--	attribute has changed.
-------------------------------------------------------------------------------

function Widget:onPress()
	if not self.Pressed then
		self:onClick()
	end
end

-------------------------------------------------------------------------------
--	Widget:onClick(): This method is called when the {{Pressed}} attribute
--	changes from '''true''' to '''false''', indicating that the pointing
--	device has been released over the element.
-------------------------------------------------------------------------------

function Widget:onClick()
end

-------------------------------------------------------------------------------
--	getPseudoClass: overrides
-------------------------------------------------------------------------------

function Widget:getPseudoClass()
	if self:checkFlags(FL_POPITEM) then
		-- in a popup item, give the hover class precedence:
		return self.Disabled and ":disabled" or
			self.Hilite and ":hover" or
			self.Selected and ":active" or
			self.Focus and ":focus" or
			""
	end
	return self.Disabled and ":disabled" or
		self.Selected and ":active" or
		self.Focus and ":focus" or
		self.Hilite and ":hover" or
		""
end

-------------------------------------------------------------------------------
--	setState: overrides
-------------------------------------------------------------------------------

function Widget:setState(bg, fg)
	if self:checkFlags(ui.FL_SETUP) then
		local props = self.Properties
		local pclass = self:getPseudoClass()
		bg = bg or props["background-color" .. pclass]
		fg = fg or props["color" .. pclass] or "detail"
		if fg ~= self.FGPen then
			self.FGPen = fg
			self:setFlags(FL_REDRAW)
		end
		Frame.setState(self, bg)
	end
end

-------------------------------------------------------------------------------
--	passMsg: overrides
-------------------------------------------------------------------------------

function Widget:passMsg(msg)
	local win = self.Window
	if msg[2] == MSG_MOUSEBUTTON then
		local armb = self:checkFlags(FL_ACTIVATERMB)
		local mx, my = self:getMsgFields(msg, "mousexy")
		if msg[3] == 1 or (armb and msg[3] == 4) then -- l/r down:
			if win.HoverElement == self and not self.Disabled then
				win:setHiliteElement(self)
				if self:checkFocus() then
					win:setFocusElement(self)
				end
				win:setActiveElement(self)
			end
		elseif msg[3] == 2 or (armb and msg[3] == 8) then -- l/r up:
			if he == self then
				win:setHiliteElement(self)
			end
		end
	elseif msg[2] == MSG_MOUSEMOVE then
		local he = win.HoverElement == self
		if win.HiliteElement == self or he and not win.MovingElement then
			win:setHiliteElement(he and self)
			return false
		end
	end
	return msg
end

-------------------------------------------------------------------------------
--	checkFocus: overrides
-------------------------------------------------------------------------------

function Widget:checkFocus()
	local m = self.Mode
	return not self.Disabled and (m == "toggle" or m == "button" or
		(m == "touch" and not self.Selected)) and
		not self:checkFlags(ui.FL_NOFOCUS)
end

-------------------------------------------------------------------------------
--	checkHilite: overrides
-------------------------------------------------------------------------------

function Widget:checkHilite()
	return not self.Disabled and self.Mode ~= "inert"
end

-------------------------------------------------------------------------------
--	Widget:onFocus(): This method is invoked when the {{Focus}}
--	attribute has changed. The {{Focus}} attribute is defined in the
--	[[#tek.ui.class.area : Area]] class.
-------------------------------------------------------------------------------

function Widget:onFocus()
	local focused = self.Focus
	if focused and self:checkFlags(FL_AUTOPOSITION) then
		self:focusRect()
	end
	local w = self.Window
	if w then
		if focused then
			w:setFocusElement(self)
		elseif w.FocusElement == self then
			w:setFocusElement()
		end
	end
	self:setFlags(FL_REDRAWBORDER)
	self:setState()
end

-------------------------------------------------------------------------------
--	Widget:onHold(): This method is invoked when the {{Hold}} attribute
--	has changed.
-------------------------------------------------------------------------------

function Widget:onHold()
end

-------------------------------------------------------------------------------
--	Widget:onDblClick(): This method is invoked when the {{DblClick}}
--	attribute has changed. It is '''true''' when the double click was
--	initiated and the mouse button is still held, and '''false''' when it has
--	been released.
-------------------------------------------------------------------------------

function Widget:onDblClick()
end

-------------------------------------------------------------------------------
--	Widget:activate([mode]): Activates this or an element's "next" element.
--	{{mode}} can be {{"focus"}} (the default), {{"next"}}, or {{"click"}}.
-------------------------------------------------------------------------------

function Widget:activate(mode)
	local win = self.Window
	if not mode or mode == "focus" then
		win:setFocusElement(self)
	elseif mode == "next" then
		local e = win:getNextElement(self)
		if e then
			win:setFocusElement(e)
		end
	elseif mode == "click" then
		win:clickElement(self)
	end
end

-------------------------------------------------------------------------------
--	beginPopup: overrides
-------------------------------------------------------------------------------

function Widget:beginPopup(baseitem)
	Frame.beginPopup(self, baseitem)
	self.Active = false
	self.Pressed = false
end

return Widget
