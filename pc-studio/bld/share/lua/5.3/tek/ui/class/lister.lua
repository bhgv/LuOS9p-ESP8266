-------------------------------------------------------------------------------
--
--	tek.ui.class.lister
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
--		Lister ${subclasses(Lister)}
--
--		This class implements a scrollable list or table. Each item in the
--		list is a table consisting of the following elements:
--
--				{ { "column1", "column2", ... }, userdata, selected, ... }
--
--		Description:
--			- {{entry[1]}} is a table containing the text of each column;
--			- {{entry[2]}} is a userdata field which can be used at will;
--			- {{entry[3]}} is a boolean indicating that the line is selected.
--
--		The following fields are reserved for internal use by the Lister;
--		you should never rely on their contents or modify them.
--
--	ATTRIBUTES::
--		- {{AlignColumn [IG]}} (number)
--			A column number to align the right edge of the list to.
--			[Default: unspecified]
--		- {{AlignElement [IG]}} (Object)
--			The object determining the right edge of the list, which
--			is an information needed for the {{AlignColumn}} attribute.
--			By default, the Lister's parent {{Canvas}} is used, but by
--			use of this attribute it is possible to align the Lister to
--			something else.
--		- {{BGAlt [IG]}} (userdata)
--			A colored pen for painting the background of alternating lines
--		- {{ColumnPadding [IG]}} (number)
--			The padding between columns, in pixels. By default, the
--			Lister's {{Font}} is used to determine a reasonable offset.
--		- {{CursorBorderClass [IG]}} (string)
--			Name of the border class used to implement this element's
--			cursor border. Default: "default"
--		- {{CursorLine [ISG]}} (number)
--			The line number of the Lister's cursor; this value may
--			be {{0}}, in which case the cursor is invisible. Changing
--			this value will invoke the Lister:onSetCursor() method.
--		- {{HeaderGroup [IG]}} ([[#tek.ui.class.group : Group]])
--			A group of elements used for the table header. The Lister
--			may take these elements into account for determining the
--			initial column widths.
--		- {{ListObject [IG]}} ([[#tek.class.list : List]])
--			The List object the Lister operates on; if none is specified,
--			the Lister creates an empty one.
--		- {{NumSelectedLines [G]}} (number)
--			The number of lines currently selected.
--		- {{SelectedLines [G]}} (table)
--			This attribute is a (sparse) table containing the numbers of
--			selected lines in the list. Use {{pairs()}} to iterate it.
--			See also Lister:getSelectedLines() for retrieving a
--			numerically indexed list.
--		- {{SelectedLine [ISG]}} (number)
--			The Lister's current selected line; this value reflects only
--			the line that was most recently selected or unselected - for
--			locating all currently selected lines, you will also need
--			the {{SelectedLines}} attribute. Setting this value will invoke
--			the Lister:onSelectLine() method.
--		- {{SelectMode [IG]}} (string)
--			The Lister's selection mode, which can be {{"none"}},
--			{{"single"}}, or {{"multi"}}.
--
--	IMPLEMENTS::
--		- Lister:addItem() - Adds an item to the list
--		- Lister:changeItem() - Overwrite item in the list
--		- Lister:changeSelection() - Changes selection of the list
--		- Lister:clear() - Removes all items from the list
--		- Lister:damageLine() - Marks line for repainting
--		- Lister:getItem() - Returns the item at the specified line
--		- Lister:getN() - Returns the number of entries in the list
--		- Lister:getSelectedLines() - Returns a table of selected entries
--		- Lister:onSelectLine() - Handler invoked for {{SelectedLine}}
--		- Lister:onSetCursor() - Handler invoked for {{CursorLine}}
--		- Lister:repaint() - Relayouts and repaints the list
--		- Lister:remItem() - Removes an item from the list
--		- Lister:setList() - Sets a new list object
--
--	STYLE PROPERTIES::
--		- {{background-color2}}
--
--	OVERRIDES::
--		- Area:askMinMax()
--		- Element:cleanup()
--		- Element:connect()
--		- Area:draw()
--		- Area:hide()
--		- Area:layout()
--		- Class.new()
--		- Frame:onFocus()
--		- Area:passMsg()
--		- Area:setState()
--		- Element:setup()
--		- Area:show()
--
-------------------------------------------------------------------------------

local db = require "tek.lib.debug"
local List = require "tek.class.list"
local ui = require "tek.ui".checkVersion(112)
local Region = ui.loadLibrary("region", 10)
local Text = ui.require("text", 28)

local assert = assert
local floor = math.floor
local ipairs = ipairs
local insert = table.insert
local intersect = Region.intersect
local max = math.max
local min = math.min
local pairs = pairs
local sort = table.sort
local tonumber = tonumber
local tostring = tostring
local type = type
local unpack = unpack or table.unpack

local Lister = Text.module("tek.ui.class.lister", "tek.ui.class.text")
Lister._VERSION = "Lister 32.1"

-------------------------------------------------------------------------------
--	Constants & Class data:
-------------------------------------------------------------------------------

local FL_LAYOUT = ui.FL_LAYOUT
local MSG_KEYDOWN = ui.MSG_KEYDOWN
local MSG_KEYUP = ui.MSG_KEYUP

-------------------------------------------------------------------------------
--	addClassNotifications: overrides
-------------------------------------------------------------------------------

function Lister.addClassNotifications(proto)
	Lister.addNotify(proto, "CursorLine", ui.NOTIFY_ALWAYS,
		{ ui.NOTIFY_SELF, "onSetCursor" })
	Lister.addNotify(proto, "SelectedLine", ui.NOTIFY_ALWAYS,
		{ ui.NOTIFY_SELF, "onSelectLine" })
	return Text.addClassNotifications(proto)
end

Lister.ClassNotifications =
	Lister.addClassNotifications { Notifications = { } }

-------------------------------------------------------------------------------
--	Class style properties:
-------------------------------------------------------------------------------

Lister.Properties = {
	["border-top-width"] = 0,
	["border-right-width"] = 0,
	["border-bottom-width"] = 0,
	["border-left-width"] = 0,
	["border-rim-width"] = 0,
	["border-focus-width"] = 0,
	["margin-top"] = 0,
	["margin-right"] = 0,
	["margin-bottom"] = 0,
	["margin-left"] = 0,
}

-------------------------------------------------------------------------------
--	init: overrides
-------------------------------------------------------------------------------

function Lister.new(class, self)
	self = self or { }
	self.AlignColumn = self.AlignColumn or false
	-- the element that determines the size for AlignColumn:
	self.AlignElement = self.AlignElement or false
	self.BackPens = { }
	-- alternative list background pen:
	self.BorderStyle = "" -- fixed
	self.Canvas = false -- fixed
	self.CanvasHeight = false -- !!
	self.ColumnPadding = self.ColumnPadding or false
	self.ColumnPositions = { 0 }
	self.ColumnWidths = { 0 }
	self.CursorLine = self.CursorLine or 0
	self.CursorObject = false
	self.FHeight = false
	self.FontHandle = false
	self.FWidth = false
	self.HeaderGroup = self.HeaderGroup or false
	self.Mode = "button"
	self.LineHeight = false
	self.LastKey = false
	self.ListObject = self.ListObject or List:new()
	self.NumColumns = 1
	self.NumSelectedLines = 0
	self.RenderData = { }
	self.SelectedLines = false
	self.SelectedLine = self.SelectedLine or 0
	-- selection modes ("none", "single", "multi"):
	self.SelectMode = self.SelectMode or "single"
	self.TrackDamage = true
	self.OldCursorLine = self.CursorLine
	self.OldSelectedLine = self.SelectedLine
	return Text.new(class, self)
end

-------------------------------------------------------------------------------
--	connect: overrides
-------------------------------------------------------------------------------

function Lister:connect(parent)
	self.Canvas = parent
	return Text.connect(self, parent)
end

-------------------------------------------------------------------------------
--	initFont:
-------------------------------------------------------------------------------

function Lister:initFont()
	local props = self.Properties
	local f = self.Application.Display:openFont(props["font"])
	self.FontHandle = f
	self.FWidth, self.FHeight = f:getTextSize("x")
	local _, b2, _, b4 = self.CursorObject:getBorder()
	self.LineHeight = self.FHeight + b2 + b4
	self.ColumnPadding = self.ColumnPadding or self.FWidth
end

-------------------------------------------------------------------------------
--	reconfigure:
-------------------------------------------------------------------------------

function Lister:reconfigure()
	Text.reconfigure(self)
	self:initFont()
	self:prepare(false)
end

-------------------------------------------------------------------------------
--	setup: overrides
-------------------------------------------------------------------------------

function Lister:setup(app, window)
	Text.setup(self, app, window)
	self:initSelectedLines()
	local props = self.Properties
	local b = tonumber(props["cursor-width"]) or 1
	self.CursorObject = ui.createHook("border", "default", self,
		{ Border = { b, b, b, b } })
	self:initFont()
	self:prepare(false)
end

-------------------------------------------------------------------------------
--	cleanup: overrides
-------------------------------------------------------------------------------

function Lister:cleanup()
	self.FontHandle = self.Application.Display:closeFont(self.FontHandle)
	self.CursorObject = ui.destroyHook(self.CursorObject)
-- 	self.SelectedLines = false
	Text.cleanup(self)
	self.Canvas = false
end

-------------------------------------------------------------------------------
--	askMinMax: overrides
-------------------------------------------------------------------------------

function Lister:askMinMax(m1, m2, m3, m4)
	m1 = m1 + self:getAttr("MinWidth")
	m2 = m2 + self.FHeight
	m3 = ui.HUGE
	m4 = ui.HUGE
	return Text.askMinMax(self, m1, m2, m3, m4)
end

-------------------------------------------------------------------------------
--	initSelectedLines:
-------------------------------------------------------------------------------

function Lister:initSelectedLines()
	local s = { }
	local n = 0
	local lo = self.ListObject
	if lo then
		for lnr = 1, lo:getN() do
			local l = lo:getItem(lnr)
			if l[3] then
				n = n + 1
				s[lnr] = lnr
			end
		end
	end
	self.SelectedLines, self.NumSelectedLines = s, n
	return s, n
end

-------------------------------------------------------------------------------
--	getLineOnScreen:
-------------------------------------------------------------------------------

function Lister:getLineOnScreen(lnr)
	local l = self.ListObject and self.ListObject:getItem(lnr)
	if l then
		local c = self.Canvas
		if c then
			local r1, r2, r3, r4 = c:getRect()
			if r1 then
				local v1 = c.CanvasLeft
				local v2 = c.CanvasTop
				local v3 = v1 + r3 - r1
				local v4 = v2 + r4 - r2
				return intersect(v1, v2, v3, v4, 0, l[4], c.CanvasWidth - 1,
					l[5])
			end
		end
	end
end

-------------------------------------------------------------------------------
--	damageLine(lnr): Marks the specified line for repainting.
-------------------------------------------------------------------------------

function Lister:damageLine(lnr)
	local r1, r2, r3, r4 = self:getLineOnScreen(lnr)
	if r1 then
		self:damage(r1, r2, r3, r4)
	end
end

-------------------------------------------------------------------------------
--	shiftSelection: shift selection by given number of steps, starting at the
--	specified line number.
-------------------------------------------------------------------------------

function Lister:shiftSelection(lnr, delta)
	if lnr then
		local s = self.SelectedLines
		if s then
			local t = { }
			for line in pairs(s) do
				if line >= lnr then
					t[line + delta] = line + delta
					s[line] = nil
				else
					t[line] = line
				end
			end
			self.SelectedLines = t
		end
	end
end

-------------------------------------------------------------------------------
--	addItem(item[, line[, quick]]): Adds an item to the list. If
--	{{line}} is unspecified, the entry is added at the end of the list. The
--	boolean {{quick}} indicates that the list should not be relayouted and
--	repainted. (For relayouting and repainting the list after mass addition,
--	see also Lister:repaint().)
-------------------------------------------------------------------------------

function Lister:addItem(entry, lnr, quick)
	local lo = self.ListObject
	if lo then
		if type(entry) ~= "table" then
			entry = { { tostring(entry) } }
		end
		self:shiftSelection(lnr, 1)
		lnr = lo:addItem(entry, lnr)
		if entry[3] then
			self.NumSelectedLines = self.NumSelectedLines + 1
			self.SelectedLines[lnr] = lnr
		end
		if not quick then
			self:prepare(true)
			for i = lnr, lo:getN() do
				self:damageLine(i)
			end
			if self.Canvas then
				self.Canvas:updateUnusedRegion()
			end
		end
	end
end

-------------------------------------------------------------------------------
--	remItem(line[, quick]): Removes the item from the list at the
--	specified line. The boolean {{quick}} indicates that the list should not
--	be relayouted and repainted. (For relayouting and repainting the list
--	after mass removal, see also Lister:repaint().)
-------------------------------------------------------------------------------

function Lister:remItem(lnr, quick)
	local lo = self.ListObject
	if lo then
		local entry = lo:remItem(lnr)
		if entry then
			if entry[3] then
				self.SelectedLines[lnr] = nil
				self.NumSelectedLines = self.NumSelectedLines - 1
			end
			self:shiftSelection(lnr + 1, -1)
			if not quick then
				self:prepare(true)
				for i = lnr, lo:getN() do
					self:damageLine(i)
				end
				self:moveLine()
			end
		end
		return entry
	end
end

-------------------------------------------------------------------------------
--	changeItem(item, line[, quick]): Overwrites the {{item}} at the
--	specified {{line}} in the list. The boolean {{quick}} indicates that
--	the list should not be relayouted and repainted. For relayouting and
--	repainting the list after mass changes, see also Lister:repaint().
-------------------------------------------------------------------------------

function Lister:changeItem(entry, lnr, quick)
	local lo = self.ListObject
	if lo then
		if self:remItem(lnr, quick) then
			if type(entry) ~= "table" then
				entry = { { tostring(entry) } }
			end
			self:addItem(entry, lnr, quick)
			return true
		end
	end
end

-------------------------------------------------------------------------------
--	setList(listobject): Sets a new [[#tek.class.list : List]]
--	object, and relayouts and repaints the list.
-------------------------------------------------------------------------------

function Lister:setList(listobject)
	assert(not listobject or listobject:instanceOf(List))
	self.ListObject = listobject
	self:initSelectedLines()
	self:prepare(true)
end

-------------------------------------------------------------------------------
--	repaint(): Relayouts and repaints the list.
-------------------------------------------------------------------------------

function Lister:repaint()
	self:prepare(true)
end

-------------------------------------------------------------------------------
--	prepare: internal
-------------------------------------------------------------------------------

function Lister:prepare(damage)
	local lo = self.ListObject
	local co = self.CursorObject
	if lo and co then -- and d then

		local b1, b2, b3, b4 = co:getBorder()
		local f = self.FontHandle
		local cw = { }
		self.ColumnWidths = cw
		local cp = self.ColumnPositions
		local nc = 0
		local y = 0
		local hg = self.HeaderGroup
		local lh = self.LineHeight

		-- initialize column widths with head item sizes:
		if hg then
			for i, e in ipairs(hg.Children) do
				cw[i] = max(e:askMinMax(0, 0, 0, 0) - self.ColumnPadding, 0)
			end
		end

		for lnr = 1, lo:getN() do
			local l = lo:getItem(lnr)
			local columns = l[1]
			nc = max(nc, #columns)
			for i, t in ipairs(columns) do
				local w = f:getTextSize(columns[i])
				cw[i] = max(cw[i] or 0, w)
			end
			l[4], l[5] = y, y + lh - 1
			y = y + lh
		end

		-- TODO: it's nonsense to depend on 'damage' here,
		-- the real question is if we have been layouted yet...
		if damage and self.AlignColumn then
			local ae = self.AlignElement or self.Canvas
			local r1, _, r3 = ae:getRect()
			if r1 then
				local pw = r3 - r1 + 1 - b1 - b3
				local cx = 0
				for i = 1, #cw do
					local w = cw[i]
					if i < nc then
						w = w + self.ColumnPadding
					end
					if self.AlignColumn == i and #cw > i then
						local ncx = pw - self.ColumnPadding - cw[i + 1]
						w = ncx - cx
						cw[i] = w
						cx = ncx
					else
						cx = cx + w
					end
				end
			end
		end

		local cx = 0
		local redraw = false
		for i, w in ipairs(cw) do

			-- column positions changed?:
			redraw = redraw or cp[i] ~= cx
			cp[i] = cx

			if i < nc then
				w = w + self.ColumnPadding
			end

			cx = cx + w

			if hg then
				local e = hg.Children[i]
				if e then
-- 					local p = e.Padding
					e.MinWidth = w	-- - p[1] - p[3]
					if i == nc then
						e.MaxWidth = ui.HUGE
					else
						e.MaxWidth = 0
					end
				end
			end
		end

		self.MinWidth = cx
		self.NumColumns = nc
		self.CanvasHeight = y

		local c = self.Canvas

		c:setValue("CanvasWidth", cx + b1 + b3)

		if self:layout(0, 0, c.CanvasWidth - 1, c.CanvasHeight - 1, damage)
			or damage then
			c:rethinkLayout(1)
			self:setFlags(ui.FL_REDRAW)
		end

		if damage then
			c:updateUnusedRegion()
		end

	end
end

-------------------------------------------------------------------------------
--	draw: overrides
-------------------------------------------------------------------------------

function Lister:draw()
	local lo, dr = self.ListObject, self.DamageRegion
	if lo and dr then
		-- repaint intra-area damagerects:
		local props = self.Properties
		local d = self.Window.Drawable
		local t = self.RenderData
		local bgcol = props["background-color"] or "background"
		t[6] = bgcol -- background pen
		t[7] = props["background-color2"] or bgcol -- background pen, alternate
		d:setFont(self.FontHandle)
		local props = self.Properties
		dr:forEach(self.drawPatch, self, t, d, lo, self.LineHeight,
			self.Canvas.CanvasWidth - 1,
			props["color"] or "list-detail", -- foreground pen
			props["background-color:active"] or "list-active", -- foreground pen
			props["color:active"] or "list-active-detail") -- foreground pen
		self.DamageRegion = false
	end
end

function Lister:drawPatch(r1, r2, r3, r4, t, d, lo, lh, x1, fpen, cpen,
	cfpen)
	d:pushClipRect(r1, r2, r3, r4)
	for lnr = floor(r2 / lh) + 1, min(lo:getN(), floor(r4 / lh) + 1) do
		local l = lo:getItem(lnr)
		-- overlap between damage and line:
		if intersect(r1, r2, r3, r4, 0, l[4], x1, l[5]) then
			local b1, b2, b3, b4 = unpack(t, 1, 4)
			local cp = self.ColumnPositions
			local bpen = t[6 + lnr % 2]
			if lnr == self.CursorLine then
				-- with cursor:
				d:fillRect(b1, l[4] + b2, x1 - b3, l[5] - b4,
					l[3] and cpen or bpen)
				for ci = 1, #cp do
					local text = l[1][ci]
					if text then
						local cx = cp[ci]
						d:pushClipRect(b1 + cx, l[4] + b2,
							b1 + cx + self.ColumnWidths[ci], l[5] - b4)
						d:drawText(b1 + cx, l[4] + b2,
							b1 + cx + self.ColumnWidths[ci], l[5] - b4,
							l[1][ci], l[3] and cfpen or fpen)
						d:popClipRect()
					end
				end
				self.CursorObject:draw(d)
			else
				-- without cursor:
				d:fillRect(0, l[4], x1, l[5], l[3] and cpen or bpen)
				for ci = 1, #cp do
					local text = l[1][ci]
					if text then
						local cx = cp[ci]
						if intersect(r1, r2, r3, r4, cx, l[4],
							b1 + cx + self.ColumnWidths[ci] - 1, l[5]) then
							d:pushClipRect(b1 + cx, l[4] + b2,
								b1 + cx + self.ColumnWidths[ci], l[5] - b4)
							-- draw text:
							d:drawText(b1 + cx, l[4] + b2,
								b1 + cx + self.ColumnWidths[ci],
								l[5] - b4, l[1][ci], l[3] and cfpen or fpen)
							d:popClipRect()
						end
					end
				end
			end
		end
	end
	d:popClipRect()
end

-------------------------------------------------------------------------------
--	layout: overrides
-------------------------------------------------------------------------------

function Lister:layout(r1, r2, r3, r4, markdamage)
	local res
	local c = self.Canvas
	local ch = self.CanvasHeight
	local cw = c.CanvasWidth
	local m1, m2, m3, m4 = self:getMargin()
	local x0 = r1 + m1
	local y0 = r2 + m2
	local x1 = r3 - m3
	local y1 = r2 + ch - 1 - m4
	local r = self.Rect
	local r1, r2, r3, r4 = r:get()
	if r1 ~= x0 or r2 ~= y0 or r3 ~= x1 or r4 ~= x1 then
		r:setRect(x0, y0, x1, y1)
		c:setValue("CanvasHeight", ch)
		self:layoutCursor()
		self:setFlags(FL_LAYOUT)
		res = true
	end
	if markdamage then
		c:damageChild(0, 0, cw - 1, ch - 1)
	end
	return res
end

function Lister:layoutCursor()
	local c = self.Canvas
	if c then
		local co = self.CursorObject
		local x1 = c.CanvasWidth - 1
		local t = self.RenderData
		t[1], t[2], t[3], t[4] = co:getBorder()
		t[5] = x1
		local lh = self.LineHeight
		local y0 = (self.CursorLine - 1) * lh
		local y1 = y0 + lh - 1
		co:layout(t[1], y0 + t[2], x1 - t[3], y1 - t[4])
	end
end

-------------------------------------------------------------------------------
--	setState: overrides
-------------------------------------------------------------------------------

function Lister:setState(bg, fg)
	bg = bg or self.Properties["background-color"]
	Text.setState(self, bg, fg)
end

-------------------------------------------------------------------------------
--	onSetCursor(): This method is invoked when the
--	{{CursorLine}} attribute has changed.
-------------------------------------------------------------------------------

function Lister:onSetCursor()
	local lnr = self.CursorLine
	local lo = self.ListObject
	if lo then
		local oldlnr = self.OldCursorLine
		lnr = lnr and min(max(0, lnr), lo:getN())
		self.CursorLine = oldlnr
		self.OldCursorLine = lnr
		self:moveLine(lnr, true)
		self:layoutCursor()
	end
end

-------------------------------------------------------------------------------
--	onDblClick(): overrides
-------------------------------------------------------------------------------

function Lister:onDblClick()
	Text.onDblClick(self)
	if self.DblClick == true then
		if not self.SelectedLines[self.CursorLine] then
			self:setValue("SelectedLine", self.CursorLine, true)
		end
	end
end

-------------------------------------------------------------------------------
--	onSelectLine(): This method is invoked when the {{SelectedLine}}
--	attribute is set.
-------------------------------------------------------------------------------

function Lister:onSelectLine()
	local lnr = self.SelectedLine
	local lo = self.ListObject
	if lo then
		local oldlnr = self.OldSelectedLine
		self.OldSelectedLine = lnr
		local m = self.SelectMode
		if m == "single" and oldlnr ~= lnr then
			local oe = lo:getItem(oldlnr)
			if oe then
				oe[3] = false
				if self.SelectedLines[oldlnr] then
					self.SelectedLines[oldlnr] = nil
					self.NumSelectedLines = self.NumSelectedLines - 1
				end
				self:damageLine(oldlnr)
			end
		end
		if m == "single" or m == "multi" then
			local ne = lo:getItem(lnr)
			if ne then
				ne[3] = not ne[3]
				if ne[3] then
					assert(not self.SelectedLines[lnr])
					self.SelectedLines[lnr] = lnr
					self.NumSelectedLines = self.NumSelectedLines + 1
				else
					assert(self.SelectedLines[lnr])
					self.SelectedLines[lnr] = nil
					self.NumSelectedLines = self.NumSelectedLines - 1
				end
				self:damageLine(lnr)
			end
		end
	end
end

-------------------------------------------------------------------------------
--	moveLine(lnr[, follow]) - Moves the list cursor to the
--	specified line; if the optional boolean {{follow}} is '''true''',
--	the visible part of the list follows the cursor.
-------------------------------------------------------------------------------

function Lister:moveLine(lnr, follow)
	local lo = self.ListObject
	if not lo then
		return
	end

	if lnr then
		lnr = min(max(lnr, 0), lo:getN())
	end

	local ca = self.Canvas
	if ca then
		local x1 = ca.CanvasWidth - 1
		local cl = self.CursorLine
		if cl >= 0 then
			local ol = lo:getItem(cl)
			if ol then
				self:damage(0, ol[4], x1, ol[5])
			end
		end
		local l = lnr and lo:getItem(lnr)
		if l then
			local y0, y1 = l[4], l[5]
			if y0 and y1 then
				self:damage(0, y0, x1, y1)
				if y1 < ca.CanvasTop then
					if follow then
						ca:setValue("CanvasTop", y0)
					end
				else
					local _, r2, _, r4 = ca:getRect()
					if r2 then
						local vh = r4 - r2 + 1 - (y1 - y0 + 1)
						if y0 > ca.CanvasTop + vh then
							if follow then
								ca:setValue("CanvasTop", y0 - vh)
							end
						end
					end
				end
			end
		end
	end

	if lnr then
		self:setValue("CursorLine", lnr)
	end
end

function Lister:findLine(y)
	local lo = self.ListObject
	if lo then
		for lnr = 1, lo:getN() do
			local l = lo:getItem(lnr)
			if y >= l[4] and y <= l[5] then
				return lnr
			end
		end
	end
end

-------------------------------------------------------------------------------
--	changeSelection(mode): Changes the selection of the entire
--	list; modes supported are:
--		- "none": marks all lines as unselected
-------------------------------------------------------------------------------

function Lister:changeSelection(mode)
	local lo = self.ListObject
	if lo then
		if mode == "none" then
			for lnr = 1, lo:getN() do
				local l = lo:getItem(lnr)
				if l[3] then
					self:setValue("SelectedLine", lnr, true)
				end
			end
		end
	end
end

-------------------------------------------------------------------------------
--	passMsg: overrides
-------------------------------------------------------------------------------

function Lister:passMsg(msg)
	if msg[2] == ui.MSG_MOUSEBUTTON then
		if msg[3] == 1 then -- leftdown:
			local qual = msg[6]
			if self.Window.HoverElement == self and not self.Disabled then
				local mx, my = self:getMsgFields(msg, "mousexy")
				local lnr = self:findLine(my)
				if lnr then
					if self.SelectMode == "multi" then
						if qual == 0 or qual >= 16 then
							local active = self.SelectedLines[lnr]
							self:changeSelection("none")
							if not active then
								self:setValue("SelectedLine", lnr, true)
							end
						elseif qual == 1 or qual == 2 then
							local s, e = self.CursorLine, lnr
							if e < s then
								s, e = e, s
							end
							for i = s, e do
								if not self:getItem(i)[3] then
									self:setValue("SelectedLine", i, true)
								end
							end
						else
							self:setValue("SelectedLine", lnr, true)
						end
					elseif self.SelectMode == "single" then
						self:setValue("SelectedLine", lnr, true)
					end
					self:moveLine(lnr, true)
				end
			end
		end
	end
	return Text.passMsg(self, msg)
end

-------------------------------------------------------------------------------
--	handleInput:
-------------------------------------------------------------------------------

local function getheight(self)
	local _, r2, _, r4 = self.Canvas:getRect()
	return r4 - r2 + 1
end

function Lister:handleInput(msg)
	local win = self.Window
	if win then
		if self == win.FocusElement then
			local key = msg[3]
			local qual = msg[6]
			local lo = self.ListObject
			local numl = lo:getN()
			local lnr = self.CursorLine
			if msg[2] == MSG_KEYDOWN then
				if key == 0xf013 then -- down
					self:moveLine(min(lnr + 1, numl), true)
					return false
				elseif key == 0xf012 then -- up
					self:moveLine(max(lnr - 1, 1), true)
					return false
				elseif key == 0xf024 then -- pgdown
					if qual == 4 or qual == 8 then
						self:moveLine(numl, true)
					else
						local entry = self:getItem(lnr)
						local y0 = entry and entry[4] or self.CanvasTop
						local l1 = self:findLine(y0 + getheight(self)) or numl
						self:moveLine(l1, true)
					end
				elseif key == 0xf023 then -- pgup
					if qual == 4 or qual == 8 then
						self:moveLine(1, true)
					else
						local entry = self:getItem(lnr)
						local y0 = entry and entry[4] or self.CanvasTop
						local l1 = self:findLine(y0 - getheight(self)) or 1
						self:moveLine(l1, true)
					end
				elseif key == 13 and self.LastKey ~= 13 then -- return
					win:clickElement(self)
					self:setValue("SelectedLine", self.CursorLine, true)
				elseif key == 32 and self.LastKey ~= 32 then -- space
					self:setValue("SelectedLine", self.CursorLine, true)
				end
				self.LastKey = key
			elseif msg[2] == MSG_KEYUP then
				self.LastKey = false
			end
		end
	end
	return msg
end

-------------------------------------------------------------------------------
--	onFocus: overrides
-------------------------------------------------------------------------------

function Lister:onFocus()
	Text.onFocus(self)
	local focused = self.Focus
	if focused then
		self.Window:addInputHandler(MSG_KEYDOWN + MSG_KEYUP, self, 
			self.handleInput)
	else
		self.Window:remInputHandler(MSG_KEYDOWN + MSG_KEYUP, self, 
			self.handleInput)
	end
end

-------------------------------------------------------------------------------
--	getN(): Returns the number of lines in the list.
-------------------------------------------------------------------------------

function Lister:getN()
	local lo = self.ListObject
	return lo and lo:getN() or 0
end

-------------------------------------------------------------------------------
--	getItem(line): Returns the item at the specified line.
-------------------------------------------------------------------------------

function Lister:getItem(lnr)
	local lo = self.ListObject
	return lo and lo:getItem(lnr)
end

-------------------------------------------------------------------------------
--	getSelectedLines([mode]): Returns a table of all
--	selected entries, sorted by (by default) their line number. Currently
--	defined modes are "ascending" or "descending". Default: "ascending"
-------------------------------------------------------------------------------

function Lister:getSelectedLines(mode)
	local t = { }
	if self.SelectedLines then
		for i in pairs(self.SelectedLines) do
			insert(t, i)
		end
		if mode == "descending" then
			sort(t, function(a, b) return a > b end)
		else
			sort(t)
		end
	end
	return t
end

-------------------------------------------------------------------------------
--	clear(): Remove all items from the list.
-------------------------------------------------------------------------------

function Lister:clear()
	local lo = self.ListObject
	if lo then
		lo:clear()
	end
end

return Lister
