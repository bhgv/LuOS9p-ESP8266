-------------------------------------------------------------------------------
--
--	tek.ui.class.text
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
--		Text ${subclasses(Text)}
--
--		This class implements widgets with text.
--
--	ATTRIBUTES::
--		- {{KeepMinHeight [I]}} (boolean)
--			After the initial size calculation, keep the minimal height of
--			the element and do not rethink the layout in response to a
--			possible new minimal height (e.g. resulting from a newly set
--			text). The boolean (default '''false''') will be translated
--			to the flag {{FL_KEEPMINHEIGHT}}, and is meaningless after
--			initialization.
--		- {{KeepMinWidth [I]}} (boolean)
--			Translates to the flag {{FL_KEEPMINWIDTH}}. See also
--			{{KeepMinHeight}}, above.
--		- {{Text [ISG]}} (string)
--			The text that will be displayed on the element; it may span
--			multiple lines (see also Text:makeTextRecords()). Setting this
--			attribute invokes the Text:onSetText() method.
--
--	STYLE PROPERTIES::
--		''color-disabled'' || Secondary color for text in disabled state
--		''font''           || Font specification, see below
--		''text-align''     || {{"left"}}, {{"center"}}, {{"right"}}
--		''vertical-align'' || {{"top"}}, {{"center"}}, {{"bottom"}}
--
--	FONT SPECIFICATION::
--		A font is specified in the form
--				"[fontname1,fontname2,...][:][size]"
--		Font names, if specified, will be probed in the order of their
--		occurrence; the first font that can be opened will  be used. For
--		the font name, the following predefined names are supported:
--			* ''ui-main'' (or an empty string) - The default font,
--			e.g. for buttons
--			* ''ui-fixed'' - The default fixed font
--			* ''ui-menu'' - The default menu font
--			* ''ui-small'' - The default small font, e.g. for group
--			captions
--			* ''ui-x-small'' - The default 'extra small' font
--			* ''ui-xx-small'' - The default 'extra extra small' font
--			* ''ui-large'' - The default 'large' font
--			* ''ui-x-large'' - The default 'extra large' font
--			* ''ui-xx-large'' - The default 'extra extra large' font
--		If no font name is specified, the main font will be used.
--		The size specification (in pixels) is optional; if absent,
--		the respective font's default size will be used.
--
--		Additional hints can be passed alongside with the fontname by appending
--		a slash and option letters, which must be in alphabetical order.
--		Currently, the letter {{b}} will be used as a hint for boldface, and
--		{{i}} for italic. For example, the string {{"ui-main/bi"}} would
--		request the default main font in bold italic.
--
--	IMPLEMENTS::
--		- Text:getTextSize() - Gets the total extents of all text records
--		- Text:makeTextRecords() - Breaks text into multiple line records
--		- Text:onSetText() - Handler for changes of the {{Text}} attribute
--
--	OVERRIDES::
--		- Object.addClassNotifications()
--		- Area:askMinMax()
--		- Element:cleanup()
--		- Area:draw()
--		- Class.new()
--		- Element:onSetStyle()
--		- Element:setup()
--
-------------------------------------------------------------------------------

local db = require "tek.lib.debug"
local ui = require "tek.ui".checkVersion(112)
local Widget = ui.require("widget", 25)

local bor = ui.bor
local floor = math.floor
local insert = table.insert
local max = math.max
local min = math.min
local remove = table.remove
local type = type

local Text = Widget.module("tek.ui.class.text", "tek.ui.class.widget")
Text._VERSION = "Text 29.1"

-------------------------------------------------------------------------------
--	constants & class data:
-------------------------------------------------------------------------------

local FL_CHANGED = ui.FL_CHANGED
local FL_SETUP = ui.FL_SETUP
local FL_REDRAW = ui.FL_REDRAW
local FL_KEEPMINWIDTH = ui.FL_KEEPMINWIDTH
local FL_KEEPMINHEIGHT = ui.FL_KEEPMINHEIGHT

-------------------------------------------------------------------------------
--	addClassNotifications: overrides
-------------------------------------------------------------------------------

function Text.addClassNotifications(proto)
	Text.addNotify(proto, "Text", ui.NOTIFY_ALWAYS, { ui.NOTIFY_SELF, "onSetText" })
	return Widget.addClassNotifications(proto)
end

Text.ClassNotifications = Text.addClassNotifications { Notifications = { } }

-------------------------------------------------------------------------------
--	init: overrides
-------------------------------------------------------------------------------

function Text.new(class, self)
	self = self or { }
	local flags = 0
	if self.KeepMinWidth then
		flags = bor(flags, FL_KEEPMINWIDTH)
	end
	if self.KeepMinHeight then
		flags = bor(flags, FL_KEEPMINHEIGHT)
	end
	self.Flags = bor(self.Flags or 0, flags)
	self.InitialWidth = false
	self.InitialHeight = false
	self.Mode = self.Mode or "inert"
	self.Text = self.Text or ""
	self.TextRecords = self.TextRecords or false
	return Widget.new(class, self)
