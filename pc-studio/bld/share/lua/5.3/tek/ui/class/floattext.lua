-------------------------------------------------------------------------------
--
--	tek.ui.class.floattext
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	OVERVIEW::
--		[[#ClassOverview]] :
--		[[#tek.class : Class]] /
--		[[#tek.class.object : Object]] /
--		[[#tek.ui.class.element : Element]] /
--		[[#tek.ui.class.area : Area]] /
--		FloatText ${subclasses(FloatText)}
--
--		This class implements a scrollable display of text. An object of
--		this class is normally the immediate child of a
--		[[#tek.ui.class.canvas : Canvas]].
--
--	ATTRIBUTES::
--		- {{FGPen [IG]}} (userdata)
--			Pen for rendering the text. This attribute is controllable via the
--			{{color}} style property.
--		- {{Font [IG]}} (string)
--			Font specifier; see [[#tek.ui.class.text : Text]] for a
--			format description. This attribute is controllable via the
--			{{font}} style property.
--		- {{Preformatted [IG]}} (boolean)
--			Boolean, indicating that the text is already formatted and should
--			not be reformatted to fit the element's width.
--		- {{Text [ISG]]}} (string)
--			The text to be displayed
--
--	IMPLEMENTS::
--		- FloatText:appendLine() - Append a line of text
--		- FloatText:onSetText() - Handler called when {{Text}} is changed
--
--	STYLE PROPERTIES::
--		{{color}} || controls the {{FloatText.FGPen}} attribute
--		{{font}} || controls the {{FloatText.Font}} attribute
--
--	OVERRIDES::
--		- Object.addClassNotifications()
--		- Area:askMinMax()
--		- Element:cleanup()
--		- Area:damage()
--		- Area:draw()
--		- Area:hide()
--		- Area:layout()
--		- Class.new()
--		- Area:setState()
--		- Element:setup()
--		- Area:show()
--
-------------------------------------------------------------------------------

local ui = require "tek.ui".checkVersion(112)

local Frame = ui.require("frame", 22)
local Region = ui.loadLibrary("region", 9)

local concat = table.concat
local insert = table.insert
local max = math.max
local intersect = Region.intersect
local remove = table.remove

local FloatText = Frame.module("tek.ui.class.floattext", "tek.ui.class.frame")
FloatText._VERSION = "FloatText 22.1"

-------------------------------------------------------------------------------
--	constants & class data:
-------------------------------------------------------------------------------

local FL_REDRAW = ui.FL_REDRAW
local FL_LAYOUT = ui.FL_LAYOUT

-------------------------------------------------------------------------------
--	addClassNotifications: overrides
-------------------------------------------------------------------------------

function FloatText.addClassNotifications(proto)
	FloatText.addNotify(proto, "Text", ui.NOTIFY_ALWAYS, { ui.NOTIFY_SELF, "onSetText" })
	return Frame.addClassNotifications(proto)
end

FloatText.ClassNotifications = FloatText.addClassNotifications { Notifications = { } }

-------------------------------------------------------------------------------
--	Class style properties:
-------------------------------------------------------------------------------

FloatText.Properties = {
	["border-top-width"] = 0,
	["border-right-width"] = 0,
	["border-bottom-width"] = 0,
	["border-left-width"] = 0,
}

-------------------------------------------------------------------------------
--	init: overrides
-------------------------------------------------------------------------------

function FloatText.new(class, self)
	self = self or { }
	self.Canvas = false
	self.CanvasHeight = false
	self.FHeight = false
	self.FontHandle = false
	self.FGPen = false
	self.FWidth = false
	self.Lines = false
	self.Preformatted = self.Preformatted or false
	self.Reposition = false
	self.Text = self.Text or ""
	if self.TrackDamage == nil then
		self.TrackDamage = true
	end
	self.WidthsCache = false
	self.WordSpacing = false
	return Frame.new(class, self)
end

-------------------------------------------------------------------------------
--	initFont
-------------------------------------------------------------------------------

function FloatText:initFont()
	local f = self.Application.Display:openFont(self.Properties["font"])
	self.FontHandle = f
	self.FWidth, self.FHeight = f:getTextSize("W")
end

-------------------------------------------------------------------------------
--	setup: overrides
-------------------------------------------------------------------------------

function FloatText:setup(app, window)
	self.Canvas = self:getParent()
	Frame.setup(self, app, window)
	self:initFont()
	self:prepareText()
end

-------------------------------------------------------------------------------
--	cleanup: overrides
-------------------------------------------------------------------------------

function FloatText:cleanup()
	self.Canvas = false
	self.FontHandle = self.Application.Display:closeFont(self.FontHandle)
	Frame.cleanup(self)
end

-------------------------------------------------------------------------------
--	erase: overrides
-------------------------------------------------------------------------------

function FloatText:erase()
	self.Reposition = false
	local dr = self.DamageRegion
	if dr then
		-- repaint intra-area damagerects:
		local d = self.Window.Drawable
		-- determine visible rectangle:
		local _, _, r3 = self:getRect()
		local ca = self.Canvas
		local x0 = ca and ca.CanvasLeft or 0
		local x1 = ca and x0 + ca.CanvasWidth - 1 or r3
		d:setFont(self.FontHandle)
		d:setBGPen(self:getBG())
		dr:forEach(self.drawPatch, self, d, x0, x1, self.FGPen)
	end
end

-------------------------------------------------------------------------------
--	drawPatch: draw a single patch
-------------------------------------------------------------------------------

function FloatText:drawPatch(r1, r2, r3, r4, d, x0, x1, fp)
	d:pushClipRect(r1, r2, r3, r4)
	local lines = self.Lines
	for lnr = 1, #lines do
		local t = lines[lnr]
		-- overlap between damage and line:
		if intersect(r1, r2, r3, r4, x0, t[2], x1, t[4]) then
			-- draw line background:
			d:fillRect(x0, t[2], x1, t[4])
			-- overlap between damage and text:
			if intersect(r1, r2, r3, r4, t[1], t[2], t[3], t[4]) then
				-- draw text:
				d:drawText(t[1], t[2], t[3], t[4], t[5], fp)
			end
		end
	end
	d:popClipRect()
end

-------------------------------------------------------------------------------
--	prepareText: internal
-------------------------------------------------------------------------------

function FloatText:prepareText()
	local f = self.FontHandle
	if f then
		local lw = 0 -- widest width in text
		local w, h
		local i = 0
		local wl = { }
		self.WidthsCache = wl -- cache for word lengths / line lengths
		if self.Preformatted then
			-- determine widths line by line
			for line in self.Text:gmatch("([^\n]*)\n?") do
				w, h = f:getTextSize(line)
				lw = max(lw, w)
				i = i + 1
				wl[i] = w
			end
		else
			-- determine widths word by word
			for spc, word in self.Text:gmatch("(%s*)([^%s]*)") do
				w, h = f:getTextSize(word)
				lw = max(lw, w)
				i = i + 1
				wl[i] = w
			end
			self.WordSpacing = f:getTextSize(" ")
		end
		self.MinWidth, self.MinHeight = lw, h
		return lw, h
	end
end

-------------------------------------------------------------------------------
--	askMinMax: overrides
-------------------------------------------------------------------------------

function FloatText:askMinMax(m1, m2, m3, m4)
	m1 = m1 + self.MinWidth
	m2 = m2 + self.FHeight
	m3 = ui.HUGE
	m4 = ui.HUGE
	return Frame.askMinMax(self, m1, m2, m3, m4)
end

-------------------------------------------------------------------------------
--	layoutText: internal
-------------------------------------------------------------------------------

local function insline(text, line, x, y, tw, so, fh)
	line = line and concat(line, " ")
	insert(text, { x, y, x + tw + so - 1, y + fh - 1, line })
end

function FloatText:layoutText(x, y, width, text)
	text = text or { }
	local line = false
	local fh = self.FHeight
	local tw = 0
	local i = 0
	local wl = self.WidthsCache
	local so = 0
	if self.Preformatted then
		local n = 0
		for line in self.Text:gmatch("([^\n]*)\n?") do
			n = n + 1
			local tw = wl[n]
			insline(text, { line }, x, y, tw, 0, fh)
			y = y + fh
		end
	else
		local ws = self.WordSpacing
		for spc, word in self.Text:gmatch("(%s*)([^%s]*)") do
			for s in spc:gmatch("%s") do
				if s == "\n" then
					insline(text, line, x, y, tw, so, fh)
					y = y + fh
					line = false
					so = 0
					tw = 0
				end
			end
			if word then
				line = line or { }
				insert(line, word)
				i = i + 1
				tw = tw + wl[i]
				if tw + so > width then
					if #line > 0 then
						remove(line)
						insline(text, line, x, y, tw, so, fh)
						y = y + fh
					end
					line = { word }
					so = 0
					tw = wl[i]
				end
				so = so + ws
			end
		end
		if line and #line > 0 then
			insline(text, line, x, y, tw, so, fh)
			y = y + fh
		end
	end
	return text, y
end

-------------------------------------------------------------------------------
--	layout: overrides
-------------------------------------------------------------------------------

function FloatText:layout(r1, r2, r3, r4, markdamage)

	local s1, s2, s3, s4 = self:getRect()
	local m1, m2, m3, m4 = self:getMargin()
	
	local changed = Frame.layout(self, r1, r2, r3, r4, markdamage)
	
	local width = r3 - r1 + 1 - m1 - m3
	local ch = self.CanvasHeight
	local x0 = r1 + m1
	local y0 = r2 + m2
	local x1 = r3 - m3
	local redraw

	if not ch or (not s1 or s3 - s1 + 1 ~= width) then
		self.Lines, ch = self:layoutText(r1 + m1, r2 + m2, width)
		ch = ch + m4
		self.CanvasHeight = ch
		redraw = true
	end

	local y1 = self.Canvas and r2 + ch - 1 - m4 or r4 - m4

	if redraw or markdamage or
		s1 ~= x0 or s2 ~= y0 or s3 ~= x1 or s4 ~= y1 then
		if self.Canvas then
			self.Canvas:setValue("CanvasHeight", self.CanvasHeight)
			if self.Reposition == "tail" then
				self.Canvas:setValue("CanvasTop", self.CanvasHeight)
			end
		end
		self.DamageRegion = Region.new(x0, y0, x1, y1)
		if not markdamage and not redraw and s1 == x0 and s2 == y0 then
			self.DamageRegion:subRect(s1, s2, s3, s4)
		end
		self.Rect:setRect(x0, y0, x1, y1)
		self:setFlags(FL_LAYOUT + FL_REDRAW)
		changed = true
	end
	
	return changed

end

-------------------------------------------------------------------------------
--	setState: overrides
-------------------------------------------------------------------------------

function FloatText:setState(bg, fg)
	fg = fg or self.Properties["color"] or "list-detail"
	if fg ~= self.FGPen then
		self.FGPen = fg
		self:setFlags(FL_REDRAW)
	end
	Frame.setState(self, bg)
end

-------------------------------------------------------------------------------
--	onSetText(): Handler called when a new {{Text}} is set.
-------------------------------------------------------------------------------

function FloatText:onSetText()
	self:prepareText()
	self.CanvasHeight = false
	self:rethinkLayout(2)
end

-------------------------------------------------------------------------------
--	appendLine(text[, movetail]): Append a line of text; if the
--	optional boolean {{movetail}} is '''true''', the visible area of the
--	element is moved towards the end of the text.
-------------------------------------------------------------------------------

function FloatText:appendLine(text, movetail)
	self.Reposition = movetail and "tail" or false
	if self.Text == "" then
		self:setValue("Text", text)
	else
		self:setValue("Text", self.Text .. "\n" .. text)
	end
end

-------------------------------------------------------------------------------
--	reconfigure: overrides
-------------------------------------------------------------------------------

function FloatText:reconfigure()
	Frame.reconfigure(self)
	self:initFont()
	self:prepareText()
end

return FloatText
