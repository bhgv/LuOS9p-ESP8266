-------------------------------------------------------------------------------
--
--	tek.ui.class.textedit
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
--		[[#tek.ui.class.sizeable : Sizeable]] /
--		TextEdit ${subclasses(TextEdit)}
--
--		Text editor class
--
--	ATTRIBUTES::
--
--	STYLE PROPERTIES::
--
--	IMPLEMENTS::
--		- TextEdit:addChar()
--		- TextEdit:getText()
--		- TextEdit:remChar()
--		- TextEdit:saveText()
--		- TextEdit:setEditing()
--
--	OVERRIDES::
--		- Element:cleanup()
--		- Area:draw()
--		- Area:layout()
--		- Class.new()
--		- Element:onSetStyle()
--		- Area:setState()
--		- Element:setup()
--		- Area:show()
--
-------------------------------------------------------------------------------

local db = require "tek.lib.debug"
local String = require "tek.lib.string"
local ui = require "tek.ui".checkVersion(112)
local Region = ui.loadLibrary("region", 9)
local Sizeable = ui.require("sizeable", 10)

local assert = assert
local concat = table.concat
local floor = math.floor
local insert = table.insert
local intersect = Region.intersect
local max = math.max
local min = math.min
local open = io.open
local remove = table.remove
local tonumber = tonumber
local tostring = tostring
local type = type
local unpack = unpack or table.unpack

local TextEdit = Sizeable.module("tek.ui.class.textedit", "tek.ui.class.sizeable")
TextEdit._VERSION = "TextEdit 21.1"

local LNR_HUGE = 1000000000
local FAKECANVASWIDTH = 1000000000 --30000
local FL_SETUP = ui.FL_SETUP
local FL_READY = ui.FL_LAYOUT + ui.FL_SHOW + FL_SETUP
local FL_TRACKDAMAGE = ui.FL_TRACKDAMAGE

local PENIDX_MARK = 64

-------------------------------------------------------------------------------
--	Constants & Class data:
-------------------------------------------------------------------------------

local NOTIFY_CURSORX = { ui.NOTIFY_SELF, "onSetCursorX", ui.NOTIFY_VALUE }
local NOTIFY_CURSORY = { ui.NOTIFY_SELF, "onSetCursorY", ui.NOTIFY_VALUE }
local NOTIFY_FILENAME = { ui.NOTIFY_SELF, "onSetFileName", ui.NOTIFY_VALUE }
local NOTIFY_CHANGED = { ui.NOTIFY_SELF, "onSetChanged", ui.NOTIFY_VALUE }

-------------------------------------------------------------------------------
--	Class style properties:
-------------------------------------------------------------------------------

TextEdit.Properties = {
	["border-top-width"] = 0,
	["border-right-width"] = 0,
	["border-bottom-width"] = 0,
	["border-left-width"] = 0,
}

-------------------------------------------------------------------------------
--	init: overrides
-------------------------------------------------------------------------------

function TextEdit.new(class, self)
	self = self or { }
	self.AutoIndent = self.AutoIndent or false
	self.AutoPosition = self.AutoPosition or false
	self.AutoWrap = self.AutoWrap or false
	self.BlinkCursor = self.BlinkCursor or false
	self.BlinkState = false
	self.BlinkTick = false
	self.BlinkTickInit = self.BlinkTickInit or 30
	self.Bookmarks = { }
	self.Changed = false
	self.CursorMode = "active" -- "hidden", "still"
	self.Cursor = false
	-- cursor styles: "line", "block", "bar+line", "bar"
	self.CursorStyle = self.CursorStyle or "line"
	self.CursorThickness = self.CursorThickness or 1 -- in line mode, minus 1
	self.CursorX = self.CursorX or 1
	self.CursorY = self.CursorY or 1
	self.Data = self.Data or { "" }	-- transformed during init
	self.Editing = false
	self.FileName = self.FileName or ""
	self.FixedFont = self.FixedFont or false
	self.FixedFWidth = false 
	self.FollowCursor = false
	self.FontHandle = false
	self.FontName = self.FontName or false
	self.FWidth = false
	self.HardScroll = self.HardScroll or false
	self.LineHeight = false
	self.LineOffset = 0
	self.LineSpacing = self.LineSpacing or 0
	self.LockCursorX = false -- last line's remembered cursor position
	self.Mark = false
	self.MarkMode = self.MarkMode or "shift" -- "block", "shift", "none"
	self.MaxLength = self.MaxLength or false
	self.BGPens = self.BGPens or { }
	self.FGPens = self.FGPens or { }
	self.Mode = "touch"
	self.MultiLine = self.MultiLine or false
	self.ReadOnly = self.ReadOnly == nil and true or self.ReadOnly
	self.RealCanvasWidth = 0 -- real width needed by current text
	self.DragScroll = self.DragScroll or false
	self.Size = self.Size or false
	self.PasswordChar = self.PasswordChar or false
	self.SuspendUpdateNest = 0
	self.UpdateSuspended = false
	self.TabSize = self.TabSize or 4
	self.TextWidth = false
	if self.UseFakeCanvasWidth == nil then
		self.UseFakeCanvasWidth = not self.AutoWrap -- unlimited line width
	end
	self.VisualCursorX = 1
	self.VisibleMargin = self.VisibleMargin or { 0, 0, 0, 0 }
	-- indicates that Y positions and heights are cached and valid:
-- 	self.YValid = true 
	return Sizeable.new(class, self)
end

-------------------------------------------------------------------------------
--	bookmarks
-------------------------------------------------------------------------------

function TextEdit:clearBookmarks()
	local b = self.Bookmarks
	while #b > 0 do
		self:toggleBookmark(remove(b, 1), false)
	end
end

function TextEdit:checkBookmark(cy)
	cy = cy or self.CursorY
	local idx, lnr = self:findBookmark(cy, -1)
	return lnr == cy, lnr == cy and idx
-- 	return self.Bookmarks[tostring(cy)]
end

function TextEdit:findBookmark(cy, delta)
	delta = delta or 0
	assert(cy)
	local b = self.Bookmarks
	local idx
	for i = 1, #b + 1 do
		local l0 = i == 1 and 0 or tonumber(b[i - 1])
		local l1 = i == (#b + 1) and LNR_HUGE or tonumber(b[i])
		if cy >= l0 and cy < l1 then
			idx = i + delta
			break
		end
	end
	return idx, b[idx] and tonumber(b[idx])
end

function TextEdit:prevBookmark(cy)
	local idx, lnr = self:findBookmark((cy or self.CursorY) - 1, -1)
	if lnr then
		self:setCursor(-1, 1, lnr, 1)
	end
end

function TextEdit:nextBookmark(cy)
	local idx, lnr = self:findBookmark(cy or self.CursorY)
	if lnr then
		self:setCursor(-1, 1, lnr, 1)
	end
end

function TextEdit:addBookmark(cy)
	self:toggleBookmark(cy, true)
end

function TextEdit:removeBookmark(cy)
	self:toggleBookmark(cy, false)
end

-------------------------------------------------------------------------------
--	toggleBookmark(cy[, onoff])
-------------------------------------------------------------------------------

function TextEdit:toggleBookmark(cy, onoff)
	cy = cy or self.CursorY
	local idx, lnr = self:findBookmark(cy, -1)
	local b = self.Bookmarks
	if lnr == cy then
		if onoff ~= true then
			remove(b, idx)
		end
	else
		if onoff ~= false then
			insert(b, idx + 1, cy)
		end
	end
	self:damageLine(cy)
end

-------------------------------------------------------------------------------
--	marked block and clipboard
-------------------------------------------------------------------------------

function TextEdit:checkInMark(cx, cy)
	local m = self.Mark
	if not m then
		return false
	end
	local res = false
	local mx0, my0, mx1, my1 = m[1], m[2], self.CursorX, self.CursorY
	mx0, my0, mx1, my1 = self:getMarkTopBottom(mx0, my0, mx1, my1)
	local in_startline = cy == my0
	local in_endline = cy == my1
	local markstart_diff = in_startline and cx - mx0
	local markend_diff = in_endline and cx - mx1
	if my0 == my1 then
		if cy == my0 and cx >= mx0 and cx < mx1 then
			res = true
		end
	elseif cy >= my0 and cy <= my1 then
		if (cy == my0 and cx >= mx0) or (cy == my1 and cx < mx1) or 
			(cy > my0 and cy < my1) then
			res = true
		end
	end
	return res, in_startline, in_endline, markstart_diff, markend_diff
end

function TextEdit:doMark(ncx, ncy)
	local m = self.Mark
	if m then
		local cy = self.CursorY
		local y0, y1 = min(m[2], cy), max(m[2], cy)
-- 		self:suspendWindowUpdate()
		for y = y0, y1 do
			self:damageLine(y)
		end
		self.Mark = false
-- 		self:updateWindow()
-- 		self:releaseWindowUpdate()
	else
		self.Mark = { ncx or self.CursorX, ncy or self.CursorY }
		self:damageLine(ncy or self.CursorY)
		return true
	end
end

function TextEdit:getMarkTopBottom(mx0, my0, mx1, my1)
	assert(mx0 and my0 and mx1 and my1)
	if my0 > my1 or (my0 == my1 and mx0 > mx1) then
		my0, my1 = my1, my0
		mx0, mx1 = mx1, mx0
	end
	return mx0, my0, mx1, my1
end

function TextEdit:getMark(cx, cy)
	local m = self.Mark
	if m then
		return self:getMarkTopBottom(m[1], m[2], 
			cx or self.CursorX, cy or self.CursorY)
	end
end

function TextEdit:endMark(cx, cy)
	local mx0, my0, mx1, my1 = self:getMark(cx, cy)
	if mx0 then
		self:doMark()
	end
	return mx0, my0, mx1, my1
end

function TextEdit:eraseBlock(mx0, my0, mx1, my1)
	self:suspendWindowUpdate()
	local lh = self.LineHeight
	local line = self:getLineText(my0)
	if my0 == my1 then
		line:erase(mx0, mx1 - 1)
		self:changeLine(my0)
		self:damageLine(my0)
		self:setCursor(0)
	else
		local numremove = my1 - my0
		local firstremove
		local join
		if mx0 > 1 then
			line:erase(mx0)
			join = true
			firstremove = my0 + 1
			numremove = numremove - 1
		else
			firstremove = my0
		end
		local line = self:getLineText(my1)
		if line then
			if mx1 > 1 then
				line:erase(1, mx1 - 1)
			end
		else
			db.error("*** no line %s", my1)
		end
		for y = numremove - 1, 0, -1 do
			self:removeLine(firstremove + y, true)
		end
		if join then
			local thisline = self:getLineText(my0)
			local nextline = self:getLineText(my0 + 1)
			if thisline and nextline then
				thisline:insert(nextline)
			elseif thisline then
				db.error("*** no nextline %s", my0 + 1)
			elseif nextline then
				db.error("*** no thisline %s", my0)
			end
			self:changeLine(my0)
			self:removeLine(my0 + 1, true)
			self:resize(0, -lh, 0, lh * my0)
			self:damageLine(my0 + 1)
		end
		if numremove > 0 then
			self:resize(0, numremove * -lh, 0, firstremove * lh)
		end
		self:damageLine(my0)
		self:setCursor(1, mx0, my0, 1)
	end
	self:updateCanvasSize()
	self:updateWindow()
	self:releaseWindowUpdate()
	self:setValue("Changed", true)
end

function TextEdit:toClip(mx0, my0, mx1, my1, do_erase)
	local clip = self.Application:obtainClipboard("empty")
	mx0, my0, mx1, my1 = self:getMarkTopBottom(mx0, my0, mx1, my1)
	for y = my0, my1 do
		local line = self:newString(self:getLineText(y)) -- need a copy!
		if y == my1 then
			line = self:newSubString(line, 1, mx1 - 1)
		end
		if y == my0 then
			line = self:newSubString(line, mx0)
		end
		insert(clip, tostring(line))
	end
	if do_erase then
		self:eraseBlock(mx0, my0, mx1, my1)
	end
	self.Application:releaseSetClipboard(clip)
	return clip
end

function TextEdit:copyMark()
	local mx0, my0, mx1, my1 = self:getMark()
	if mx0 then
		return self:toClip(mx0, my0, mx1, my1)
	end
end

function TextEdit:cutMark(mode)
	local mx0, my0, mx1, my1 = self:endMark()
	if mx0 then
		local clip
		if mode == "erase" then
			self:eraseBlock(mx0, my0, mx1, my1)
		else
			clip = self:toClip(mx0, my0, mx1, my1, "erase")
		end
		self:setValue("Changed", true)
		self:setCursor(1, mx0, my0)
		self.LockCursorX = false -- !
		return clip
	end
end

function TextEdit:eraseMark()
	return self:cutMark("erase")
end

function TextEdit:pasteClip(c)
	self:insertShiftMark()
	c = c or self.Application:obtainClipboard()
	if c and c[1] then
		local cx, cy = self.CursorX, self.CursorY
		self:suspendWindowUpdate()
		local ncl = self.MultiLine and #c or min(#c, 1)
		if ncl == 1 then
			local line = self:getLineText(cy)
			line:insert(c[1], cx)
			cx = cx + c[1]:len()
		else
			local line0 = self:getLineText(cy) -- reference
			local line1 = self:newSubString(line0, cx) -- copy
			line0:erase(cx, -1)
			line0:insert(c[1], cx)
			cx = c[#c]:len() + 1
			line1:insert(c[#c], 1)
			self:insertLineStr(cy + 1, line1)
			for i = 2, #c - 1 do
				self:insertLineStr(cy + i - 1, c[i])
			end
		end
		for i = 0, ncl do
			-- db.warn("change line %s", cy + i)
			self:changeLine(cy + i)
			self:damageLine(cy + i)
		end
		self:setValue("Changed", true)
		self:setCursor(-1, cx, cy + ncl - 1, 1)
		self.LockCursorX = false -- !
		local lh = self.LineHeight
		local numl = ncl - 1
		self:resize(0, lh * numl, 0, lh * cy)
		self:updateWindow()
		self:releaseWindowUpdate()
	end
end

-------------------------------------------------------------------------------
--	changed_width = initText()
-------------------------------------------------------------------------------

function TextEdit:initText()
	if self:checkFlags(FL_SETUP) then
		local d = self.Data
		if not d.Initialized then
			for i = 1, self:getN() do
				local line = d[i]
				d[i] = self:createLine(line)
			end
			d.Initialized = true
		end
		return self:recalcTextWidth()
	end
end

-------------------------------------------------------------------------------
--	changed_width = recalcTextWidth()
-------------------------------------------------------------------------------

function TextEdit:recalcTextWidth()
	local maxw = 0
	if self.UseFakeCanvasWidth then
		-- text width recalculation avoided
		maxw = FAKECANVASWIDTH
	elseif self:checkFlags(FL_SETUP) then
		local d = self.Data
		for i = 1, self:getN() do
			local line = d[i]
			assert(type(line) ~= "string")
			maxw = max(maxw, line[2])
		end
	end
	if self.TextWidth ~= maxw then
		self.TextWidth = maxw
		-- self:layoutText()
		return true
	end
end

-------------------------------------------------------------------------------
--	w, h = getRealCanvasSize()
-------------------------------------------------------------------------------

function TextEdit:getRealCanvasSize()
	local maxw = self.TextWidth
	local nlines = self:getN()
	local m1, m2, m3, m4 = self:getMargin()
	local w = maxw + m1 + m3 + self.FWidth
	local h = self.LineHeight * nlines + m2 + m4
	self.RealCanvasWidth = w
	return w, h
end

-------------------------------------------------------------------------------
--	updateCanvasSize()
-------------------------------------------------------------------------------

function TextEdit:updateCanvasSize()
	local c = self.Parent
	if c then
		local w, h = self:getRealCanvasSize()
		c:setValue("CanvasWidth", 
			self.UseFakeCanvasWidth and FAKECANVASWIDTH or w)
		c:setValue("CanvasHeight", h)
		c:rethinkLayout()
	end
end

-------------------------------------------------------------------------------
--	newtext(text) - Initialize text from a string or a table of strings.
-------------------------------------------------------------------------------

function TextEdit:newText(text)
	self:endMark()
	local data
	if not text then
		data = { "" }
	elseif type(text) == "string" then
		data = { }
		for l in (text .. "\n"):gmatch("([^\n]*)\n") do
			insert(data, l)
		end
	else -- assuming it's a table
		data = text
	end
	data.Initialized = false
	self.Data = data
	self:initText()
	self:updateCanvasSize()
	self.LockCursorX = false
	self:clearBookmarks()
	self:setValue("FileName", "")
	self:setValue("Changed", false)
	self:setCursor(-1, 1, 1, 0)
-- 	self:damageLine(1)
	if self:checkFlags(FL_SETUP) then
		local c = self.Parent
		c:damageChild(0, 0, c.CanvasWidth, c.CanvasHeight)
	end
end

-------------------------------------------------------------------------------
--	layoutText:
-------------------------------------------------------------------------------

-- function TextEdit:getHeadLineNumber(lnr)
-- 	lnr = lnr or self.CursorY
-- 	local line = self.Data[lnr]
-- 	if line then
-- 		local extl = tonumber(line[1])
-- 		if extl then
-- 			lnr = lnr + extl
-- 		end
-- 		return lnr
-- 	end
-- end
-- 
-- function TextEdit:getLineHead(lnr)
-- 	return self.Data[self:getHeadLineNumber(lnr)]
-- end
-- 
-- function TextEdit:getLinePart(lnr)
-- 	lnr = lnr or self.CursorY
-- 	local line = self:getLine(lnr)
-- 	if line then
-- 		local extl = tonumber(line[1])
-- 		if extl then
-- 			return self:getLine(lnr + extl)[1], line[3], line[4]
-- 		end
-- 		return line[1], line[3] or 1, line[4] or line[1]:len()
-- 	end
-- end
-- 
-- function TextEdit:mergeLine(lnr)
-- 	lnr = self:getHeadLineNumber(lnr)
-- 	local data = self.Data
-- 	lnr = lnr + 1
-- 	while data[lnr] and type(data[lnr]) == "number" do
-- 		remove(lnr)
-- 	end
-- 	return lnr - 1
-- end
-- 
-- function TextEdit:mergeText()
-- 	local data = self.Data
-- 	local lnr = 1
-- 	while lnr <= #data do
-- 		self:mergeLine(lnr)
-- 		lnr = lnr + 1
-- 	end
-- end
-- 
-- function TextEdit:layoutText()
-- 	local r1, _, r3 = self.Parent:getRect()
-- 	if r1 and self.DynWrap then
-- 		local width = r3 - r1 + 1
-- 		db.warn("layout to width: %s", width)
-- 		
-- 		local data = self.Data
-- 		local onl = #data
-- 		
-- 		self:mergeText()
-- 		db.warn("text merged: %s -> %s lines", onl, #data)
-- 
-- 		for lnr = #data, 1, -1 do
-- 			local line = data[lnr]
-- 			local lw = line[2]
-- 			if lw > width then
-- 				db.warn("split line %s", lnr)
-- 				local text = line[1]
-- 				local nlnr = 0
-- 				data[lnr][3] = 1
-- 				
-- 				local x0 = 1
-- 				while lw > 0 do
-- 					local x = self:getCursorByX(text, width)
-- 					assert(x > 1)
-- 					data[lnr + nlnr][4] = x0 + x - 2
-- 					-- db.warn("%d-%d:\t%s", data[lnr + nlnr][3], 
--						data[lnr + nlnr][4], text:sub(1, x - 1))
-- 					local nlw = self:getTextWidth(text, 0, x - 1)
-- 					text = text:sub(x)
-- 					text = InputString:new(text)
-- 					nlnr = nlnr + 1
-- 					x0 = x0 + x - 1
-- 					insert(data, lnr + nlnr, { -nlnr, nlw, x0 })
-- 					lw = lw - nlw
-- 				end
-- 			end
-- 		end
-- 	end
-- end

-------------------------------------------------------------------------------
--	changeLine:
-------------------------------------------------------------------------------

function TextEdit:changeLine(lnr)
	local line = self.Data[lnr or self.CursorY]
	if not line then
		db.info("no line %s", lnr)
		return
	end
	local olen = line[2]
	local nlen = self:getTextWidth(line[1], 0, -1)
	line[2] = nlen
	local tw = self.TextWidth
	if nlen > tw then
		self.TextWidth = line[2]
	elseif olen ~= nlen and olen == tw then
		self:recalcTextWidth()
	end
	if self.TextWidth ~= tw then
		self:getRealCanvasSize()
		local insx = min(tw, self.TextWidth)
		self:resize(self.TextWidth - tw, 0, insx, 0)
		self:updateWindow()
	end
end

-------------------------------------------------------------------------------
--	initFont
-------------------------------------------------------------------------------

function TextEdit:initFont()
	local props = self.Properties
	local fname = self.FontName or props["font"]
	local f = self.Application.Display:openFont(fname)
	self.FontHandle = f
	local fw, lh

	if self.PasswordChar then
		fw, lh = f:getTextSize(self.PasswordChar)
		self.FWidth = fw
		self.FixedFWidth = fw
		self.FixedFont = true
	else
		-- naive test whether the font is fixed width:
		local fwtest = "Wi1Ml|Xw@!/"
		local w
		self.FixedFont = true
		for i = 1, fwtest:len() do
			local nw = f:getTextSize(fwtest:sub(i, i))
			if w and nw ~= w then
				self.FixedFont = false
				break
			end
			w = nw
		end
		fw, lh = f:getTextSize(" ")
		self.FWidth = fw
		if self.FixedFont then
			self.FixedFWidth = fw
		end
	end
	
	if self.Size and self.Parent then
		self.Parent.MinWidth = fw * self.Size
	end
	
	self.LineOffset = floor(self.LineSpacing / 2)
	self.LineHeight = lh + self.LineSpacing
	self:initText()
	self:updateCanvasSize()
end

-------------------------------------------------------------------------------
--	reconfigure: overrides
-------------------------------------------------------------------------------

function TextEdit:reconfigure()
	Sizeable.reconfigure(self)
	self:initFont()
end

-------------------------------------------------------------------------------
--	setup: overrides
-------------------------------------------------------------------------------

function TextEdit:setup(app, window)
	Sizeable.setup(self, app, window)

	local props = self.Properties
	self.BGPens[PENIDX_MARK] = self.BGPens[PENIDX_MARK] or props["findmark-background-color"] or "list-alt"
	self.FGPens[PENIDX_MARK] = self.FGPens[PENIDX_MARK] or props["findmark-color"] or "list-detail"

	self:initFont()
	
	self:addNotify("CursorX", ui.NOTIFY_ALWAYS, NOTIFY_CURSORX)
	self:addNotify("CursorY", ui.NOTIFY_ALWAYS, NOTIFY_CURSORY)
	self:addNotify("FileName", ui.NOTIFY_ALWAYS, NOTIFY_FILENAME)
	self:addNotify("Changed", ui.NOTIFY_ALWAYS, NOTIFY_CHANGED)
end

-------------------------------------------------------------------------------
--	cleanup: overrides
-------------------------------------------------------------------------------

function TextEdit:cleanup()
	self:remNotify("Changed", ui.NOTIFY_ALWAYS, NOTIFY_CHANGED)
	self:remNotify("FileName", ui.NOTIFY_ALWAYS, NOTIFY_FILENAME)
	self:remNotify("CursorY", ui.NOTIFY_ALWAYS, NOTIFY_CURSORY)
	self:remNotify("CursorX", ui.NOTIFY_ALWAYS, NOTIFY_CURSORX)
	self:setEditing(false)
	self.FontHandle = self.Application.Display:closeFont(self.FontHandle)
	Sizeable.cleanup(self)
end

-------------------------------------------------------------------------------
--	show: overrides
-------------------------------------------------------------------------------

function TextEdit:show()
	Sizeable.show(self)
	self.BlinkTick = 0
end

-------------------------------------------------------------------------------
--	layout: overrides
-------------------------------------------------------------------------------

function TextEdit:layout(x0, y0, x1, y1, markdamage)
	local c = self.Cursor
	if c and c[6] == 1 then
		local insx = self.InsertX
		local insy = self.InsertY
		if insx or insy then
			local m1, m2, m3, m4 = self:getMargin()
			local n1 = x0 + m1
			local n2 = y0 + m2
			local n3 = x1 - m3
			local n4 = y1 - m4
			local r1, r2, r3, r4 = self:getRect()
			markdamage = markdamage ~= false
			if n1 == r1 and n2 == r2 and markdamage and 
				self:checkFlags(FL_TRACKDAMAGE) then
				
				local dw = n3 - r3
				local dh = n4 - r4
				if dw ~= 0 and insx and c[1] > insx then
					c[1] = c[1] + dw
					c[3] = c[3] + dw
				end
				if dh ~= 0 and insy and c[2] > insy then
					c[2] = c[2] + dh
					c[4] = c[4] + dh
				end
			end
		end
	end
	return Sizeable.layout(self, x0, y0, x1, y1, markdamage)
end

-------------------------------------------------------------------------------
--	draw: overrides
-------------------------------------------------------------------------------

function TextEdit:draw()
	local dr = self.DamageRegion
	if dr then
		-- repaint intra-area damagerects:
		local d = self.Window.Drawable
		d:setFont(self.FontHandle)
		dr:forEach(self.drawPatch, self)
		self.DamageRegion = false
	end
end

-------------------------------------------------------------------------------
--	drawPatch:
-------------------------------------------------------------------------------

function TextEdit:drawPatch(r1, r2, r3, r4)

	local d = self.Window.Drawable
	local data = self.Data
	local x, y = self:getRect()
	local lh = self.LineHeight
	local lo = self.LineOffset
	local l0, l1 
	local password = self.PasswordChar

	local bgpen, tx, ty = self:getBG()
	local m1, m2 = self:getMargin()
	d:setBGPen(bgpen, tx - m1, ty - m2)

--	if self.Wrap then
--
--	else
		-- line height is constant:
		l0 = floor((r2 - y) / lh) + 1
		l1 = floor((r4 - y) / lh) + 1
--	end

	d:pushClipRect(r1, r2, r3, r4)

	local r = Region.new(r1, r2, r3, r4)

	for lnr = l0, l1 do
		local x0, y0, x1, y1 = self:getLineGeometry(lnr)
		local x2 = x0
		self:foreachSnippet(lnr, function(self, pos, snip, vx, gx0, gx1,
			fgpen, bgpen, ly)
			if ly > 0 then
				return true -- cannot handle multiline for now
			end
			local x1 = x0 + gx1
			local x0 = x0 + gx0
			if x0 > r3 then
				return true -- past the patch, break loop
			end
			if intersect(x0, y0, x1, y1, r1, r2, r3, r4) then
				r:subRect(x0, y0, x1, y1)
				d:fillRect(x0, y0, x1, y1, bgpen)
				if password then
					local slen = snip:len()
					d:drawText(x0, y0 + lo, x1, y1 - lo, password:rep(slen), 
						fgpen)
				else
					d:drawText(x0, y0 + lo, x1, y1 - lo, snip:get(), fgpen)
				end
			end
			x2 = x1 + 1
		end)
		local linebg = self:getLinePens(lnr, "lineend")
		r:subRect(x2, y0, x1, y1)
		d:fillRect(x2, y0, x1, y1, linebg)
	end

	r:forEach(d.fillRect, d)
	
	local c = self.Cursor
	if c and intersect(r1, r2, r3, r4, c[1], c[2], c[3], c[4]) then
		if c[6] == 1 then
			local cs = self.CursorStyle
			if cs == "block" then
				d:fillRect(c[1], c[2], c[7], c[4], "cursor")
				d:drawText(c[1], c[2] + lo, c[7], c[4] + lo, c[5],
					"cursor-detail")
			elseif cs == "line" or cs == "bar+line" then
				d:fillRect(c[1], c[2], c[7], c[4], c[9])
			end
		end
	end

	d:popClipRect()
end

-------------------------------------------------------------------------------
--	onFocus: overrides
-------------------------------------------------------------------------------

function TextEdit:onFocus()
	Sizeable.onFocus(self)
	self:setEditing(not self.Disabled and self.Focus)
end

-------------------------------------------------------------------------------
--	onSelect: overrides
-------------------------------------------------------------------------------

function TextEdit:onSelect()
	Sizeable.onSelect(self)
	self:setEditing(not self.Disabled and self.Selected)
end

-------------------------------------------------------------------------------
--	old_editing = setEditing(boolean)
-------------------------------------------------------------------------------

function TextEdit:setEditing(onoff)
	local old_editing = self.Editing
	local w = self.Window
	if onoff and not old_editing then
		self:setCursor(-1)
		self.Editing = true
		if self:checkFlags(ui.FL_SETUP) then
			w:addInputHandler(ui.MSG_INTERVAL, self, self.updateInterval)
			w:addInputHandler(ui.MSG_KEYUP + ui.MSG_KEYDOWN, self,
				self.handleKeyboard)
		end
		self:setValue("FileName", self.FileName, true)
	elseif not onoff and old_editing then
		self:setCursor(0)
		self.Editing = false
		self:setValue("Selected", false)
		if self:checkFlags(ui.FL_SETUP) then
			w:remInputHandler(ui.MSG_KEYUP + ui.MSG_KEYDOWN, self,
				self.handleKeyboard)
			w:remInputHandler(ui.MSG_INTERVAL, self, self.updateInterval)
		end
	end
	return old_editing
end

-------------------------------------------------------------------------------
--	
-------------------------------------------------------------------------------

function TextEdit:damageLine(l)
	if self:checkFlags(FL_READY) then
		local x0, y0, x1, y1 = self:getLineRect(l, 1)
		if x0 then
			self.Parent:damageChild(x0, y0, x1, y1)
		else
			db.info("no text in line %s", l)
		end
	end
end

-------------------------------------------------------------------------------
--	added = addChar(utf8): Adds an utf8 character to the text. By adding a
--	newline ({{"\n"}}) or CR ({{"\r"}}), a new line is produced, {{"\127"}}
--	(DEL) deletes the character under the cursor, {{"\008"}} performs a
--	backspace.
-------------------------------------------------------------------------------

function TextEdit:addChar(utf8)
	if utf8 == "\127" then
		return self:delChar()
	elseif utf8 == "\008" then
		return self:backSpace()
	elseif utf8 == "\013" or utf8 == "\010" then
		return self:enter()
	end
	local cx, cy = self:getCursor()
	local line = self:getLine()
	local text = line[1]
	if not self.MaxLength or text:len() < self.MaxLength then
		text:insert(utf8, cx)
		self:setValue("Changed", true)
		self:changeLine(cy)
		if self:checkFlags(FL_READY) then
			local x0, y0, x1, y1 = self:getLineRect(cy, cx, -1)
			self.Parent:damageChild(x0, y0, x1, y1)
		end
		return true -- inserted
	end
end

function TextEdit:remChar()
	local cx, cy = self:getCursor()
	local line = self:getLine()
	local text = line[1]
	-- add width of the deleted char to refresh area:
	local w = self:getTextWidth(text, cx, cx)
	text:erase(cx, cx)
	self:setValue("Changed", true)
	self:changeLine()
	self:setCursor(-1)
	if self:checkFlags(FL_READY) then
		local x0, y0, x1, y1 = self:getLineRect(cy, cx, -1)
		self.Parent:damageChild(x0, y0, x1 + w, y1)
	end
end

function TextEdit:newString(text)
	return String:new():set(text)
end

-- new string object from substring of string object (copies metadata)
function TextEdit:newSubString(text, p0, p1)
	return String:new():set(text, p0, p1)
end

-- NOTE: after calling this function, consider text being used as a reference!
function TextEdit:createLine(text)
	text = type(text) == "string" and self:newString(text) or text
	return { text, self:getTextWidth(text, 0, -1) }
end

function TextEdit:followCursor()
	local follow = self.FollowCursor
	if follow then
		local x0, y0, x1, y1 = unpack(self.Cursor, 1, 4)
		local m1, m2, m3, m4 = self:getMargin()
		local v = self.VisibleMargin
		local fw, lh = self.FWidth, self.LineHeight
		x0 = x0 - m1 - fw * v[1]
		y0 = y0 - m2 - lh * v[2]
		x1 = x1 + m3 + fw * v[3]
		y1 = y1 + m4 + lh * v[4]
		local smooth = follow * (self.DragScroll or 0)
		local cw = self.Parent.CanvasWidth
		self.Parent.CanvasWidth = self.RealCanvasWidth
		if self.Parent:focusRect(x0, y0, x1, y1, smooth) then
			self:setFlags(ui.FL_REDRAW)
		else
			self.FollowCursor = false
		end
		self.Parent.CanvasWidth = cw
	end
end

-------------------------------------------------------------------------------
--	insertLineStr(lnr, str)
--	Note, str is being relinquished by caller!
-------------------------------------------------------------------------------

function TextEdit:insertLineStr(lnr, str, bmdelta)
	bmdelta = bmdelta or 0
	local line = self:createLine(str)
	insert(self.Data, lnr or #self.Data, line)
	local idx = self:findBookmark(lnr + bmdelta)
	if idx then
		if self:checkBookmark(lnr + bmdelta) then
			idx = idx - 1
		end
		local b = self.Bookmarks
		for i = idx, #b do
			b[i] = b[i] + 1
		end
	end
	self:setValue("Changed", true)
end

-------------------------------------------------------------------------------
--	removeLine(lnr)
-------------------------------------------------------------------------------

function TextEdit:removeLine(lnr, remove_bookmark)
	
	if self:checkBookmark(lnr) and self:checkBookmark(lnr + 1) then
		-- bookmark in this and next line, merge:
		remove_bookmark = true
	end

	if remove_bookmark then
		self:removeBookmark(lnr)
	end
	
	local idx, blnr = self:findBookmark(lnr)
	if idx then
		local b = self.Bookmarks
		for i = idx, #b do
			b[i] = b[i] - 1
		end
	end
	local line = remove(self.Data, lnr)
	self:setValue("Changed", true)
	if line[2] == self.TextWidth then
		self:initText()
	end
end

-------------------------------------------------------------------------------
--
-------------------------------------------------------------------------------

function TextEdit:getN()
	return #self.Data
end

function TextEdit:getNumLines()
	local d = self.Data
	local nl = #d
	if nl == 1 and d[1][1]:len() == 0 then -- do not count empty line 1:
		return 0
	end
	return nl
end

function TextEdit:getCursor(visual)
	if visual then
		return self.VisualCursorX, self.CursorY
	end
	return self.CursorX, self.CursorY
end

function TextEdit:getLine(lnr)
	lnr = lnr or self.CursorY
-- 	assert(lnr >= 1 and lnr <= #self.Data)
	local line = self.Data[lnr]
-- 	assert(line)
	return line, lnr
end

function TextEdit:getLineLength(lnr)
	local line, lnr = self:getLine(lnr)
-- 	assert(line, "no text line: "..lnr)
-- 	assert(line[1], "no text line: " .. lnr)
-- 	local line = self:getLineHead(lnr)
	return line and line[1]:len()
end

--
--	the resulting value is a reference, no copy!
--
function TextEdit:getLineText(lnr)
	local line, lnr = self:getLine(lnr)
-- 	assert(line)
-- 	assert(line[1], "no text line: " .. lnr)
-- 	local line = self:getLineHead(lnr)
	return line and line[1]
end

-- function TextEdit:getLineWidth(lnr, x)
-- 	local line = self:getLine(lnr)
-- 	if line then
-- 		if not x then
-- 			return line[2]
-- 		end
-- 		return self:getTextWidth(line[1], 0, x)
-- 	end
-- end

function TextEdit:getScreenSize(in_chars)
	local r1, r2, r3, r4 = self.Parent:getRect()
	local w = r3 - r1 + 1
	local h = r4 - r2 + 1
	if in_chars then
		w = floor(w / self.FWidth)
		h = floor(h / self.LineHeight)
	end
	return w, h
end

function TextEdit:getRawTextWidth(text, a, b)
	if a == 0 and b == 0 then
		return 0
	end
	local fw = self.FixedFWidth
	if fw then
		return fw * (b - a + 1)
	end
	return self.FontHandle:getTextSize(text:sub(a, b))
end

function TextEdit:getPens(mode)
	local stateextra = self.Disabled and ":disabled" or ""
	local props = self.Properties
	if mode == "mark" then
		return props["mark-background-color" .. stateextra] or "cursor",
			props["mark-color" .. stateextra] or "cursor-detail"
	elseif mode == "bookmark" then
		return props["bookmark-background-color" .. stateextra] or "list-alt",
			props["bookmark-color" .. stateextra] or "list-detail"
	elseif mode == "cursorline" then
		return props["cursor-background-color" .. stateextra] or "list-alt", 
			props["cursor-color" .. stateextra] or "list-detail"
	else
		return self:getBG(), props["color" .. stateextra] or "list-detail"
	end
end

function TextEdit:getLinePens(cy, mode)
	local cx = (self:getLineLength(cy) or 0) + 1
	if self:checkInMark(cx, cy) and (mode == "lineend" or
		cx == 1 or self:checkInMark(1, cy)) then
		return self:getPens("mark")
	end
	if self:checkBookmark(cy) then
		return self:getPens("bookmark")
	end
	local cs = self.CursorStyle
	if (cs == "bar+line" or cs == "bar") and cy == self.CursorY then
		return self:getPens("cursorline")
	end
	return self:getPens()
end

function TextEdit:foreachSnippet(lnr, func)
	local text = self:getLineText(lnr)
	if not text then
		db.info("no text in line %s!", lnr)
		return
	end
	local idx, vx, gx = 0, 0, 0
	local pos0, pos1
	local pen_bg, pen_fg = self:getLinePens(lnr)
	local mark_bg, mark_fg = self:getPens("mark")
	local maxwidth = self.AutoWrap and 
		self.Parent.CanvasWidth or FAKECANVASWIDTH
	local ly = 0 -- intra line nr
	
	while true do
		local snipvx, snipgx
		local fgpen = pen_fg
		local bgpen, snip
		local char1, meta1 = text:getval(idx + 1)
		local mark1 = self:checkInMark(idx + 1, lnr)
		local break_line
		
		while char1 do
			idx = idx + 1
			local char, meta, mark = char1, meta1, mark1
			pos1 = idx
			if not pos0 then
				pos0 = pos1
				snipvx = vx
				snipgx = gx
				fgpen = mark and mark_fg or self.FGPens[meta] or pen_fg
				bgpen = mark and mark_bg or self.BGPens[meta] or pen_bg
			end
			vx = vx + 1
			if char == 9 then
				local tabsize = self.TabSize
				local inslen = tabsize - ((vx - 1) % tabsize)
				pos1 = pos1 + inslen
				snip = self:newString((" "):rep(inslen))
				vx = vx + inslen - 1 -- -1 because non-printable
				break
			end
			if maxwidth < FAKECANVASWIDTH and
				gx + self:getRawTextWidth(text, pos0, pos1 + 1) > maxwidth then
				break_line = true
				break
			end
			if pos0 and pos1 - pos0 > 32 then
				break
			end
			mark1 = self:checkInMark(pos1 + 1, lnr)
			if mark1 ~= mark then
				break
			end
			char1, meta1 = text:getval(pos1 + 1)
			if char1 == 9 or meta1 ~= meta then
				break
			end
		end
		
		if not pos0 then
			return
		end

		snip = snip or self:newSubString(text, pos0, pos1)
		local gwidth = self:getRawTextWidth(snip, 1, snip:len())
		gx = gx + gwidth
		local ret_pos0 = pos0
		pos0 = nil
		
		if func(self, ret_pos0, snip, snipvx, snipgx, snipgx + gwidth - 1,
			fgpen, bgpen, ly) then
			break
		end
		
		if break_line then
			gx = 0
			ly = ly + 1
		end
	end
	
end

-------------------------------------------------------------------------------
--	x = getVisualCursorX(cx, cy)
--	for a cursor position, get visual position on
--	display (considering tabulators)
-------------------------------------------------------------------------------

function TextEdit:getVisualCursorX(cx, cy)
	local text = self:getLineText(cy or self.CursorY)
	return text and text:getVisualPos(self.TabSize, cx or self.CursorX) or 1
end

-------------------------------------------------------------------------------
--	x = getRealCursorX(cx, cy)
--	for a visual position, get real position on text (considering tabulators)
-------------------------------------------------------------------------------

function TextEdit:getRealCursorX(vx, cy)
	local rx = 0
	local text = self:getLineText(cy or self.CursorY)
	if text then
		text:forEachChar(self.TabSize, function(a, c, v)
			if v >= vx then
				return a
			end
			rx = a
		end)
	end
	return rx + 1
end

-------------------------------------------------------------------------------
--	getTextWidth(text, p0, p1)
--	take into account tabs
-------------------------------------------------------------------------------

function TextEdit:getTextWidth(text, p0, p1)
	return text:getTextWidth(self.TabSize, p0, p1, 
		self.FixedFWidth or self.getRawTextWidth, self, self.FWidth)
end

-------------------------------------------------------------------------------
--	getLineGeometry
-------------------------------------------------------------------------------

function TextEdit:getLineGeometry(lnr, mode)
	local r1, r2, r3, r4 = self:getRect()
	local lh = self.LineHeight
	local y0 = r2 + (lnr - 1) * lh
	local y1 = y0 + lh - 1
	return r1, y0, r3, y1
end

-------------------------------------------------------------------------------
--	x0, y0, x1, y1 = getLineRect(line[, xstart[, xend]]) - Gets a line's
--	rectangle on the screen. If {{xend}} is less than {{0}}, it is added to
--	the character position past the end the string. If {{xend}} is omitted,
--	returns the right edge of the canvas.
-------------------------------------------------------------------------------

function TextEdit:getLineRect(lnr, xstart, xend)
	local line = self:getLine(lnr)
	if line then
		local x0, y0, r3, y1 = self:getLineGeometry(lnr)
		local x1
		if xstart and xstart <= 1 and xend and xend == -1 then
			return x0, y0, x0 + line[2], y1
		end
		line = line[1]
		local f = self.FontHandle
		if xstart then
			x0 = x0 + self:getTextWidth(line, 1, xstart - 1)
		end
		if not xend then
			x1 = r3
		else
			if xend < 0 then
				xend = line:len() + 1 + xend
			end
			local w = self:getTextWidth(line, xstart, xend) - 1
			x1 = x0 + w - 1 + self.FWidth
		end
		return x0, y0, x1, y1
	end
end

-------------------------------------------------------------------------------
--	setCursor(blink, cx, cy, follow) - blink=1 cursor on, blink=0 cursor off,
--	blink=-1 cursor on in next frame, blink=false no change. follow: 
--	false=do not follow cursor, -1=visible area jumps to the cursor
--	immediately, 1=visible area moves gradually (if enabled)
-------------------------------------------------------------------------------

function TextEdit:setCursor(bs, cx, cy, follow)

	local c = self.Cursor
	local changed = not c
	local obs = self.BlinkState
	local ocx, ocy = self:getCursor()
	local ychanged

	if bs and bs < 0 then
		self.BlinkTick = 0
		bs = 0
		changed = true
	end

	if bs then
		changed = bs ~= obs
		self.BlinkState = bs
	else
		bs = obs
	end

	local nodamage = self.SuspendUpdateNest > 0 or
		self.CursorMode == "hidden" or self.CursorStyle == "none"
	
	if cx or cy then
		if cy and cy ~= ocy then
			assert(cy <= self:getN())
			ychanged = true
			self:setValue("CursorY", cy)
			changed = true
			local m = self.Mark
			if m and not nodamage then
				local y0, y1 = min(ocy, cy), max(ocy, cy)
				for i = y0, y1 do
					self:damageLine(i)
				end
			end
		else
			cy = ocy
		end

		if cx and cx < 0 then
			cx = self:getLineLength() + 2 + cx
		end

		if cx and cx ~= ocx then
			self:setValue("CursorX", cx)
			changed = true
		else
			cx = ocx
		end
	else
--		changed = true
		cx = ocx
		cy = ocy
	end

	if not changed and not follow then
		return
	end

	if self:checkFlags(FL_READY) then
		local d = self.Window.Drawable
		local r1, r2, r3, r4 = self:getRect()
		local text = self:getLineText()
		local textc = text:getchar(cx)
		if textc == "" or textc == "\t" then
			textc = " "
		end
		local cw = self.FixedFWidth or self.FontHandle:getTextSize(textc)
		local h = self.LineHeight
		local x0 = r1 + self:getTextWidth(text, 1, cx - 1)

		local x1 = x0 + cw - 1
		local x1paint = self.CursorStyle == "block" and x1 or
			x0 + min(cw - 1, self.CursorThickness)

		local y0 = r2 + h * (cy - 1)
		local y1 = y0 + h - 1

		local cs = self.CursorStyle
		local clchanged = ychanged and (cs == "bar" or cs == "bar+line")
		
		if c then
			-- old cursor, and visible?
			if c[6] and 
				(c[1] ~= x0 or c[2] ~= y0 or c[3] ~= x1 or c[4] ~= y1) then
				local x0 = min(x0, c[1])
				local x1 = max(x1, c[3])
				if clchanged then
					x0 = r1
					x1 = r3
				end
				if not nodamage then
					self.Parent:damageChild(x0, min(y0, c[2]),
						x1, max(y1, c[4]))
				end
			end
		else
			c = { }
			self.Cursor = c
		end

		c[1] = x0
		c[2] = y0
		c[3] = x1 -- x1 of the positioning rectangle
		c[4] = y1
		c[5] = textc -- the character
		c[6] = bs -- blinkstate (0,1)
		c[7] = x1paint -- x1 of the painted rectangle!
		c[8], c[9] = self:getLinePens(cy)

		if follow then
			self.FollowCursor = follow
		end
		
		if clchanged then
			x0 = r1
			x1 = r3
		end
		if not nodamage then
			self.Parent:damageChild(x0, y0, x1, y1)
		end
	end
end

-------------------------------------------------------------------------------
--	cx, cy[, lockcx] = moveCursorPosition(cx, cy, dx, dy[, lockcx])
--	Handle cursor positioning
-------------------------------------------------------------------------------

function TextEdit:moveCursorPosition(cx, cy, dx, dy, lockcx)
	local nlines = self:getN()
	local len = self:getLineLength(cy)
	cx = cx + dx
	if dx > 0 and cx > len + 1 then
		if cy < nlines then
			cy = cy + 1
			cx = 1
		else
			cx = len + 1
		end
	elseif dx < 0 and cx == 0 then
		lockcx = false
		if cy > 1 then
			cy = cy - 1
			cx = self:getLineLength(cy) + 1
		else
			cx = 1
		end
	end
	if dx ~= 0 then
		lockcx = self:getVisualCursorX(cx)
	end
	if dy > 0 and cy < nlines then
		cy = min(cy + dy, nlines)
		local vx = lockcx
		if vx then
			cx = self:getRealCursorX(vx, cy)
		end
		cx = min(cx, self:getLineLength(cy) + 1)
	elseif dy < 0 and cy > 1 then
		cy = max(cy + dy, 1)
		local vx = lockcx
		if vx then
			cx = self:getRealCursorX(vx, cy)
		end
		cx = min(cx, self:getLineLength(cy) + 1)
	end
	return cx, cy, lockcx
end

-------------------------------------------------------------------------------
--	moveCursor()
-------------------------------------------------------------------------------

function TextEdit:moveCursor(dx, dy)
	local cx, cy = self:getCursor()
	local lockcx 
	cx, cy, self.LockCursorX = self:moveCursorPosition(cx, cy, dx, dy,
		self.LockCursorX)
	self:setCursor(-1, cx, cy, 1)
end

function TextEdit:cursorLeft()
	self:moveCursor(-1, 0)
end

function TextEdit:cursorRight()
	self:moveCursor(1, 0)
end

function TextEdit:cursorUp()
	self:moveCursor(0, -1)
end

function TextEdit:cursorDown()
	self:moveCursor(0, 1)
end

function TextEdit:backSpace()
	local cx, cy = self:getCursor()
	if cx > 1 then
		self:cursorLeft()
		self:remChar()
		return false
	elseif cy > 1 then
		self:suspendWindowUpdate()
		local lh = self.LineHeight
		local prevline = self:getLineText(cy - 1)
		local thisline = self:getLineText(cy)
		local cx = prevline:len() + 1
		thisline:insert(prevline, 1)
		cy = cy - 1
		self:removeLine(cy)
		self:changeLine(cy)
		self:resize(0, -lh, 0, lh * cy)
		self:damageLine(cy)
		self:setCursor(-1, cx, cy, 1)
		self:updateWindow()
		self:releaseWindowUpdate()
	end
end

function TextEdit:delChar()
	local cx, cy = self:getCursor()
	local lh = self.LineHeight
	local curline = self:getLineText(cy)
	local curlen = self:getLineLength(cy)
	self:suspendWindowUpdate()
	if cx < curlen + 1 then
		self:remChar()
	elseif cy < self:getN() then
		local nextline = self:getLineText(cy + 1)
		nextline:insert(curline, 1)
		self:removeLine(cy)
		self:changeLine(cy)
		self:damageLine(cy)
		self:resize(0, -lh, 0, lh * cy)
		self:setCursor(-1)
		self:updateWindow()
	end
	self:releaseWindowUpdate()
end

function TextEdit:enter(followcursor)
	local cx, cy = self:getCursor()
	local lh = self.LineHeight
	local oldline = self:getLineText(cy)

	local indentstr
	if self.AutoIndent then
		indentstr = self:newString()
		for i = 1, oldline:len() do
			local c = oldline:getchar(i)
			if not c then
				break
			end
			if c == "\t" or c == " " then
				indentstr:insert(c)
			else
				break
			end
		end
	end

	self:suspendWindowUpdate()
	self:setCursor(0)
	self:resize(0, lh, 0, lh * cy)
	self:damageLine(cy)
	cy = cy + 1
	
	local newcx = 1
	if cx <= 1 then
		local newline = self:newString()
		if indentstr then
			newline:insert(indentstr, 1)
			newcx = indentstr:len() + 1
		end
		self:insertLineStr(cy - 1, newline)
	elseif cx >= oldline:len() then
		local newline = self:newString()
		if indentstr then
			newline:insert(indentstr, 1)
			newcx = indentstr:len() + 1
		end
		self:insertLineStr(cy, newline, -1)
	else
		local newline = self:newSubString(oldline, cx)
		if indentstr then
			newline:insert(indentstr, 1)
			newcx = indentstr:len() + 1
		end
		self:insertLineStr(cy, newline, -1)
		oldline:erase(cx, -1)
		-- set userdata in both strings:
		newline:attachdata(oldline:getdata())
	end
	
	self:changeLine(cy - 1)
	self:changeLine(cy)
	self:setCursor(-1, newcx, cy, followcursor == nil and 1)
	self:updateWindow()
	self:releaseWindowUpdate()
end

function TextEdit:cursorEOF()
	local cy = self:getN()
	local cx = self:getLineLength(cy) + 1
	self.LockCursorX = cx -- false?
	self:setCursor(-1, cx, cy, 1)
end

function TextEdit:cursorSOF()
	self.LockCursorX = 1	-- false?
	self:setCursor(-1, 1, 1, 1)
end

function TextEdit:cursorEOL()
	self.LockCursorX = self:getVisualCursorX(self:getLineLength()) + 1
	self:setCursor(-1, -1, false, 1)
end

function TextEdit:cursorSOL()
	self.LockCursorX = 1	-- false?
	self:setCursor(-1, 1, false, 1)
end

function TextEdit:pageUp()
	local _, sh = self:getScreenSize(true)
	self:moveCursor(0, -sh)
end

function TextEdit:pageDown()
	local _, sh = self:getScreenSize(true)
	self:moveCursor(0, sh)
end

function TextEdit:getCursorByX(text, x)
	if x < 0 then
		return 1
	end
	local p0 = 0
	local p1 = text:len()
	local pn
	local x0 = 0
	local x1 = self:getTextWidth(text, 1, p1)
	local xn
	while true do
		if x >= x1 then
			return p1 + 1
		end
		if p0 + 1 == p1 then
			return p1
		end
		pn = p0 + floor((p1 - p0) / 2)
		xn = self:getTextWidth(text, 1, pn)
		if xn > x then
			p1 = pn
			x1 = xn
		else
			p0 = pn
			x0 = xn
		end
	end
end

function TextEdit:getCursorByXY(x, y)
	local cy = floor(y / self.LineHeight) + 1
	if cy < 1 then
		return 1, 1
	end
	local nl = self:getN()
	if cy > nl then
		local cx = self:getCursorByX(self:getLineText(nl), x)
		return cx, nl
	end
	local cx = self:getCursorByX(self:getLineText(cy), x)
	return cx, cy
end

function TextEdit:setCursorByXY(x, y, opt)
	if not self.Disabled then
		self:setEditing(true)
		local m1, m2 = self:getMargin()
		x, y = self:getCursorByXY(x - m1, y - m2)
		self:setCursor(opt or -1, x, y, 0)
		self.LockCursorX = self:getVisualCursorX()
	end
end

function TextEdit:deleteLine(dy)
	local nl = self:getN()
	if nl > 0 then
		self:suspendWindowUpdate()
		local cx, cy = self:getCursor()
		dy = dy or cy
		if nl > 1 then
			self:removeLine(dy, true)
			if cy == nl then
				cy = cy - 1
			end
			local lh = self.LineHeight
			self:resize(0, -lh, 0, lh * dy)
			self:damageLine(cy)
			self:updateWindow()
			self:updateCanvasSize()
		else
			self:getLineText(1):set("")
			self:changeLine(dy)
			self:damageLine(dy)
		end
		self:setCursor(-1, 1, cy, 1)
		self:releaseWindowUpdate()
	end
end

-------------------------------------------------------------------------------
--	breakText(readfunc, tab)
--	break text at lineends and insert strings into table
-------------------------------------------------------------------------------

function TextEdit.breakText(readfunc, data)
	if type(readfunc) == "string" then
		local s = readfunc
		readfunc = function()
			local res = s
			s = nil
			return res
		end
	end
	local buf = ""
	data = data or { }
	while true do
		local nbuf = readfunc()
		if not nbuf then
			break
		end
		buf = buf .. nbuf
		while true do
			local p, p1 = buf:find("\r?\n")
			if not p then
				break
			end
			insert(data, buf:sub(1, p - 1))
			buf = buf:sub(p1 + 1)
		end
	end
	insert(data, buf)
	return data
end

-------------------------------------------------------------------------------
--	loadText(filename)
-------------------------------------------------------------------------------

function TextEdit:loadText(fname)
	local f = open(fname)
	if f then
		local data = self.breakText(function() return f:read(4096) end)
		self:newText(data)
		self:setValue("FileName", fname)
		return true
	end
end

-------------------------------------------------------------------------------
--	saveText(filename)
-------------------------------------------------------------------------------

function TextEdit:saveText(fname)
	local f, msg = open(fname, "wb")
	if f then
		local numl = self:getN()
		local n = 0
		for lnr = 1, numl do
			f:write(self:getLineText(lnr):get())
			n = n + 1
			if lnr < numl then
				f:write("\n")
			end
		end
		f:close()
		db.info("lines saved: %s", n)
		self:setValue("FileName", fname)
		self:setValue("Changed", false)
		return true
	end
	return false, msg
end

-------------------------------------------------------------------------------
--	updateMark()
-------------------------------------------------------------------------------

function TextEdit:updateMark()
end

-------------------------------------------------------------------------------
--	updateCursorMark(msg, funcname)
-------------------------------------------------------------------------------

function TextEdit:updateCursorMark(msg, funcname)
	if self.MarkMode == "shift" then
		local qual = msg and msg[6]
		local shift = qual and (qual >= 1 and qual <= 3)
		if shift and not self.Mark then
			self:doMark()
		elseif not shift and self.Mark then
			self:doMark()
		end
	end
	
	self[funcname](self)
	
	if self.Mark then
		self:updateMark()
	end
end

-------------------------------------------------------------------------------
--	insertShiftMark()
-------------------------------------------------------------------------------

function TextEdit:insertShiftMark()
	if self.MarkMode ~= "shift" then
		return
	end
	if self.Mark then
		self:eraseMark()
		return true
	end
end

-------------------------------------------------------------------------------
--	handleKeyboard()
-------------------------------------------------------------------------------

function TextEdit:handleKeyboard(msg)

	if not self:checkFlags(FL_READY) or self.Window.ActivePopup then
		return msg
	end

	if msg[2] == ui.MSG_KEYDOWN then
		local code = msg[3]
		local qual = msg[6]
		local utf8code = msg[7]
		local ml = self.MultiLine
		local ro = self.ReadOnly
		-- db.warn("code: %s - qual: %s", code, qual)
		while true do
			if qual == 4 and code == 8 then
				-- db.warn("ctrl+backspace")
				break
			elseif qual == 4 and code == 127 then
				-- db.warn("ctrl+delete")
				break
			end
			if qual > 3 or code == 27 then -- ctrl, alt, esc, ...
				if ml and (code == 0xf023 or code == 0xf024) then
					-- relinquish qual+pageup/down
				else
					break -- do not aborb
				end
			elseif code == 13 and ml and not ro then
				self:insertShiftMark(msg)
				self:enter()
			elseif code == 8 and not ro then
				if not self:insertShiftMark(msg) then
					self:backSpace()
				end
			elseif code == 127 and not ro then
				if not self:insertShiftMark(msg) then
					self:delChar()
				end
			elseif code == 0xf010 then
				self:updateCursorMark(msg, "cursorLeft")
			elseif code == 0xf011 then
				self:updateCursorMark(msg, "cursorRight")
			elseif ml and code == 0xf012 then
				self:updateCursorMark(msg, "cursorUp")
			elseif ml and code == 0xf013 then
				self:updateCursorMark(msg, "cursorDown")
			elseif ml and code == 0xf023 then
				self:updateCursorMark(msg, "pageUp")
			elseif ml and code == 0xf024 then
				self:updateCursorMark(msg, "pageDown")
			elseif code == 0xf025 then
				self:updateCursorMark(msg, "cursorSOL")
			elseif code == 0xf026 then
				self:updateCursorMark(msg, "cursorEOL")
			elseif (code == 9 or (code > 31 and 
				(code < 0xe000 or code > 0xf8ff))) and not ro then
				self:suspendWindowUpdate()
				self:insertShiftMark(msg)
				if self:addChar(utf8code) then
					self:cursorRight()
				end
				self:releaseWindowUpdate()
			else
				db.trace("unhandled keycode")
				break
			end
			return false -- absorb
		end
	end
	return msg
end

-------------------------------------------------------------------------------
--	updateInterval:
-------------------------------------------------------------------------------

function TextEdit:updateInterval(msg)
	if self.Window.ActivePopup then
		return msg
	end
	
	self:followCursor()

	if self.CursorMode == "still" then
		self:setCursor(1)
		return msg
	elseif self.CursorMode == "hidden" then
		self:setCursor(0)
		return msg
	end
	
	if not self.BlinkCursor then
		if self.BlinkState ~= 1 then
			self:setCursor(1)
		end
		return msg
	end
	
	local bt = self.BlinkTick
	bt = bt - 1
	if bt < 0 then
		bt = self.BlinkTickInit
		local bs = not self.BlinkCursor and 1 
			or ((self.BlinkState == 1) and 0) or 1
		self:setCursor(bs)
	end
	self.BlinkTick = bt
	return msg
end

-------------------------------------------------------------------------------
--	setState: overrides
-------------------------------------------------------------------------------

function TextEdit:setState(bg, fg)
	Sizeable.setState(self, bg or self.BGPen, fg or self.FGPen)
end

-------------------------------------------------------------------------------
--	onSetCursorX(cx)
-------------------------------------------------------------------------------

function TextEdit:onSetCursorX(cx)
	local vx = self:getVisualCursorX(cx)
	self.VisualCursorX = vx
end

-------------------------------------------------------------------------------
--	onSetCursorY(cy)
-------------------------------------------------------------------------------

function TextEdit:onSetCursorY(cy)
end

-------------------------------------------------------------------------------
--	onSetFileName(fname)
-------------------------------------------------------------------------------

function TextEdit:onSetFileName(fname)
end

-------------------------------------------------------------------------------
--	onSetChanged(changed)
-------------------------------------------------------------------------------

function TextEdit:onSetChanged(changed)
end

-------------------------------------------------------------------------------
--	getText()
-------------------------------------------------------------------------------

function TextEdit:getText()
	local t = { }
	local d = self.Data
	for lnr = 1, #d do
		insert(t, d[lnr][1]:get())
	end
	return concat(t, "\n")
end

-------------------------------------------------------------------------------
--	find(text)
-------------------------------------------------------------------------------

function TextEdit:find(search)
	local sx, sy = self:moveCursorPosition(self.CursorX, self.CursorY, 1, 0, 
		true)
	local numl = self:getN()
	for y = 0, numl - 1 do
		local cy = ((sy - 1 + y) % numl) + 1
		local line = self:getLineText(cy)
		local cx = line:find(search, sx)
		if cx then
			self:setCursor(-1, cx, cy, 1)
			return true
		end
		sx = 1
	end
end

-------------------------------------------------------------------------------
--	mark(lnr, p0, p1)
-------------------------------------------------------------------------------

function TextEdit:clearMark()
	local numl = self:getN()
	for y = 1, numl do
		local line = self:getLineText(y)
		local damage
		for x = 1, line:len() do
			local _, meta = line:getval(x)
			if meta then
				line:setmetadata(0, x)
				damage = true
			end
		end
		if damage then
			self:damageLine(y)
		end
	end	
end

function TextEdit:mark(line, lnr, p0, p1)
	line:setmetadata(PENIDX_MARK, p0, p1)
end

function TextEdit:markWord(text)
	local marklen = String:new():set(text):len()
	local numl = self:getN()
	for y = 1, numl do
		local line = self:getLineText(y)
		local p0 = 0
		local damage
		while true do
			p0 = line:find(text, p0 + 1)
			if not p0 then
				break
			end
			self:mark(line, y, p0, p0 + marklen - 1)
			damage = true
		end
		if damage then
			self:damageLine(y)
		end
	end
end

-------------------------------------------------------------------------------
--	wordNext()
-------------------------------------------------------------------------------

local function isalphanum(v)
	v = v or 0
	return (v >= 97 and v <= 122) or (v >= 65 and v <= 90) 
		or (v >= 48 and v <= 57)
end

local function iswhite(c)
	return c == " " or c == "\t"
end

function TextEdit:getNextWordDelta()
	local cx, cy = self:getCursor()
	local line = self:getLineText(cy)
	local v0 = isalphanum(line:getval(cx))
	local mx = 1
	for i = cx + 1, line:len() do
		local v = isalphanum(line:getval(i))
		if v ~= v0 then
			if v then
				break
			end
			v0 = v
		end
		mx = mx + 1
	end
	return mx
end

function TextEdit:wordNext()
	local mx = self:getNextWordDelta()
	self:moveCursor(mx, 0)
end

function TextEdit:deleteNextWord()
	local mx0, my0 = self:getCursor()
	local dx = self:getNextWordDelta()
	if dx > 0 then
		self:eraseBlock(mx0, my0, mx0 + dx, my0)
	end
end

-------------------------------------------------------------------------------
--	wordPrev()
-------------------------------------------------------------------------------

function TextEdit:getPrevWordDelta()
	local cx, cy = self:getCursor()
	local line = self:getLineText(cy)
	local v0 = isalphanum(line:getval(cx - 1))
	local mx = 0
	for i = cx, 1, -1 do
		local v = isalphanum(line:getval(i - 1))
		if v ~= v0 then
			if not v then
				break
			end
			v0 = v
		end
		mx = mx - 1
	end
	return mx
end

function TextEdit:wordPrev()
	local mx = self:getPrevWordDelta()
	self:moveCursor(mx, 0)
end

function TextEdit:deletePrevWord()
	local mx0, my0 = self:getCursor()
	local dx = self:getPrevWordDelta()
	if dx < 0 then
		self:eraseBlock(mx0 + dx, my0, mx0, my0)
		self:moveCursor(dx, 0)
	end
end

function TextEdit:findCurrentWord()
	local cx, cy = self:getCursor()
	local line = self:getLineText(cy)
	if isalphanum(line:getval(cx)) then
		local cx0 = cx
		while cx0 > 1 and isalphanum(line:getval(cx0 - 1)) do
			cx0 = cx0 - 1
		end
		local cx1 = cx
		local llen = self:getLineLength(cy)
		while cx1 < llen and isalphanum(line:getval(cx1 + 1)) do
			cx1 = cx1 + 1
		end
		return cx0, cx1 + 1
	end
end

-------------------------------------------------------------------------------
--	updateWindow()
-------------------------------------------------------------------------------

function TextEdit:updateWindow()
	if self.SuspendUpdateNest == 0 then
		self.Window:update()
		self.UpdateSuspended = false
	else
		self.UpdateSuspended = true
	end
end

function TextEdit:suspendWindowUpdate()
	self.SuspendUpdateNest = self.SuspendUpdateNest + 1
end

function TextEdit:releaseWindowUpdate()
	self.SuspendUpdateNest = self.SuspendUpdateNest - 1
	assert(self.SuspendUpdateNest >= 0)
	if self.SuspendUpdateNest == 0 and self.UpdateSuspended then
		self:updateWindow()
	end
end


function TextEdit:setCursorMode(mode)
	assert(mode == "still" or mode == "active" or mode == "hidden")
	local ocm = self.CursorMode
	self.CursorMode = mode
	return ocm
end


function TextEdit:indentBlock(num, inschar)
	local _, my0, mx1, my1 = self:getMark()
	if my1 then
		if mx1 == 1 then
			my1 = my1 - 1
		end
	else
		_, my0 = self:getCursor()
		my1 = my0
	end
	self:suspendWindowUpdate()
	num = num or 1
	if num > 0 then
		local ins = (inschar or "\t"):rep(num)
		for lnr = my0, my1 do
			local text = self:getLineText(lnr)
			text:insert("\t", 1)
			self:changeLine(lnr)
			self:damageLine(lnr)
		end
	elseif num < 0 then
		for lnr = my0, my1 do
			local text = self:getLineText(lnr)
			if iswhite(text:sub(1, 1)) then
				text:erase(1, 1)
				self:changeLine(lnr)
				self:damageLine(lnr)
			end
		end
	end
	self:releaseWindowUpdate()
	self:setValue("Changed", true)
	return true
end


function TextEdit:getSelection(which)
	which = which or 1 -- 1 = clipboard (default), 2 = selection
	local s, c = self.Window.Drawable:getAttrs("sc")
	if (which == 1 and c) or (which == 2 and s) then
		db.info("denied - would request clipboard/selection from self")
		return 
	end
	local clip = self.Window.Drawable:getSelection(which)
	if clip then
		if self.MultiLine then
			clip = self.breakText(clip)
		elseif type(clip) == "string" then
			clip = { clip:gsub("[\n\r]", " ") }
		end
	end
	if not clip and which ~= 2 then
		clip = self.Application:obtainClipboard()
	end
	return clip
end

-------------------------------------------------------------------------------
--	onDisable: overrides
-------------------------------------------------------------------------------

function TextEdit:onDisable()
	self:onFocus()
end

return TextEdit