end

-------------------------------------------------------------------------------
--	setup: overrides
-------------------------------------------------------------------------------

function Text:setup(app, window)
	if self.KeyCode == true then
		local sc = ui.ShortcutMark
		local keycode = self.Text:match("^[^" .. sc .. "]*" .. sc .. "(.)")
		self.KeyCode = keycode and "IgnoreAltShift+" .. keycode or true
	end
	Widget.setup(self, app, window)
	if not self.TextRecords then
		self:makeTextRecords(self.Text)
	end
end

-------------------------------------------------------------------------------
--	cleanup: overrides
-------------------------------------------------------------------------------

function Text:cleanup()
	self.TextRecords = false
	Widget.cleanup(self)
end

-------------------------------------------------------------------------------
--	width, height = getTextSize([textrecord]): This function calculates
--	the total width and height required by the object's text records.
--	Optionally, it can be passed a table of text records which are to be
--	evaluated.
-------------------------------------------------------------------------------

function Text:getTextSize(tr)
	tr = tr or self.TextRecords
	local totw, toth = 0, 0
	local x, y
	for i = 1, #tr do
		local t = tr[i]
		local tw, th = t[9], t[10]
		totw = max(totw, tw + t[5] + t[7])
		toth = max(toth, th + t[6] + t[8])
		if t[15] then
			x = min(x or 1000000, t[15])
		end
		if t[16] then
			y = min(y or 1000000, t[16])
		end
	end
	return totw, toth, x, y
end

-------------------------------------------------------------------------------
--	askMinMax: overrides
-------------------------------------------------------------------------------

function Text:askMinMax(m1, m2, m3, m4)
	local w, h = self:getTextSize()
	local minw, minh = w, h
	if self:checkFlags(FL_KEEPMINWIDTH) then
		if self.InitialWidth then
			minw = self.InitialWidth
		else
			self.InitialWidth = minw
		end
	end
	if self:checkFlags(FL_KEEPMINHEIGHT) then
		if self.InitialHeight then
			minh = self.InitialHeight
		else
			self.InitialHeight = minh
		end
	end
	m1 = m1 + minw
	m2 = m2 + minh
	m3 = m3 + w
	m4 = m4 + h
	return Widget.askMinMax(self, m1, m2, m3, m4)
end

-------------------------------------------------------------------------------
--	layout: overrides
-------------------------------------------------------------------------------

local function aligntext(align, opkey, x0, w1, w0)
	if not align or align == "center" then
		return x0 + floor((w1 - w0) / 2)
	elseif align == opkey then
		return x0 + w1 - w0
	end
	return x0
end

function Text:layoutText()
	local r1, r2, r3, r4 = self:getRect()
	if r1 then
		local props = self.Properties
		local p1, p2, p3, p4 = self:getPadding()
		local w0, h0 = self:getTextSize()
		local w = r3 - r1 + 1 - p3 - p1
		local h = r4 - r2 + 1 - p4 - p2
		local x0 = aligntext(props["text-align"], "right", r1 + p1, w, w0)
		local y0 = aligntext(props["vertical-align"], "bottom", r2 + p2, h, h0)
		local tr = self.TextRecords
		for i = 1, #tr do
			local t = tr[i]
			local x = x0 + t[5]
			local y = y0 + t[6]
			local w = w0 - t[7] - t[5]
			local h = h0 - t[8] - t[6]
			local tw, th = t[9], t[10]
			t[15] = aligntext(t[3], "right", x, w, tw)
			t[16] = aligntext(t[4], "bottom", y, h, th)
		end
	end
end

function Text:layout(x0, y0, x1, y1, markdamage)
	local res = Widget.layout(self, x0, y0, x1, y1, markdamage)
	if self:checkClearFlags(FL_CHANGED) or res then
		self:layoutText()
	end
	return res
end

-------------------------------------------------------------------------------
--	draw: overrides
-------------------------------------------------------------------------------

