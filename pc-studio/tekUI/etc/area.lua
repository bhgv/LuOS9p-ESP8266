-------------------------------------------------------------------------------
--
--	tek.ui.class.area
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	OVERVIEW::
--		[[#ClassOverview]] :
--		[[#tek.class : Class]] /
--		[[#tek.class.object : Object]] /
--		[[#tek.ui.class.element : Element]] / 
--		Area ${subclasses(Area)}
--
--		This is the base class of all visible user interface elements.
--		It implements an outer margin, layouting, drawing, and the
--		relationships to its neighbour elements.
--
--	ATTRIBUTES::
--		- {{AutoPosition [I]}} (boolean)
--			When the element receives the focus, this flag instructs it to
--			automatically position itself into the visible area of any Canvas
--			that may contain it. An affected [[#tek.ui.class.canvas : Canvas]]
--			must have its {{AutoPosition}} attribute enabled as well for this
--			option to take effect (but unlike the Area class, in a Canvas it
--			is disabled by default).
--			The boolean (default '''true''') will be translated to the flag
--			{{FL_AUTOPOSITION}}, and is meaningless after initialization.
--		- {{BGPen [G]}} (color specification)
--			The current color (or texture) for painting the element's
--			background. This value is set in Area:setState(), where it is
--			derived from the element's current state and the
--			''background-color'' style property. Valid are color names (e.g. 
--			{{"detail"}}, {{"fuchsia"}}, see also
--			[[#tek.ui.class.display : Display]] for more), a hexadecimal RGB
--			specification (e.g. {{"#334455"}}, {{"#f0f"}}), or an image URL
--			in the form {{"url(...)"}}.
--		- {{DamageRegion [G]}} ([[#tek.lib.region : Region]])
--			see {{TrackDamage}}
--		- {{Disabled [ISG]}} (boolean)
--			If '''true''', the element is in disabled state and loses its
--			ability to interact with the user. This state variable is handled
--			in the [[#tek.ui.class.widget : Widget]] class.
--		- {{EraseBG [I]}} (boolean)
--			If '''true''', the element's background is painted automatically
--			using the Area:erase() method. Set this attribute to '''false'''
--			if you intend to paint the background yourself in Area:draw().
--			The boolean (default '''true''') will be translated to the flag
--			{{FL_ERASEBG}}, and is meaningless after initialization.
--		- {{Flags [SG]}} (Flags field)
--			This attribute holds various status flags, among others:
--			- {{FL_SETUP}} - Set in Area:setup() and cleared in Area:cleanup()
--			- {{FL_LAYOUT}} - Set in Area:layout(), cleared in Area:cleanup()
--			- {{FL_SHOW}} - Set in Area:show(), cleared in Area:hide()
--			- {{FL_REDRAW}} - Set in Area:layout(), Area:damage(),
--			Area:setState() and possibly other places to indicate that the
--			element needs to be repainted. Cleared in Area:draw().
--			- {{FL_REDRAWBORDER}} - To indicate a repaint of the element's
--			borders. Handled in the [[#tek.ui.class.frame : Frame]] class.
--			- {{FL_CHANGED}} - This flag indicates that the contents of an
--			element have changed, i.e. when children were added to a group,
--			or when setting a new text or image should cause a recalculation
--			of its size.
--			- {{FL_POPITEM}} - Used to identify elements in popups, handled in
--			[[#tek.ui.class.popitem : PopItem]].
--		- {{Focus [SG]}} (boolean)
--			If '''true''', the element has the input focus. This state variable
--			is handled by the [[#tek.ui.class.widget : Widget]] class. Note:
--			This attribute represents only the current state. If you want to
--			place the initial focus on an element, use the {{InitialFocus}}
--			attribute in the [[#tek.ui.class.widget : Widget]] class.
--		- {{HAlign [IG]}} ({{"left"}}, {{"center"}}, {{"right"}})
--			Horizontal alignment of the element in its group. This attribute
--			can be controlled using the {{halign}} style property.
--		- {{Height [IG]}} (number, {{"auto"}}, {{"fill"}}, {{"free"}})
--			Height of the element, in pixels, or
--				- {{"auto"}} - Reserves the minimum required
--				- {{"free"}} - Allows the element's height to grow to any size.
--				- {{"fill"}} - To fill up the height that other elements in the
--				same group have claimed, without claiming more.
--			This attribute can be controlled using the {{height}} style
--			property.
--		- {{Hilite [SG]}} (boolean)
--			If '''true''', the element is in highlighted state. This
--			state variable is handled by the [[#tek.ui.class.widget : Widget]]
--			class.
--		- {{MaxHeight [IG]}} (number or {{"none"}})
--			Maximum height of the element, in pixels [default: {{"none"}}].
--			This attribute is controllable via the {{max-height}} style
--			property.
--		- {{MaxWidth [IG]}} (number or {{"none"}})
--			Maximum width of the element, in pixels [default: {{"none"}}].
--			This attribute is controllable via the {{max-width}} style
--			property.
--		- {{MinHeight [IG]}} (number)
--			Minimum height of the element, in pixels [default: {{0}}].
--			This attribute is controllable via the {{min-height}} style
--			property.
--		- {{MinWidth [IG]}} (number)
--			Minimum width of the element, in pixels [default: {{0}}].
--			This attribute is controllable via the {{min-width}} style
--			property.
--		- {{Selected [ISG]}} (boolean)
--			If '''true''', the element is in selected state. This state
--			variable is handled by the [[#tek.ui.class.widget : Widget]] class.
--		- {{TrackDamage [I]}} (boolean)
--			If '''true''', the element collects intra-area damages in a
--			[[#tek.lib.region : Region]] named {{DamageRegion}}, which can be
--			used by class writers to implement minimally invasive repaints.
--			Default: '''false''', the element is repainted in its entirety.
--			The boolean will be translated to the flag {{FL_TRACKDAMAGE}} and
--			is meaningless after initialization.
--		- {{VAlign [IG]}} ({{"top"}}, {{"center"}}, {{"bottom"}})
--			Vertical alignment of the element in its group. This attribute
--			can be controlled using the {{valign}} style property.
--		- {{Weight [IG]}} (number)
--			Specifies the weight that is attributed to the element relative
--			to its siblings in the same group. By recommendation, the weights
--			in a group should sum up to 0x10000.
--		- {{Width [IG]}} (number, {{"auto"}}, {{"fill"}}, {{"free"}})
--			Width of the element, in pixels, or
--				- {{"auto"}} - Reserves the minimum required
--				- {{"free"}} - Allows the element's width to grow to any size
--				- {{"fill"}} - To fill up the width that other elements in the
--				same group have claimed, without claiming more.
--			This attribute can be controlled using the {{width}} style
--			property.
--
--	STYLE PROPERTIES::
--		''background-attachment'' || {{"scollable"}} or {{"fixed"}}
--		''background-color'' || Controls {{Area.BGPen}}
--		''fixed'' || coordinates used by the fixed layouter: {{"x0 y0 x1 y1"}}
--		''halign'' || controls the {{Area.HAlign}} attribute
--		''height'' || controls the {{Area.Height}} attribute
--		''margin-bottom'' || the element's bottom margin, in pixels
--		''margin-left'' || the element's left margin, in pixels
--		''margin-right'' || the element's right margin, in pixels
--		''margin-top'' || the element's top margin, in pixels
--		''max-height'' || controls the {{Area.MaxHeight}} attribute
--		''max-width'' || controls the {{Area.MaxWidth}} attribute
--		''min-height'' || controls the {{Area.MinHeight}} attribute
--		''min-width'' || controls the {{Area.MinWidth}} attribute
--		''padding-bottom'' || the element's bottom padding
--		''padding-left'' || the element's left padding
--		''padding-right'' || the element's right padding
--		''padding-top'' || the element's top padding
--		''valign'' || controls the {{Area.VAlign}} attribute
--		''width'' || controls the {{Area.Width}} attribute
--
--		Note that repainting elements with a {{"fixed"}}
--		''background-attachment'' can be expensive. This variant should be
--		used sparingly, and some classes may implement it incompletely or
--		incorrectly.
--
--	IMPLEMENTS::
--		- Area:askMinMax() - Queries the element's minimum and maximum size
--		- Area:checkClearFlags() - Check and clear an element's flags
--		- Area:checkFlags() - Check an element's flags
--		- Area:checkFocus() - Checks if the element can receive the input focus
--		- Area:checkHilite() - Checks if the element can be highlighted
--		- Area:damage() - Notifies the element of a damage
--		- Area:draw() - Paints the element
--		- Area:drawBegin() - Prepares the rendering context
--		- Area:drawEnd() - Reverts the changes made in drawBegin()
--		- Area:erase() - Erases the element's background
--		- Area:focusRect() - Makes the element fully visible, if possible
--		- Area:getBG() - Gets the element's background properties
--		- Area:getBGElement() - Gets the element's background element
--		- Area:getByXY() - Checks if the element covers a coordinate
--		- Area:getChildren() - Gets the element's children
--		- Area:getDisplacement(): Get this element's coordinate displacement
--		- Area:getGroup() - Gets the element's group
--		- Area:getMargin() - Gets the element's margin
--		- Area:getMinMax() - Gets the element's calculated min/max sizes
--		- Area:getMsgFields() - Get fields of an input message
--		- Area:getNext() - Gets the element's successor in its group
--		- Area:getPadding() - Gets the element's paddings
--		- Area:getParent() - Gets the element's parent element
--		- Area:getPrev() - Gets the element's predecessor in its group
--		- Area:getRect() - Returns the element's layouted coordinates
--		- Area:getSiblings() - Gets the element's siblings
--		- Area:hide() - Gets called when the element is about to be hidden
--		- Area:layout() - Layouts the element into a rectangle
--		- Area:passMsg() - Passes an input message to the element
--		- Area:punch() - Subtracts the outline of the element from a
--		[[#tek.lib.region : Region]]
--		- Area:rethinkLayout() - Causes a relayout of the element and its group
--		- Area:setFlags() - Sets an element's flags
--		- Area:setState() - Sets the background attribute of an element
--		- Area:show() - Gets called when the element is about to be shown
--
--	OVERRIDES::
--		- Element:cleanup()
--		- Class.new()
--		- Element:onSetStyle()
--		- Element:setup()
--
-------------------------------------------------------------------------------

local db = require "tek.lib.debug"
local ui = require "tek.ui".checkVersion(112)
local Element = ui.require("element", 19)
local Region = ui.loadLibrary("region", 10)

local assert = assert
local band = ui.band
local bnot = ui.bnot
local bor = ui.bor
local intersect = Region.intersect
local max = math.max
local min = math.min
local newregion = Region.new
local tonumber = tonumber
local type = type

local Area = Element.module("tek.ui.class.area", "tek.ui.class.element")
Area._VERSION = "Area 57.5"

local FL_REDRAW = ui.FL_REDRAW
local FL_LAYOUT = ui.FL_LAYOUT
local FL_SETUP = ui.FL_SETUP
local FL_SHOW = ui.FL_SHOW
local FL_CHANGED = ui.FL_CHANGED
local FL_REDRAWBORDER = ui.FL_REDRAWBORDER
local FL_UPDATE = ui.FL_UPDATE
local FL_CHANGED = ui.FL_CHANGED
local FL_BUBBLEUP = FL_REDRAW + FL_REDRAWBORDER + FL_CHANGED
local FL_AUTOPOSITION = ui.FL_AUTOPOSITION
local FL_ERASEBG = ui.FL_ERASEBG
local FL_TRACKDAMAGE = ui.FL_TRACKDAMAGE
local FL_DONOTBLIT = ui.FL_DONOTBLIT

local HUGE = ui.HUGE

function Area.addClassNotifications(proto)
	Area.addNotify(proto, "Invisible", ui.NOTIFY_ALWAYS, 
		{ ui.NOTIFY_SELF, "onSetInvisible" })
	return Element.addClassNotifications(proto)
end

Area.ClassNotifications = Area.addClassNotifications { Notifications = { } }

-------------------------------------------------------------------------------
--	new: overrides
-------------------------------------------------------------------------------

function Area.new(class, self)
	self = self or { }
	-- Calculated minimum/maximum sizes of the element:
	self.MinMax = newregion()
	-- The layouted rectangle of the element on the display:
	self.Rect = newregion()

	self.BGPen = false
	self.DamageRegion = false
	self.Disabled = self.Disabled or false
	self.Focus = false
	self.HAlign = self.HAlign or false
	self.Height = self.Height or false
	self.Hilite = false
	self.MaxHeight = self.MaxHeight or false
	self.MaxWidth = self.MaxWidth or false
	self.MinHeight = self.MinHeight or false
	self.MinWidth = self.MinWidth or false
	self.Selected = self.Selected or false
	self.VAlign = self.VAlign or false
	self.Weight = self.Weight or false
	self.Width = self.Width or false
	self.Invisible = self.Invisible or false
	
	local flags = 0
	if self.AutoPosition == nil or self.AutoPosition then
		flags = bor(flags, FL_AUTOPOSITION)
	end
	if self.EraseBG == nil or self.EraseBG then
		flags = bor(flags, FL_ERASEBG)
	end
	if self.TrackDamage then
		flags = bor(flags, FL_TRACKDAMAGE)
	end
	self.Flags = bor(self.Flags or 0, flags)
	
	return Element.new(class, self)
end

-------------------------------------------------------------------------------
--	Area:setup(app, win): After passing the call on to Element:setup(),
--	initializes fields which are being used by Area:layout(), and sets
--	{{FL_SETUP}} in the {{Flags}} field to indicate that the element is
--	ready for getting layouted and displayed.
-------------------------------------------------------------------------------

function Area:setup(app, win)
	Element.setup(self, app, win)
	self:setFlags(FL_SETUP)
end

-------------------------------------------------------------------------------
--	Area:cleanup(): Clears all temporary layouting data and the {{FL_LAYOUT}}
--	and {{FL_SETUP}} flags, before passing on the call to Element:cleanup().
-------------------------------------------------------------------------------

function Area:cleanup()
	Element.cleanup(self)
	self.DamageRegion = false
	self.MinMax = newregion()
	self.Rect = newregion()
	self:checkClearFlags(FL_LAYOUT + FL_SETUP + FL_REDRAW)
end

-------------------------------------------------------------------------------
--	Area:show(): This function is called when the element's window is opened.
-------------------------------------------------------------------------------

function Area:show()
	self:setState()
	if not self.Invisible then
		self:setFlags(FL_SHOW)
	end
end

-------------------------------------------------------------------------------
--	Area:hide(): Override this method to free all display-related resources
--	previously allocated in Area:show().
-------------------------------------------------------------------------------

function Area:hide()
	self:checkClearFlags(FL_SHOW + FL_REDRAW + FL_REDRAWBORDER)
end

-------------------------------------------------------------------------------
--	Area:rethinkLayout([repaint[, check_size]]): Slates an element (and its
--	children) for relayouting, which takes place during the next Window update
--	cycle. If the element's coordinates change, this will cause it to be
--	repainted. The parent element (usually a Group) will be checked as well,
--	so that it has the opportunity to update its FreeRegion.
--	The optional argument {{repaint}} can be used to specify additional hints:
--		- {{1}} - marks the element for repainting unconditionally (not
--		implying possible children)
--		- {{2}} - marks the element (and all possible children) for repainting
--		unconditionally
--	The optional argument {{check_size}} (a boolean) can be used to
--	recalculate the element's minimum and maximum size requirements.
-------------------------------------------------------------------------------

function Area:rethinkLayout(repaint, check_size)
	if self:checkFlags(FL_SETUP) then
		if check_size then
			-- indicate possible change of group structure:
			self:getGroup():setFlags(FL_CHANGED)
		end
		self.Window:addLayout(self, repaint or 0, check_size or false)
	end
end

-------------------------------------------------------------------------------
--	minw, minh, maxw, maxh = Area:askMinMax(minw, minh, maxw, maxh): This
--	method is called during the layouting process for adding the required
--	width and height to the minimum and maximum size requirements of the
--	element, before passing the result on to its super class. {{minw}},
--	{{minh}} are cumulative of the minimal size of the element, while
--	{{maxw}}, {{maxw}} collect the size the element is allowed to expand to.
-------------------------------------------------------------------------------

function Area:askMinMax(m1, m2, m3, m4)
	assert(self:checkFlags(FL_SETUP), "Element not set up")
	local minw = self:getAttr("MinWidth")
	local minh = self:getAttr("MinHeight")
	local maxw = self:getAttr("MaxWidth")
	local maxh = self:getAttr("MaxHeight")
	local p1, p2, p3, p4 = self:getPadding()
	m1 = max(minw, m1 + p1 + p3)
	m2 = max(minh, m2 + p2 + p4)
	m3 = max(min(maxw, m3 + p1 + p3), m1)
	m4 = max(min(maxh, m4 + p2 + p4), m2)
	local ma1, ma2, ma3, ma4 = self:getMargin()
	m1 = m1 + ma1 + ma3
	m2 = m2 + ma2 + ma4
	m3 = m3 + ma1 + ma3
	m4 = m4 + ma2 + ma4
	self.MinMax:setRect(m1, m2, m3, m4)
	return m1, m2, m3, m4
end

-------------------------------------------------------------------------------
--	changed = Area:layout(x0, y0, x1, y1[, markdamage]): Layouts the element
--	into the specified rectangle. If the element's (or any of its childrens')
--	coordinates change, returns '''true''' and marks the element as damaged,
--	unless the optional argument {{markdamage}} is set to '''false'''.
-------------------------------------------------------------------------------

function Area:layout(x0, y0, x1, y1, markdamage)

	local r = self.Rect
	local m1, m2, m3, m4 = self:getMargin()

	x0 = x0 + m1
	y0 = y0 + m2
	x1 = x1 - m3
	y1 = y1 - m4

	local r1, r2, r3, r4 = r:get()
	if r1 == x0 and r2 == y0 and r3 == x1 and r4 == y1 then
		-- nothing changed:
		return
	end

	r:setRect(x0, y0, x1, y1)
	self:setFlags(FL_LAYOUT)
	markdamage = markdamage ~= false

	if not r1 then
		-- this is the first layout:
		if markdamage then
			self:setFlags(FL_REDRAW)
		end
		return true
	end

	-- delta shift, delta size:
	local dx = x0 - r1
	local dy = y0 - r2
	local dw = x1 - x0 - r3 + r1
	local dh = y1 - y0 - r4 + r2

	-- get element transposition:
	local d = self.Window.Drawable
	local sx, sy = d:setShift()
	local win = self.Window

	-- get window/cliprect
	local ww, wh = d:getAttrs("wh")
	local c1, c2, c3, c4 = d:getClipRect()
	if c1 then
		c1, c2, c3, c4 = intersect(c1, c2, c3, c4, 0, 0, ww - 1, wh - 1)
	else
		c1, c2, c3, c4 = 0, 0, ww - 1, wh - 1
	end
	
	local samesize = dw == 0 and dh == 0
	local validmove = (dx == 0) ~= (dy == 0)
	local trackdamage = self:checkFlags(FL_TRACKDAMAGE)

	-- refresh element by copying if:
	-- * shifting occurs only on one axis
	-- * size is unchanged OR TrackDamage enabled
	-- * element is not already slated for copying
	-- * element's background is position-independent
	
	if validmove and (samesize or trackdamage) and
		not self:checkFlags(FL_DONOTBLIT) and
		not win.BlitObjects[self] then
		local _, _, _, pos_independent = self:getBG()
		if pos_independent then
			
			-- get source rect, incl. border:
			local s1 = x0 - dx - m1
			local s2 = y0 - dy - m2
			local s3 = x1 - dx + m3
			local s4 = y1 - dy + m4
			
			local r = newregion(r1 + sx - m1, r2 + sy - m2, r3 + sx + m3, 
				r4 + sy + m4)
			r:subRect(c1, c2, c3, c4)
			r:shift(dx, dy)
			r:andRect(c1, c2, c3, c4)
			
			if r:isEmpty() then
				-- completely visible before and after:
				win.BlitObjects[self] = true
				win:addBlit(s1 + sx, s2 + sy, s3 + sx, s4 + sy, dx, dy,
					c1, c2, c3, c4)
				if samesize then
					-- something changed, no redraw. second value: border_ok
					return true, true
				end
				-- move oldrect to new pos
				r1 = r1 + dx
				r2 = r2 + dy
				r3 = r3 + dx
				r4 = r4 + dy
			end
			
		end
	end
	
	if trackdamage and markdamage then
		-- if damage is to be marked and can be tracked:
		if x0 == r1 and y0 == r2 then
			-- did not move, size changed:
			local r = newregion(x0, y0, x1, y1):subRect(r1, r2, r3, r4)
			-- clip new damage through current cliprect, corrected by shift:
			r:andRect(c1 - sx, c2 - sy, c3 - sx, c4 - sy)
			r:forEach(self.damage, self)
		else
			self:damage(x0, y0, x1, y1)
		end
	end

	if markdamage then
		self:setFlags(FL_REDRAW)
	end
	return true
end

-------------------------------------------------------------------------------
--	Area:punch(region): Subtracts the element from (by punching a hole into)
--	the specified Region. This function is called by the layouter.
-------------------------------------------------------------------------------

function Area:punch(region)
	region:subRegion(self.Rect)
end

-------------------------------------------------------------------------------
--	Area:damage(x0, y0, x1, y1): If the element overlaps with the given
--	rectangle, this function marks it as damaged by setting {{ui.FL_REDRAW}}
--	in the element's {{Flag}} field. Additionally, if the element's
--	{{FL_TRACKDAMAGE}} flag is set, intra-area damage rectangles are
--	collected in {{DamageRegion}}.
-------------------------------------------------------------------------------

function Area:damage(r1, r2, r3, r4)
	if self:checkFlags(FL_LAYOUT + FL_SHOW) then
		local s1, s2, s3, s4 = self:getRect()
		r1, r2, r3, r4 = intersect(r1, r2, r3, r4, s1, s2, s3, s4)
		local trackdamage = self:checkFlags(FL_TRACKDAMAGE)
		if r1 and (trackdamage or not self:checkFlags(FL_REDRAW)) then
			local dr = self.DamageRegion
			if dr then
				dr:orRect(r1, r2, r3, r4)
			elseif trackdamage then
				self.DamageRegion = newregion(r1, r2, r3, r4)			
			end
			self:setFlags(FL_REDRAW)
		end
	end
end

-------------------------------------------------------------------------------
--	success = Area:draw(): If the element is marked for a repaint (indicated
--	by the presence of the flag {{ui.FL_REDRAW}} in the {{Flags}} field),
--	draws the element into the rectangle that was assigned to it by the
--	layouter, clears {{ui.FL_REDRAW}}, and returns '''true'''. If the
--	flag {{FL_ERASEBG}} is set, this function also clears the element's
--	background by calling Area:erase().
--
--	When overriding this function, the control flow is roughly as follows:
--
--			function ElementClass:draw()
--			  if SuperClass.draw(self) then
--			    -- your rendering here
--			    return true
--			  end
--			end
--
--	There are rare occasions in which a class modifies the drawing context,
--	e.g. by setting a coordinate displacement. Such modifications must
--	be performed in Area:drawBegin() and undone in Area:drawEnd(). In this
--	case, the control flow looks like this:
--
--			function ElementClass:draw()
--			  if SuperClass.draw(self) and self:drawBegin() then
--			    -- your rendering here
--			    self:drawEnd()
--			    return true
--			  end
--			end
--
-------------------------------------------------------------------------------

local FL_REDRAW_OK = FL_LAYOUT + FL_SHOW + FL_SETUP + FL_REDRAW

function Area:draw()
	assert(not self.Invisible)
	-- check layout, show, setup, redraw, and clear redraw:
	if self:checkClearFlags(FL_REDRAW_OK, FL_REDRAW) then
		if self:checkFlags(FL_ERASEBG) and self:drawBegin() then
			self:erase()
			self:drawEnd()
		end
		self.DamageRegion = false
		return true
	end
end

-------------------------------------------------------------------------------
--	bgpen, tx, ty, pos_independent = Area:getBG(): Gets the element's
--	background properties. {{bgpen}} is the background paint (which may be a
--	solid color, texture, or gradient). If the element's
--	''background-attachment'' is not {{"fixed"}}, then {{tx}} and {{ty}} are
--	the coordinates of the texture origin. {{pos_independent}} indicates
--	whether the background is independent of the element's position.
-------------------------------------------------------------------------------

function Area:getBG()
	local scrollable = false
	local p = self.Properties
	local bgpen, tx, ty = self.BGPen
	if not bgpen or 
		p["background-color" .. self:getPseudoClass()] == "transparent" then
		bgpen, tx, ty = self:getParent():getBG()
	else
		scrollable = p["background-attachment"] ~= "fixed"
		if scrollable then
			tx, ty = self:getRect()
		end
	end
	return bgpen, tx, ty, self.Window:isSolidPen(bgpen) or scrollable
end

-------------------------------------------------------------------------------
--	element = Area:getBGElement(): Returns the element that is responsible for
--	painting the surroundings (or the background) of the element. This
--	information is useful for painting transparent or translucent parts of
--	the element, e.g. an inactive focus border.
-------------------------------------------------------------------------------

function Area:getBGElement()
	return self:getParent():getBGElement()
end

-------------------------------------------------------------------------------
--	Area:erase(): Clears the element's background. This method is invoked by
--	Area:draw() if the {{FL_ERASEBG}} flag is set, and when a repaint is
--	possible and necessary. Area:drawBegin() has been invoked when this
--	function is called, and Area:drawEnd() will be called afterwards.
-------------------------------------------------------------------------------

function Area:erase()
	local d = self.Window.Drawable
	d:setBGPen(self:getBG())
	local dr = self.DamageRegion
	if dr then
		-- repaint intra-area damagerects:
		dr:forEach(d.fillRect, d)
	else
		local r1, r2, r3, r4 = self:getRect()
		d:fillRect(r1, r2, r3, r4)
	end
end

-------------------------------------------------------------------------------
--	self = Area:getByXY(x, y): Returns {{self}} if the element covers
--	the specified coordinate.
-------------------------------------------------------------------------------

function Area:getByXY(x, y)
	local r1, r2, r3, r4 = self:getRect()
	if r1 and x >= r1 and x <= r3 and y >= r2 and y <= r4 then
		return self
	end
end

-------------------------------------------------------------------------------
--	msg = Area:passMsg(msg): If the element has the ui.FL_RECVINPUT flag set,
--	this function receives input messages. Additionally, to receive messages
--	of the type ui.MSG_MOUSEMOVE, the flag ui.FL_RECVMOUSEMOVE must be set.
--	After processing, it is free to return the message unmodified (thus
--	allowing it to be passed on to other elements), or to absorb the message
--	by returning '''false'''. You are not allowed to modify any data
--	inside the original message; if you alter it, you must return a copy.
--	The message types ui.MSG_INTERVAL, ui.MSG_USER, and ui.MSG_REQSELECTION
--	are not received by this function. To receive these, you must register an
--	input handler using Window:addInputHandler().
-------------------------------------------------------------------------------

function Area:passMsg(msg)
	return msg
end

-------------------------------------------------------------------------------
--	Area:setState(bg): Sets the {{BGPen}} attribute according to
--	the state of the element, and if it changed, marks the element
--	for repainting.
-------------------------------------------------------------------------------

function Area:setState(bg, fg)
	local props = self.Properties
	bg = bg or props["background-color"] or "background"
	if bg == "transparent" then
		bg = self:getBGElement().BGPen
	end
	if bg ~= self.BGPen then
		self.BGPen = bg
		self:setFlags(FL_REDRAW)
	end
end

-------------------------------------------------------------------------------
--	can_receive = Area:checkFocus(): Returns '''true''' if this element can
--	receive the input focus.
-------------------------------------------------------------------------------

function Area:checkFocus()
end

-------------------------------------------------------------------------------
--	can_hilite = Area:checkHilite(): Returns '''true''' if this element can
--	be highlighted, e.g by being hovered over with the pointing device.
-------------------------------------------------------------------------------

function Area:checkHilite()
end

-------------------------------------------------------------------------------
--	element = Area:getNext([mode]): Returns the next element in its group.
--	If the element has no successor and the optional argument {{mode}} is
--	{{"recursive"}}, returns the next element in the parent group (and so
--	forth, until it reaches the topmost group).
-------------------------------------------------------------------------------

local function findelement(self)
	local g = self:getSiblings()
	if g then
		local n = #g
		for i = 1, n do
			if g[i] == self then
				return g, n, i
			end
		end
	end
end

function Area:getNext(mode)
	local g, n, i = findelement(self)
	if g then
		if i == n then
			if mode == "recursive" then
				return self:getParent():getNext(mode)
			end
			return false
		end
		return g[i % n + 1]
	end
end

-------------------------------------------------------------------------------
--	element = Area:getPrev([mode]): Returns the previous element in its group.
--	If the element has no predecessor and the optional argument {{mode}} is
--	{{"recursive"}}, returns the previous element in the parent group (and so
--	forth, until it reaches the topmost group).
-------------------------------------------------------------------------------

function Area:getPrev(mode)
	local g, n, i = findelement(self)
	if g then
		if i == 1 then
			if mode == "recursive" then
				return self:getParent():getPrev(mode)
			end
			return false
		end
		return g[(i - 2) % n + 1]
	end
end

-------------------------------------------------------------------------------
--	element = Area:getGroup(parent): Returns the closest
--	[[#tek.ui.class.group : Group]] containing the element. If the {{parent}}
--	argument is '''true''', this function will start looking for the closest
--	group at its parent - otherwise, the element itself is returned if it is
--	a group already. Returns '''nil''' if the element is not currently
--	connected.
-------------------------------------------------------------------------------

function Area:getGroup()
	local p = self:getParent()
	if p then
		if p:getGroup() == p then
			return p
		end
		return p:getGroup(true)
	end
end

-------------------------------------------------------------------------------
--	table = Area:getSiblings(): Returns a table containing the element's
--	siblings, which includes the element itself. Returns '''nil''' if the
--	element is not currently connected. Note: The returned table must be
--	treated read-only.
-------------------------------------------------------------------------------

function Area:getSiblings()
	local p = self:getParent()
	return p and p:getChildren()
end

-------------------------------------------------------------------------------
--	element = Area:getParent(): Returns the element's parent element, or
--	'''false''' if it currently has no parent.
-------------------------------------------------------------------------------

function Area:getParent()
	return self.Parent
end

-------------------------------------------------------------------------------
--	element = Area:getChildren([mode]): Returns a table containing the
--	element's children, or '''nil''' if this element cannot have children.
--	The optional argument {{mode}} is the string {{"init"}} when this function
--	is called during initialization or deinitialization.
-------------------------------------------------------------------------------

function Area:getChildren()
end

-------------------------------------------------------------------------------
--	x0, y0, x1, y1 = Area:getRect(): This function returns the
--	rectangle which the element has been layouted to, or '''nil'''
--	if the element has not been layouted yet.
-------------------------------------------------------------------------------

function Area:getRect()
	if self:checkFlags(FL_LAYOUT + FL_SHOW + FL_SETUP) then
		return self.Rect:get()
	end
end

-------------------------------------------------------------------------------
--	moved = Area:focusRect([x0, y0, x1, y1]): Tries to shift any Canvas
--	containing the element into a position that makes the element fully
--	visible. Optionally, a rectangle can be specified that is to be made
--	visible. Returns '''true''' to indicate that some kind of repositioning
--	has taken place.
-------------------------------------------------------------------------------

function Area:focusRect(r1, r2, r3, r4)
	if not r1 then
		r1, r2, r3, r4 = self:getRect()
	end
	local parent = self:getParent()
	if r1 and parent then
		local m1, m2, m3, m4 = self:getMargin()
		return parent:focusRect(r1 - m1, r2 - m2, r3 - m3, r4 - m4)
	end
end

-------------------------------------------------------------------------------
--	can_draw = Area:drawBegin(): Prepares the drawing context, returning a
--	boolean indicating success. This function must be overridden if a class
--	wishes to modify the drawing context, e.g. for installing a coordinate
--	displacement.
-------------------------------------------------------------------------------

function Area:drawBegin()
	return self:checkFlags(FL_LAYOUT + FL_SHOW + FL_SETUP)
end

-------------------------------------------------------------------------------
--	Area:drawEnd(): Reverts the changes made to the drawing context during
--	Area:drawBegin().
-------------------------------------------------------------------------------

function Area:drawEnd()
end

-------------------------------------------------------------------------------
--	left, top, right, bottom = Area:getPadding(): Returns the element's
--	padding style properties.
-------------------------------------------------------------------------------

function Area:getPadding()
	local p = self.Properties
	return p["padding-left"] or 0, p["padding-top"] or 0,
		p["padding-right"] or 0, p["padding-bottom"] or 0
end

-------------------------------------------------------------------------------
--	left, top, right, bottom = Area:getMargin(): Returns the element's
--	margins in the order left, top, right, bottom.
-------------------------------------------------------------------------------

function Area:getMargin()
	local p = self.Properties
	return p["margin-left"] or 0, p["margin-top"] or 0, 
		p["margin-right"] or 0, p["margin-bottom"] or 0
end

-------------------------------------------------------------------------------
--	minx, miny, maxx, maxy = Area:getMinMax(): Returns the element's
--	calculated minimum and maximum size requirements, which are available
--	after Area:askMinMax().
-------------------------------------------------------------------------------

function Area:getMinMax()
	return self.MinMax:get()
end

-------------------------------------------------------------------------------
--	onSetStyle: overrides
-------------------------------------------------------------------------------

function Area:onSetStyle()
	if not self:checkFlags(FL_SETUP) then
		db.warn("setstyle on uninitialized element")
	end
	Element.onSetStyle(self)
	self:setState()
	self:rethinkLayout(2, true)
end

-------------------------------------------------------------------------------
--	setFlags(flags): Set element's flags. The flags {{FL_REDRAW}},
--	{{FL_CHANGED}}, and {{FL_REDRAWBORDER}} will additionally cause the flag
--	{{FL_UPDATE}} to be set, which will also bubble up in the element
--	hierarchy until it reaches to topmost element.
-------------------------------------------------------------------------------

function Area:setFlags(flags)
	local f = bor(self.Flags, flags)
	self.Flags = f
	-- any of the bubbling flags set, update not already set?
	if band(flags, FL_BUBBLEUP) ~= 0 and band(f, FL_UPDATE) == 0 then
		-- bubble up UPDATE
		local e = self
		repeat
			e.Flags = bor(e.Flags, FL_UPDATE)
			e = e:getParent()
		until not e
	end
end

-------------------------------------------------------------------------------
--	all_present = Area:checkFlags(flags): Check for presence of all of the
--	specified {{flags}} in the element.
-------------------------------------------------------------------------------

function Area:checkFlags(flags)
	return band(self.Flags, flags) == flags
end

-------------------------------------------------------------------------------
--	checkClearFlags(chkflags, clrflags)
-------------------------------------------------------------------------------

function Area:checkClearFlags(chkflags, clrflags)
	local f = self.Flags
	self.Flags = band(f, bnot(clrflags or chkflags))
	return band(f, chkflags) == chkflags
end

-------------------------------------------------------------------------------
--	... = Area:getMsgFields(msg, field): Get specified field(s) from an input
--	message. Possible fields include:
--	* {{"mousexy"}}: Returns the message's mouse coordinates (x, y)
-------------------------------------------------------------------------------

function Area:getMsgFields(msg, mode)
	if mode == "mousexy" then
		local mx, my = msg[4], msg[5]
		local e = self:getParent()
		while e do
			local dx, dy = e:getDisplacement()
			mx = mx - dx
			my = my - dy
			e = e:getParent()
		end
		return mx, my
	end
end

-------------------------------------------------------------------------------
--	dx, dy = Area:getDisplacement(): Get the element's coordinate displacement
-------------------------------------------------------------------------------

function Area:getDisplacement()
	return 0, 0
end

-------------------------------------------------------------------------------
--	beginPopup([baseitem]): Prepare element for being used in a popup.
-------------------------------------------------------------------------------

function Area:beginPopup(baseitem)
	self.Hilite = false
	self.Focus = false
end

-------------------------------------------------------------------------------
--	reconfigure:
-------------------------------------------------------------------------------

function Area:reconfigure()
	self.DamageRegion = false
	self.MinMax = newregion()
	self.Rect = newregion()
	self.BGPen = false
	self:setState()
	self:checkClearFlags(FL_LAYOUT)
	self:rethinkLayout(2, true)
end

-------------------------------------------------------------------------------
--	getAttr:
-------------------------------------------------------------------------------

local attrs = {
	Width = function(self)
		local w = self.Width or self.Properties["width"]
		return tonumber(w) or w or false
	end,
	Height = function(self)
		local h = self.Height or self.Properties["height"]
		return tonumber(h) or h or false
	end,
	HAlign = function(self)
		return self.HAlign or self.Properties["halign"] or false
	end,
	VAlign = function(self)
		return self.VAlign or self.Properties["valign"] or false
	end,
	MinWidth = function(self)
		local props = self.Properties
		local minw = self.MinWidth or props["min-width"]
		return tonumber(minw) or tonumber(self.Width or props["width"]) or 0
	end,
	MinHeight = function(self)
		local props = self.Properties
		local minh = self.MinHeight or props["min-height"]
		return tonumber(minh) or tonumber(self.Height or props["height"]) or 0
	end,
	MaxWidth = function(self)
		local props = self.Properties
		local maxw = self.MaxWidth or props["max-width"]
		local w = self.Width or props["width"]
		if w == "auto" then
			maxw = 0
		elseif maxw == "none" then
			maxw = HUGE
		end
		return tonumber(maxw) or tonumber(w) or HUGE
	end,
	MaxHeight = function(self)
		local props = self.Properties
		local maxh = self.MaxHeight or props["max-height"]
		local h = self.Height or props["height"]
		if h == "auto" then
			maxh = 0
		elseif maxh == "none" then
			maxh = HUGE
		end
		return tonumber(maxh) or tonumber(h) or HUGE
	end,
}

function Area:getAttr(attr, ...)
	if attrs[attr] then
		return attrs[attr](self)
	end
	return Element.getAttr(self, attr, ...)
end


function Area:onSetInvisible()
	if self.Invisible then
		self:checkClearFlags(FL_SHOW)
	else
		self:setFlags(FL_SHOW)
	end
	self:getGroup():rethinkLayout(2, true)
end

return Area
