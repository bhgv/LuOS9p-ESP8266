-------------------------------------------------------------------------------
--
--	tek.ui.class.canvas
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	OVERVIEW::
--		[[#ClassOverview]] :
--		[[#tek.class : Class]] /
--		[[#tek.class.object : Object]] /
--		[[#tek.ui.class.element : Element]] /
--		[[#tek.ui.class.area : Area]] /
--		[[#tek.ui.class.area : Frame]] /
--		Canvas ${subclasses(Canvas)}
--
--		This class implements a scrollable area acting as a managing container
--		for a child element. Currently, this class is used exclusively for
--		child objects of the [[#tek.ui.class.scrollgroup : ScrollGroup]] class.
--
--	ATTRIBUTES::
--		- {{AutoPosition [I]}} (boolean)
--			See [[#tek.ui.class.area : Area]]
--		- {{AutoHeight [IG]}} (boolean)
--			The height of the canvas is automatically adapted to the height
--			of the region it is layouted into. Default: '''false'''
--		- {{AutoWidth [IG]}} (boolean)
--			The width of the canvas is automatically adapted to the width
--			of the canvas it is layouted into. Default: '''false'''
--		- {{CanvasHeight [ISG]}} (number)
--			The height of the canvas in pixels
--		- {{CanvasLeft [ISG]}} (number)
--			Left visible offset of the canvas in pixels
--		- {{CanvasTop [ISG]}} (number)
--			Top visible offset of the canvas in pixels
--		- {{CanvasWidth [ISG]}} (number)
--			The width of the canvas in pixels
--		- {{Child [ISG]}} (object)
--			The child element being managed by the Canvas
--		- {{KeepMinHeight [I]}} (boolean)
--			Report the minimum height of the Canvas' child object as the
--			Canvas' minimum display height. The boolean will be translated
--			to the flag {{FL_KEEPMINHEIGHT}}, and is meaningless after
--			initialization.
--		- {{KeepMinWidth [I]}} (boolean)
--			Translates to the flag {{FL_KEEPMINWIDTH}}. See also
--			{{KeepMinHeight}}, above.
--		- {{UnusedRegion [G]}} ([[#tek.lib.region : Region]])
--			Region of the Canvas which is not covered by its {{Child}}
--		- {{UseChildBG [IG]}} (boolean)
--			If '''true''', the Canvas borrows its background properties from
--			its child for rendering its {{UnusedRegion}}. If '''false''',
--			the Canvas' own background properties are used. Default: '''true'''
--		- {{VIncrement [IG]}} (number)
--			Vertical scroll step, used e.g. for mouse wheels
--
--	IMPLEMENTS::
--		- Canvas:damageChild() - Damages a child object where it is visible
--		- Canvas:onSetChild() - Handler called when {{Child}} is set
--		- Canvas:updateUnusedRegion() - Updates region not covered by Child
--
--	OVERRIDES::
--		- Object.addClassNotifications()
--		- Area:askMinMax()
--		- Element:cleanup()
--		- Element:connect()
--		- Area:damage()
--		- Element:decodeProperties()
--		- Element:disconnect()
--		- Area:draw()
--		- Area:drawBegin()
--		- Area:drawEnd()
--		- Element:erase()
--		- Area:focusRect()
--		- Area:getBG()
--		- Area:getBGElement()
--		- Area:getByXY()
--		- Area:getChildren()
--		- Area:getDisplacement()
--		- Area:hide()
--		- Area:layout()
--		- Class.new()
--		- Area:passMsg()
--		- Element:setup()
--		- Area:show()
--
-------------------------------------------------------------------------------

local db = require "tek.lib.debug"
local ui = require "tek.ui".checkVersion(112)
local Application = ui.require("application", 29)
local Area = ui.require("area", 56)
local Element = ui.require("element", 16)
local Frame = ui.require("frame", 16)
local Region = ui.loadLibrary("region", 9)
local bor = ui.bor
local floor = math.floor
local max = math.max
local min = math.min
local intersect = Region.intersect
local tonumber = tonumber

local Canvas = Frame.module("tek.ui.class.canvas", "tek.ui.class.frame")
Canvas._VERSION = "Canvas 37.6"

-------------------------------------------------------------------------------
--	constants & class data:
-------------------------------------------------------------------------------

local FL_REDRAW = ui.FL_REDRAW
local FL_UPDATE = ui.FL_UPDATE
local FL_RECVINPUT = ui.FL_RECVINPUT
local FL_RECVMOUSEMOVE = ui.FL_RECVMOUSEMOVE
local FL_AUTOPOSITION = ui.FL_AUTOPOSITION
local FL_KEEPMINWIDTH = ui.FL_KEEPMINWIDTH
local FL_KEEPMINHEIGHT = ui.FL_KEEPMINHEIGHT

-------------------------------------------------------------------------------
--	addClassNotifications: overrides
-------------------------------------------------------------------------------

function Canvas.addClassNotifications(proto)
	Canvas.addNotify(proto, "Child", ui.NOTIFY_ALWAYS, { ui.NOTIFY_SELF, "onSetChild" })
	return Frame.addClassNotifications(proto)
end

Canvas.ClassNotifications = Canvas.addClassNotifications { Notifications = { } }

-------------------------------------------------------------------------------
--	init: overrides
-------------------------------------------------------------------------------

function Canvas.new(class, self)
	self = self or { }
	if self.AutoPosition == nil then
		self.AutoPosition = false
	end
	self.AutoHeight = self.AutoHeight or false
	self.AutoWidth = self.AutoWidth or false
	self.CanvasHeight = tonumber(self.CanvasHeight) or 0
	self.CanvasLeft = tonumber(self.CanvasLeft) or 0
	self.CanvasTop = tonumber(self.CanvasTop) or 0
	self.CanvasWidth = tonumber(self.CanvasWidth) or 0
	self.NullArea = Area:new { MaxWidth = 0, MinWidth = 0 }
	self.Child = self.Child or self.NullArea
	local flags = 0
	if self.KeepMinWidth then
		flags = bor(flags, FL_KEEPMINWIDTH)
	end
	if self.KeepMinHeight then
		flags = bor(flags, FL_KEEPMINHEIGHT)
	end
	self.Flags = bor(self.Flags or 0, flags)
	self.OldCanvasLeft = self.CanvasLeft
	self.OldCanvasTop = self.CanvasTop
	self.OldChild = self.Child
	self.ShiftX = 0
	self.ShiftY = 0
	self.TempMsg = { }
	-- track intra-area damages, so that they can be applied to child object:
	self.TrackDamage = true
	self.UnusedRegion = false
	if self.UseChildBG == nil then
		self.UseChildBG = true
	end
	self.VIncrement = self.VIncrement or 10
	return Frame.new(class, self)
end

-------------------------------------------------------------------------------
--	connect: overrides
-------------------------------------------------------------------------------

function Canvas:connect(parent)
	-- this connects recursively:
	Application.connect(self.Child, self)
	self.Child:connect(self)
	return Frame.connect(self, parent)
end

-------------------------------------------------------------------------------
--	disconnect: overrides
-------------------------------------------------------------------------------

function Canvas:disconnect(parent)
	Frame.disconnect(self, parent)
	return Element.disconnect(self.Child, parent)
end

-------------------------------------------------------------------------------
--	setup: overrides
-------------------------------------------------------------------------------

function Canvas:setup(app, window)
	Frame.setup(self, app, window)
	self:setFlags(FL_RECVINPUT + FL_RECVMOUSEMOVE)
	self.Child:setup(app, window)
end

-------------------------------------------------------------------------------
--	cleanup: overrides
-------------------------------------------------------------------------------

function Canvas:cleanup()
	self.Child:cleanup()
	self:checkClearFlags(0, FL_RECVINPUT + FL_RECVMOUSEMOVE)
	Frame.cleanup(self)
end

-------------------------------------------------------------------------------
--	decodeProperties: overrides
-------------------------------------------------------------------------------

function Canvas:decodeProperties(p)
	self.Child:decodeProperties(p)
	Frame.decodeProperties(self, p)
end

-------------------------------------------------------------------------------
--	show: overrides
-------------------------------------------------------------------------------

function Canvas:show()
	self.Child:show()
	Frame.show(self)
end

-------------------------------------------------------------------------------
--	hide: overrides
-------------------------------------------------------------------------------

function Canvas:hide()
	self.Child:hide()
	Frame.hide(self)
end

-------------------------------------------------------------------------------
--	askMinMax: overrides
-------------------------------------------------------------------------------

function Canvas:askMinMax(m1, m2, m3, m4)
	local maxw = self:getAttr("MaxWidth")
	local maxh = self:getAttr("MaxHeight")
	local c1, c2, c3, c4 = self.Child:askMinMax(0, 0, maxw, maxh)
	m1 = m1 + c1
	m2 = m2 + c2
	m3 = m3 + c3
	m4 = m4 + c4
	m1 = self:checkFlags(FL_KEEPMINWIDTH) and m1 or 0
	m2 = self:checkFlags(FL_KEEPMINHEIGHT) and m2 or 0
	m3 = maxw and max(maxw, m1) or self.CanvasWidth
	m4 = maxh and max(maxh, m2) or self.CanvasHeight
	return Frame.askMinMax(self, m1, m2, m3, m4)
end

-------------------------------------------------------------------------------
--	layout: overrides
-------------------------------------------------------------------------------

local function markchilddamage(child, r1, r2, r3, r4, sx, sy)
	child:damage(r1 + sx, r2 + sy, r3 + sx, r4 + sy)
end

function Canvas:layout(r1, r2, r3, r4, markdamage)

	local sizechanged
	local m1, m2, m3,m4 = self:getMargin()
	local s1, s2, s3, s4 = self:getRect()
	local c = self.Child
	local mm1, mm2, mm3, mm4 = c:getMinMax()
	local res = Frame.layout(self, r1, r2, r3, r4, markdamage)

	if self.AutoWidth then
		local w = r3 - r1 + 1 - m1 - m3
		w = max(w, mm1)
		w = mm3 and min(w, mm3) or w
		if w ~= self.CanvasWidth then
			self:setValue("CanvasWidth", w)
			sizechanged = true
		end
	end

	if self.AutoHeight then
		local h = r4 - r2 + 1 - m2 - m4
		h = max(h, mm2)
		h = mm4 and min(h, mm4) or h
		if h ~= self.CanvasHeight then
			self:setValue("CanvasHeight", h)
			sizechanged = true
		end
	end

	if self:drawBegin() then
	
		-- layout child until width and height settle in:
		-- TODO: break out if they don't settle in?
		local iw, ih
		repeat
			iw, ih = self.CanvasWidth, self.CanvasHeight
			if c:layout(0, 0, iw - 1, ih - 1, sizechanged) then
				sizechanged = true
			end
		until self.CanvasWidth == iw and self.CanvasHeight == ih
		
		self:drawEnd()
	
	end

	-- propagate intra-area damages calculated in Frame.layout to child object:
	local dr = self.DamageRegion
	if dr and markdamage ~= false then
		local sx = self.CanvasLeft - s1
		local sy = self.CanvasTop - s2
		-- mark as damage shifted into canvas space:
		dr:forEach(markchilddamage, c, sx, sy)
	end

	if res or sizechanged or not self.UnusedRegion then
		self:updateUnusedRegion()
	end

	if res or sizechanged then
		self:setFlags(FL_REDRAW)
		return true
	end

end

-------------------------------------------------------------------------------
--	updateUnusedRegion(): Updates the {{UnusedRegion}} attribute, which
--	contains the Canvas' area which isn't covered by its {{Child}}.
-------------------------------------------------------------------------------

function Canvas:updateUnusedRegion()
	-- determine unused region:
	local r1, r2, r3, r4 = self:getRect()
	if r1 then
		local ur = (self.UnusedRegion or Region.new()):setRect(0, 0,
			max(r3 - r1, self.CanvasWidth - 1),
			max(r4 - r2, self.CanvasHeight - 1))
		self.UnusedRegion = ur
		local c = self.Child
		r1, r2, r3, r4 = c:getRect()
		if r1 then
			local props = c.Properties
			local b1 = tonumber(props["margin-left"]) or 0
			local b2 = tonumber(props["margin-top"]) or 0
			local b3 = tonumber(props["margin-right"]) or 0
			local b4 = tonumber(props["margin-bottom"]) or 0
			local m1, m2, m3, m4 = c:getMargin()
			ur:subRect(r1 - m1 + b1, r2 - m2 + b2, r3 + m3 - b3, r4 + m4 - b4)
			self:setFlags(FL_REDRAW)
		end
	end
end

-------------------------------------------------------------------------------
--	erase: overrides
-------------------------------------------------------------------------------

function Canvas:erase()
	local d = self.Window.Drawable
	d:setBGPen(self:getBG())
	self.UnusedRegion:forEach(d.fillRect, d)
end

-------------------------------------------------------------------------------
--	draw: overrides
-------------------------------------------------------------------------------

function Canvas:draw()
	local res = Frame.draw(self)
	if self.Child:checkClearFlags(FL_UPDATE) then
		if self:drawBegin() then
			self.Child:draw()
			self:drawEnd()
		end
	end
	return res
end

-------------------------------------------------------------------------------
--	getBG: overrides
-------------------------------------------------------------------------------

function Canvas:getBG()
	local cx, cy
	if self.UseChildBG and self.Child then
		local bgpen = self.Child.BGPen
		if bgpen then
			if self.Properties["background-attachment"] ~= "fixed" then
				cx, cy = self:getRect()
				local m1, m2 = self:getMargin()
				cx = cx - m1
				cy = cy - m2
			end
			return bgpen, cx, cy, self.Window:isSolidPen(bgpen)
		end
	end
	cx, cy = self:getRect()
	local bgpen, tx, ty, pos_independent = Frame.getBG(self)
	return bgpen, (tx or 0) - cx, (ty or 0) - cy, pos_independent
end

-------------------------------------------------------------------------------
--	damage: overrides
-------------------------------------------------------------------------------

function Canvas:damage(r1, r2, r3, r4)
	Frame.damage(self, r1, r2, r3, r4)
	-- clip absolute:
	local s1, s2, s3, s4 = self:getRect()
	if s1 then
		r1, r2, r3, r4 = intersect(r1, r2, r3, r4, s1, s2, s3, s4)
		if r1 then
			-- shift into canvas space:
			local sx = self.CanvasLeft - s1
			local sy = self.CanvasTop - s2
			self.Child:damage(r1 + sx, r2 + sy, r3 + sx, r4 + sy)
		end
	end
end

-------------------------------------------------------------------------------
--	damageChild(r1, r2, r3, r4): Damages the specified region in the
--	child area where it overlaps with the visible part of the canvas.
-------------------------------------------------------------------------------

function Canvas:damageChild(r1, r2, r3, r4)
	local s1, s2, s3, s4 = self:getRect()
	if s1 then
		local x = self.CanvasLeft
		local y = self.CanvasTop
		local w = min(self.CanvasWidth, s3 - s1 + 1)
		local h = min(self.CanvasHeight, s4 - s2 + 1)
		r1, r2, r3, r4 = intersect(r1, r2, r3, r4, x, y, x + w - 1, y + h - 1)
		if r1 then
			self.Child:damage(r1, r2, r3, r4)
		end
	end
end

-------------------------------------------------------------------------------
--	sx, sy = checkArea(x, y) - Checks if {{x, y}} are inside the element's
--	rectangle, and if so, returns the canvas shift by x and y
-------------------------------------------------------------------------------

function Canvas:checkArea(x, y)
	local r1, r2, r3, r4 = self:getRect()
	if r1 and x >= r1 and x <= r3 and y >= r2 and y <= r4 then
		return r1 - self.CanvasLeft, r2 - self.CanvasTop
	end
end

-------------------------------------------------------------------------------
--	getByXY: overrides
-------------------------------------------------------------------------------

function Canvas:getByXY(x, y)
	local sx, sy = self:checkArea(x, y)
	return sx and self.Child:getByXY(x - sx, y - sy)
end

-------------------------------------------------------------------------------
--	passMsg: overrides
-------------------------------------------------------------------------------

function Canvas:passMsg(msg)
	local mx, my = self:getMsgFields(msg, "mousexy")
	local isover = self:checkArea(mx, my)
	if isover then
		if msg[2] == ui.MSG_MOUSEBUTTON then
			local r1, r2, r3, r4 = self:getRect()
			local h = self.CanvasHeight - (r4 - r2 + 1)
			if msg[3] == 64 then -- wheelup
				self:setValue("CanvasTop",
					max(0, min(h, self.CanvasTop - self.VIncrement)))
				return false -- absorb
			elseif msg[3] == 128 then -- wheeldown
				self:setValue("CanvasTop",
					max(0, min(h, self.CanvasTop + self.VIncrement)))
				return false -- absorb
			end
		elseif msg[2] == ui.MSG_KEYDOWN then
			local _, r2, _, r4 = self:getRect()
			local h = r4 - r2 + 1
			if msg[3] == 0xf023 then -- PgUp
				self:setValue("CanvasTop", self.CanvasTop - h)
			elseif msg[3] == 0xf024 then -- PgDown
				self:setValue("CanvasTop", self.CanvasTop + h)
			end
		end
	end
	return self.Child:passMsg(msg)
end

-------------------------------------------------------------------------------
--	getChildren: overrides
-------------------------------------------------------------------------------

function Canvas:getChildren()
	return { self.Child }
end

-------------------------------------------------------------------------------
--	Canvas:onSetChild(): This handler is invoked when the canvas'
--	{{Child}} element has changed.
-------------------------------------------------------------------------------

function Canvas:onSetChild()
	local child = self.Child
	local oldchild = self.OldChild
	if oldchild then
		if oldchild == self.Window.FocusElement then
			self.Window:setFocusElement()
		end
		oldchild:hide()
		oldchild:cleanup()
	end
	child = child or self.NullArea
	self.Child = child
	self.OldChild = child
	self.Application:decodeProperties(child)
	child:setup(self.Application, self.Window)
	child:show(self.Window.Drawable)
	child:connect(self)
	self:rethinkLayout(2, true)
end

-------------------------------------------------------------------------------
--	focusRect - overrides
-------------------------------------------------------------------------------

function Canvas:focusRect(x0, y0, x1, y1, smooth)
	local r1, r2, r3, r4 = self:getRect()
	if not r1 then
		return
	end
	local vw = r3 - r1
	local vh = r4 - r2
	local vx0 = self.CanvasLeft
	local vy0 = self.CanvasTop
	local vx1 = vx0 + vw
	local vy1 = vy0 + vh
	local moved

	-- make fully visible, if possible
	if self.CanvasWidth <= vw then
		x0, x1 = 0, 0
	end
	if self.CanvasHeight <= vh then
		y0, y1 = 0, 0
	end
	
	if x0 and self:checkFlags(FL_AUTOPOSITION) then
		local n1, n2, n3, n4 = intersect(x0, y0, x1, y1, vx0, vy0, vx1, vy1)
		if n1 == x0 and n2 == y0 and n3 == x1 and n4 == y1 then
			return
		end
		if y1 > vy1 then
			vy1 = y1
			vy0 = vy1 - vh
		end
		if y0 < vy0 then
			vy0 = y0
			vy1 = vy0 + vh
		end
		if x1 > vx1 then
			vx1 = x1
			vx0 = vx1 - vw
		end
		if x0 < vx0 then
			vx0 = x0
			vx1 = vx0 + vw
		end
		if smooth and smooth > 0 then
			local nx = floor((vx0 - self.CanvasLeft) / smooth)
			local ny = floor((vy0 - self.CanvasTop) / smooth)
			if nx > 1 or nx < -1 then
				vx0 = self.CanvasLeft + nx
			end
			if ny > 1 or ny < -1 then
				vy0 = self.CanvasTop + ny
			end
		end
		local t = self.CanvasLeft
		self:setValue("CanvasLeft", vx0)
		moved = self.CanvasLeft ~= t
		t = self.CanvasTop
		self:setValue("CanvasTop", vy0)
		if not moved then
			moved = self.CanvasTop ~= t
		end
		vx0 = x0
		vy0 = y0
		vx1 = x1
		vy1 = y1
	end
	local parent = self:getParent()
	if parent then
		local res = parent:focusRect(r1 + vx0, r2 + vy0, r3 + vx1, r4 + vy1)
		if not moved then
			moved = res
		end
	end
	return moved
end

-------------------------------------------------------------------------------
--	getBGElement: overrides
-------------------------------------------------------------------------------

function Canvas:getBGElement()
	return self
end

-------------------------------------------------------------------------------
--	drawBegin: overrides
-------------------------------------------------------------------------------

function Canvas:drawBegin()
	if Frame.drawBegin(self) then
		local r1, r2, r3, r4 = self:getRect()
		local sx, sy = self:getDisplacement()
		self.ShiftX = sx
		self.ShiftY = sy
		local d = self.Window.Drawable
		d:pushClipRect(r1, r2, r3, r4)
		d:setShift(sx, sy)
		return true
	end
end

-------------------------------------------------------------------------------
--	drawEnd: overrides
-------------------------------------------------------------------------------

function Canvas:drawEnd()
	local d = self.Window.Drawable
	d:setShift(-self.ShiftX, -self.ShiftY)
	d:popClipRect()
	Frame.drawEnd(self)
end

-------------------------------------------------------------------------------
--	getDisplacement: overrides
-------------------------------------------------------------------------------

function Canvas:getDisplacement()
	local dx, dy = Frame.getDisplacement(self)
	local r1, r2 = self:getRect()
	if r1 then
		dx = dx + r1
		dy = dy + r2
	end
	dx = dx - self.CanvasLeft
	dy = dy - self.CanvasTop
	return dx, dy
end

-------------------------------------------------------------------------------
--	over, x, y = getMouseOver(msg): From an input message, retrieves the mouse
--	position as seen from the child object. The first return value is 
--	'''false''', {{"child"}} if the mouse is over the child object, or
--	{{"canvas"}} if the mouse is over the canvas.
-------------------------------------------------------------------------------

function Canvas:getMouseOver(msg)
	local c = self.Child
	if c then
		local cx = self.CanvasLeft
		local cy = self.CanvasTop
		local c1, c2, c3, c4 = self:getRect()
		local vw = c3 - c1 + 1
		local vh = c4 - c2 + 1
		local x, y = c:getMsgFields(msg, "mousexy")
		local x0 = x - cx
		local y0 = y - cy
		local is_over = x0 >= 0 and x0 < vw and y0 >= 0 and y0 < vh
		local position = false
		local over = false
		if is_over then
			position = true
			local m1, m2, m3, m4 = c:getMargin()
			if x >= m1 and y >= m2 and x < self.CanvasWidth - m3 and
				y < self.CanvasHeight - m4 then -- in text:
				over = "child"
			else
				over = "canvas"
			end
		end
		return over, x, y
	end
end

-------------------------------------------------------------------------------
--	reconfigure()
-------------------------------------------------------------------------------

function Canvas:reconfigure()
	Frame.reconfigure(self)
	self.Child:reconfigure()
end

return Canvas