function Text:draw()
	if Widget.draw(self) then
		local r1, r2, r3, r4 = self:getRect()
		local d = self.Window.Drawable
		local p1, p2, p3, p4 = self:getPadding()
		d:pushClipRect(r1 + p1, r2 + p2, r3 - p3, r4 - p4)
		local fp = self.FGPen
		local tr = self.TextRecords
		for i = 1, #tr do
			local t = tr[i]
			local x, y = t[15], t[16]
			if x then
				d:setFont(t[2])
				if self.Disabled then
					local fp2 = self.Properties["color-shadow:disabled"]
						or "disabled-detail-shine"
					d:drawText(x + 2, y + 2, x + t[9] + 1, y + t[10] + 1, t[1],
						fp2)
					if t[11] then
						-- draw underline:
						d:fillRect(x + t[11] + 2, y + t[12] + 2,
							x + t[11] + t[13] + 1, y + t[12] + t[14] + 1, fp2)
					end
				end
				d:drawText(x + 1, y + 1, x + t[9], y + t[10], t[1], fp)
				if t[11] then
					-- draw underline:
					d:fillRect(x + t[11] + 1, y + t[12] + 1,
						x + t[11] + t[13], y + t[12] + t[14], fp)
				end
			else
				db.warn("text not layouted")
			end
		end
		d:popClipRect()
		return true
	end
end

-------------------------------------------------------------------------------
--	newTextRecord: Note that this function might get called before the element
--	has a Drawable, therefore we must open the font from Application.Display
-------------------------------------------------------------------------------

function Text:newTextRecord(line, font, halign, valign, m1, m2, m3, m4)
	local keycode, _
	local r = { line, font, halign or "center", valign or "center",
		m1 or 0, m2 or 0, m3 or 0, m4 or 0 }
	if self.KeyCode then
		local sc = ui.ShortcutMark
		local a, b = line:match("([^" .. sc .. "]*)" .. sc ..
			"?([^" .. sc .. "]*)")
		if b ~= "" then
			keycode = b:sub(1, 1)
			-- insert underline rectangle:
			r[11] = font:getTextSize(a)
			_, r[12], r[14] = self.Application.Display:getFontAttrs(font)
			r[13] = font:getTextSize(keycode)
			r[1] = a .. b
		end
	end
	local w, h = font:getTextSize(r[1])
	r[9], r[10] = w + 2, h + 2 -- for disabled state
	return r, keycode
end

-------------------------------------------------------------------------------
--	addTextRecord: internal
-------------------------------------------------------------------------------

function Text:addTextRecord(...)
	return self:setTextRecord(#self.TextRecords + 1, ...)
end

-------------------------------------------------------------------------------
--	setTextRecord: internal
-------------------------------------------------------------------------------

function Text:setTextRecord(pos, ...)
	local record, keycode = self:newTextRecord(...)
	self.TextRecords[pos] = record
	self:setFlags(FL_CHANGED)
	return record, keycode
end

-------------------------------------------------------------------------------
--	makeTextRecords(text): This function parses the given {{text}}, breaks it
--	along the encountered newline characters into single-line records, and
--	places the resulting table in the element's {{TextRecords}} field.
--	Each record has the form
--			{ [1]=text, [2]=font, [3]=align-horizontal, [4]=align-vertical,
--			  [5]=margin-left, [6]=margin-right, [7]=margin-top,
--			  [8]=margin-bottom, [9]=font-height, [10]=text-width }
--	More undocumented fields may follow at higher indices. {{font}} is taken
--	from opening the font specified in the object's {{Font}} attribute,
--	which also determines {{font-height}} and is used for calculating the
--	{{text-width}} (in pixels). The alignment parameters are taken from the
--	object's {{TextHAlign}} and {{TextVAlign}} attributes, respectively.
-------------------------------------------------------------------------------

function Text:makeTextRecords(text)
	if self:checkFlags(FL_SETUP) then
		local props = self.Properties
		text = text or ""
		local tr = { }
		self.TextRecords = tr
		local y, nl = 0, 0
		local font = self.Application.Display:openFont(props["font"])
		for line in (text .. "\n"):gmatch("([^\n]*)\n") do
			local r = self:addTextRecord(line, font, props["text-align"],
				props["vertical-align"], 0, y, 0, 0)
			y = y + r[10]
			nl = nl + 1
		end
		y = 0
		for i = nl, 1, -1 do
			tr[i][8] = y
			y = y + tr[i][10]
		end
	end
end

-------------------------------------------------------------------------------
--	onSetText(): This handler is invoked when the element's {{Text}}
--	attribute has changed.
-------------------------------------------------------------------------------

function Text:onSetText()
	self:makeTextRecords(self.Text)
	self:setFlags(FL_REDRAW)
	local resizeable = not self:checkFlags(FL_KEEPMINWIDTH)
	self:rethinkLayout(resizeable and 1 or 0, resizeable)
end

-------------------------------------------------------------------------------
--	onSetStyle: overrides
-------------------------------------------------------------------------------

function Text:onSetStyle()
	Widget.onSetStyle(self)
	self:makeTextRecords(self.Text)
end

-------------------------------------------------------------------------------
--	reconfigure:
-------------------------------------------------------------------------------

function Text:reconfigure()
	Widget.reconfigure(self)
	self.InitialWidth = false
	self.InitialHeight = false
	self:makeTextRecords(self.Text)
end

return Text
