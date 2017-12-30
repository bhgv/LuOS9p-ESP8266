-------------------------------------------------------------------------------
--
--	tek.ui.class.window
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
--		Window ${subclasses(Window)}
--
--		This class implements a [[#tek.ui.class.group : Group]] which
--		fills up a window on the [[#tek.ui.class.display : Display]].
--
--	ATTRIBUTES::
--		- {{Center [IG]}} (boolean)
--			Instructs the Window to open centered.
--		- {{DblClickJitter [IG]}} (number)
--			Maximum sum of squared pixel deltas (dx² + dy²) between
--			mouse positions to be tolerated for a double click.
--			The default is 70. Large touchscreens require a much larger value.
--		- {{DblClickTimeout [IG]}} (number)
--			Maximum number of microseconds between events to be recognized as
--			a double click. Default: 32000. Use a larger value for
--			touchscreens.
--		- {{FullScreen [IG]}} (boolean)
--			Instructs the Window to open borderless and in fullscreen mode.
--		- {{HideOnEscape [IG]}} (boolean)
--			Instructs the window to invoke the Window:onHide() method
--			when the Escape key is pressed. Default: '''false'''
--		- {{Left [IG]}} (number)
--			The window's left offset on the display, in pixels
--		- {{Modal [IG]}} (boolean)
--			Instructs all other windows to reject input while this window is
--			open.
--		- {{MouseX [G]}} (number)
--		- {{MouseY [G]}} (number)
--			The current window coordinates of the pointing device.
--		- {{RootWindow [IG]}} (boolean)
--			Hint that the window can be used as the background or main window
--			that determines the size of the screen or display resolution.
--		- {{Status [ISG]}} (string)
--			Status of the Window, which can be:
--				- {{"initializing"}} - The window is initializing
--				- {{"hide"}} - The window is hidden or about to hide;
--				if you initialize the Window with this value, it will be
--				created in hidden state.
--				- {{"opening"}} - The window is about to open.
--				- {{"show"}} - The window is shown.
--				- {{"closing"}} - The Window is about to hide.
--			Changing this attribute invokes the Window:onChangeStatus()
--			method.
--		- {{Title [IG]}} (string)
--			The window's title
--		- {{Top [IG]}} (number)
--			The window's top offset on the display, in pixels
--
--	IMPLEMENTS::
--		- Window:addInputHandler() - Adds an input handler to the window
--		- Window:addInterval() - Adds an interval timer to the window
--		- Window:checkDblClickTime() - Checks a time for a doubleclick event
--		- Window:clickElement() - Simulates a click on an element
--		- Window:onChangeStatus() - Handler for {{Status}}
--		- Window:onHide() - Handler for when the window is about to be closed
--		- Window:postMsg() - Post an user message in the Window's message queue
--		- Window:remInputHandler() - Removes an input handler from the window
--		- Window:remInterval() - Removes an interval timer from the window
--		- Window:setActiveElement() - Sets the window's active element
--		- Window:setDblClickElement() - Sets the window's doubleclick element
--		- Window:setFocusElement() - Sets the window's focused element
--		- Window:setHiliteElement() - Sets the window's hover element
--		- Window:setMovingElement() - Sets the window's moving element
--
--	OVERRIDES::
--		- Object.addClassNotifications()
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
local Display = ui.require("display", 25)
local Widget = ui.require("widget", 26)
local Group = ui.require("group", 31)
local Region = ui.loadLibrary("region", 10)

local assert = assert
local band = ui.band
local floor = math.floor
local insert = table.insert
local intersect = Region.intersect
local max = math.max
local min = math.min
local pairs = pairs
local remove = table.remove
local setmetatable = setmetatable
local sort = table.sort
local tonumber = tonumber
local type = type
local unpack = unpack or table.unpack

local Window = Group.module("tek.ui.class.window", "tek.ui.class.group")
Window._VERSION = "Window 46.5"

-------------------------------------------------------------------------------
--	constants & class data:
-------------------------------------------------------------------------------

local HUGE = ui.HUGE

local MSG_MOUSEMOVE = ui.MSG_MOUSEMOVE
local MSG_MOUSEBUTTON = ui.MSG_MOUSEBUTTON

local MSGTYPES = { ui.MSG_CLOSE, ui.MSG_FOCUS, ui.MSG_NEWSIZE, ui.MSG_REFRESH,
	ui.MSG_MOUSEOVER, ui.MSG_KEYDOWN, MSG_MOUSEMOVE, MSG_MOUSEBUTTON,
	ui.MSG_INTERVAL, ui.MSG_KEYUP, ui.MSG_REQSELECTION }

local FL_REDRAW = ui.FL_REDRAW
local FL_SHOW = ui.FL_SHOW
local FL_LAYOUT = ui.FL_LAYOUT
local FL_UPDATE = ui.FL_UPDATE
local FL_ACTIVATERMB = ui.FL_ACTIVATERMB
local FL_ISWINDOW = ui.FL_ISWINDOW

-------------------------------------------------------------------------------
--	addClassNotifications: overrides
-------------------------------------------------------------------------------

function Window.addClassNotifications(proto)
	Window.addNotify(proto, "Status", ui.NOTIFY_ALWAYS,
		{ ui.NOTIFY_SELF, "onChangeStatus"})
	return Group.addClassNotifications(proto)
end

Window.ClassNotifications = 
	Window.addClassNotifications { Notifications = { } }

-------------------------------------------------------------------------------
--	DragButton
-------------------------------------------------------------------------------

local function getpseudoclass(self)
	if self.Window.WindowFocus then
		return ":windowfocus"
	end
	return self:getSuper().getPseudoClass(self)
end

local DragButton = ui.Button:newClass { _NAME = "_window-decoration-dragbutton" }

function DragButton.new(class, self)
	self = self or { }
	self.Draggable = self.Draggable == nil or self.Draggable
	self.StartX = false
	self.StartY = false
	self.NoFocus = true
	self.Style = "border-width: 0; margin: 0"
	return ui.Button.new(class, self)
end

function DragButton:startMove(x, y)
	local wx, wy = self.Window.Drawable:getAttrs("xy")
	self.StartX = x - wx
	self.StartY = y - wy
	return true
end

function DragButton:doMove(x, y)
	local wx = x - self.StartX
	local wy = y - self.StartY
	self.Window.Drawable:setAttrs { Left = wx, Top = wy }
end

function DragButton:passMsg(msg)
	if self.Draggable then
		local sx, sy = self.Application.MouseX, self.Application.MouseY
		if msg[2] == MSG_MOUSEMOVE then
			if self.Window.MovingElement == self then
				self:doMove(sx, sy)
				-- do not pass message to other elements while dragging:
				return false
			end
		elseif msg[2] == MSG_MOUSEBUTTON then
			if msg[3] == 1 then -- leftdown:
				if self:getByXY(msg[4], msg[5]) then
					-- raise
					self.Window.Drawable:setAttrs { WindowHints = "t" }
				end
				if self.Window.HoverElement == self and not self.Disabled then
					if self:startMove(sx, sy) then
						self.Window:setMovingElement(self)
					end
				end
			elseif msg[3] == 2 then -- leftup:
				if self.Window.MovingElement == self then
					self.Window:setMovingElement()
				end
			end
		end
	end
	return ui.Button.passMsg(self, msg)
end

DragButton.getPseudoClass = getpseudoclass

-------------------------------------------------------------------------------
--	SizeButton
-------------------------------------------------------------------------------

local SizeButton = 
	ui.ImageWidget:newClass { _NAME = "_window-decoration-sizebutton" }

function SizeButton.new(class, self)
	self = self or { }
	self.StartX = false
	self.StartY = false
	self.WinWidth = false
	self.WinHeight = false
	self.NoFocus = true
	self.Width = "auto"
	self.Height = "fill"
	self.Mode = "button"
	self.Image = ui.getStockImage("arrowright")
	self.Style = "border-width: 0; margin: 0"
	return ui.ImageWidget.new(class, self)
end

function SizeButton:startMove(x, y)
	self.StartX = x
	self.StartY = y
	self.WinWidth, self.WinHeight = self.Window.Drawable:getAttrs("wh")
	return true
end

function SizeButton:doMove(x, y)
	local ww = x - self.StartX + self.WinWidth
	local wh = y - self.StartY + self.WinHeight
	self.Window.Drawable:setAttrs { Width = ww, Height = wh }
end

function SizeButton:passMsg(msg)
	local sx, sy = self.Application.MouseX, self.Application.MouseY
	if msg[2] == ui.MSG_MOUSEBUTTON then
		if msg[3] == 1 then -- leftdown:
			if self.Window.HoverElement == self and not self.Disabled then
				if self:startMove(sx, sy) then
					self.Window:setMovingElement(self)
				end
			end
		elseif msg[3] == 2 then -- leftup:
 			if self.Window.MovingElement == self then
				self.Window:setMovingElement()
			end
		end
	elseif msg[2] == ui.MSG_MOUSEMOVE then
		if self.Window.MovingElement == self then
			self:doMove(sx, sy)
			-- do not pass message to other elements while dragging:
			return false
		end
	end
	return ui.ImageWidget.passMsg(self, msg)
end

SizeButton.getPseudoClass = getpseudoclass


local SizeBottom = 
	ui.Widget:newClass { _NAME = "_window-decoration-sizebottom" }

function SizeBottom.new(class, self)
	self = self or { }
	self.Width = "free"
	self.Height = "fill" 
	self.Mode = "button" 
	self.NoFocus = true
	self.Style = "border-width: 0; margin: 0"
	return ui.Widget.new(class, self)
end

SizeBottom.getPseudoClass = getpseudoclass

-------------------------------------------------------------------------------
--	CloseButton
-------------------------------------------------------------------------------

local CloseButton =
	ui.ImageWidget:newClass { _NAME = "_window-decoration-closebutton" }

function CloseButton.new(class, self)
	self = self or { }
	self.StartX = false
	self.StartY = false
	self.WinWidth = false
	self.WinHeight = false
	self.NoFocus = true
	self.Height = "fill"
	self.MaxWidth = 20
	self.Mode = "button"
	self.Image = ui.getStockImage("radiobutton", 3)
	self.Style = "border-width: 0; margin: 0"
	return ui.ImageWidget.new(class, self)
end

CloseButton.getPseudoClass = getpseudoclass

-------------------------------------------------------------------------------
--	init: overrides
-------------------------------------------------------------------------------

function Window.new(class, self)
	self = self or { }
	self.ActiveElement = false
	-- Item in this window in which an active popup is anchored:
	self.ActivePopup = false
	self.Blits = { }
	self.BlitObjects = { }
	self.Borderless = self.Borderless or false
	self.CanvasStack = { }
	self.Center = self.Center or false
	self.DblClickElement = false
	self.DblClickCheckInfo = { } -- check_element, sec, usec, mousex, mousey
	self.DblClickJitter = self.DblClickJitter or ui.DBLCLICKJITTER
	self.DblClickTimeout = self.DblClickTimeout or ui.DBLCLICKTIME
	self.Drawable = false
	self.EventMask = ui.MSG_ALL
	self.Flags = ui.bor(self.Flags or 0, FL_ISWINDOW)
	self.FocusElement = false
	
	self.PopupWindow = self.PopupWindow or false
	if self.PopupWindow then
		self.CloseButton = false
		self.DragButton = false
		self.SizeButton = false
		self.RootWindow = false
	else
		self.CloseButton = self.CloseButton == nil or self.CloseButton
		self.DragButton = self.DragButton == nil or self.DragButton
		self.SizeButton = self.SizeButton or false
		self.RootWindow = self.RootWindow or false
	end

	self.FullScreen = self.FullScreen or 
		(self.RootWindow and ui.FullScreen == "true")
	
	self.HideOnEscape = self.HideOnEscape or false
	self.HiliteElement = false
	-- Active hold tick counter - number of ticks left to next hold event:
	self.HoldTickActive = 0
	-- Hold tick counter reinitialization (see below):
	self.HoldTickActiveInit = 0
	-- Number of ticks for first hold tick counter initialization:
	self.HoldTickFirst = self.HoldTickFirst or 22
	-- Number of ticks for hold counter repeat initialization:
	self.HoldTickRepeat = self.HoldTickRepeat or 7
	self.HoverElement = false
	self.InputHandlers =
	{
		[ui.MSG_CLOSE] = { },
		[ui.MSG_FOCUS] = { },
		[ui.MSG_NEWSIZE] = { },
		[ui.MSG_REFRESH] = { },
		[ui.MSG_MOUSEOVER] = { },
		[ui.MSG_KEYDOWN] = { },
		[ui.MSG_MOUSEMOVE] = { },
		[ui.MSG_MOUSEBUTTON] = { },
		[ui.MSG_INTERVAL] = { },
		[ui.MSG_KEYUP] = { },
		[ui.MSG_REQSELECTION] = { },
	}
	self.IntervalMsg = false
	self.IntervalMsgStore =
	{
		[2] = ui.MSG_INTERVAL,
		[3] = 0,
		[4] = 0,
		[5] = 0,
		[6] = 0,
	}
	self.IntervalCount = 0
	self.KeyShortcuts = { }
	self.Left = self.Left or false
	self.Modal = self.Modal or false
	self.MouseX = false
	self.MouseY = false
	self.MouseMoveMsg = false
	self.MouseMoveMsgStore =
	{
		[2] = ui.MSG_MOUSEMOVE,
		[3] = 0,
		[4] = 0,
		[5] = 0,
		[6] = 0,
	}
	self.MovingElement = false
	self.MsgQueue = { }
	self.NewSizeMsg = false
	self.NewSizeMsgStore =
	{
		[2] = ui.MSG_NEWSIZE,
		[3] = 0,
		[4] = 0,
		[5] = 0,
		[6] = 0,
	}
	self.PenTable = false
	-- Root window in a cascade of popup windows:
	self.PopupRootWindow = self.PopupRootWindow or false
	self.RefreshMsg = false
	self.RefreshMsgStore =
	{
		[2] = ui.MSG_REFRESH,
		[3] = 0,
		[4] = 0,
		[5] = 0,
		[6] = 0,
	}
	self.Relayouts = { }
	self.Status = self.Status or "initializing"
	self.Title = self.Title or false
	self.Top = self.Top or false
	self.WindowFocus = false
	self.WindowMinMax = { }
	self.WinRect = false
	
	if not (self.CloseButton or self.DragButton or self.SizeButton)
		or self.RootWindow
		or self.PopupWindow or self.Borderless or ui.Mode ~= "workbench" 
		or Display.getDisplayAttrs("M") then
		return Group.new(class, self)
	end

	-- add window decorations.
	
	local children = self.Children or { }
	self.Children = false
	
	local sizebutton = self.SizeButton
	local dragbutton = self.DragButton
	local closebutton = self.CloseButton
	
	local wgroup = Group:new 
	{
		Orientation = self.Orientation or "horizontal",
		Style = self.Style,
		Columns = self.Columns,
		Legend = self.Legend,
		Class = "window-decoration-group",
		Children = children 
	}
	
	self.Orientation = "vertical"
	self.Columns = nil
	self.Style = nil
	self.Legend = nil
	self.Class = "window-decoration"
	self.Children = { }
	
	self = Group.new(class, self)
		
	local topgroup = Group:new { Width = "fill" }
	
	if dragbutton then
		dragbutton = DragButton:new { Width = "fill", Height = "fill",
			Text = self.Title, Draggable = dragbutton }
		dragbutton:addStyleClass("window-decoration-button")
		topgroup:addMember(dragbutton)
	end
	
	if closebutton then
		closebutton = CloseButton:new {
			onClick = function(self)
				self.Window:hide()
			end
		}
		closebutton:addStyleClass("window-decoration-button")
		topgroup:addMember(closebutton)
	end

	self:addMember(topgroup)
	self:addMember(wgroup)

	local sizebottom
	
	if sizebutton then
		sizebutton = SizeButton:new()
		sizebutton:addStyleClass("window-decoration-button")
		sizebottom = SizeBottom:new { }
		sizebottom:addStyleClass("window-decoration-button")
		self:addMember(Group:new
		{ 
			Height = "auto",
			Children =
			{
				sizebottom,
				sizebutton
			}
		})
	end
	
	self:addNotify("WindowFocus", ui.NOTIFY_ALWAYS,
		{ ui.NOTIFY_SELF, ui.NOTIFY_FUNCTION, function()
		if dragbutton then
			dragbutton:setState()
		end
		if closebutton then
			closebutton:setState()
		end
		if sizebutton then
			sizebutton:setState()
			if sizebottom then
				sizebottom:setState()
			end
		end
	end })

	return self
	
end

-------------------------------------------------------------------------------
--	setup: overrides
-------------------------------------------------------------------------------

function Window:setup(app)
	Group.setup(self, app, self)
	self:addInputHandler(0x171f, self, self.handleInput)
end

-------------------------------------------------------------------------------
--	cleanup: overrides
-------------------------------------------------------------------------------

function Window:cleanup()
	self:remInputHandler(0x171f, self, self.handleInput)
	Group.cleanup(self)
	assert(self.IntervalCount == 0)
end

-------------------------------------------------------------------------------
--	isSolidPen: Check if a pen is a solid color
-------------------------------------------------------------------------------

function Window:isSolidPen(pen)
	return not pen or self.Drawable:getPaintInfo(self.PenTable[pen]) == 1
end

-------------------------------------------------------------------------------
--	show: overrides
-------------------------------------------------------------------------------

function Window:show()
	self.Status = "show"
	if not self.Drawable then
		local m1, m2, m3, m4, x, y, w, h = self:getWindowDimensions()
		local display = self.Application.Display
		local drawable
		self.PenTable = { }
		drawable = Display.openDrawable {
			UserData = self,
			Title = self.Title or self.Application.ProgramName,
			Width = w,
			Height = h,
			Left = x,
			Top = y,
			MinWidth = m1,
			MinHeight = m2,
			MaxWidth = m3,
			MaxHeight = m4,
			Center = self.Center,
			FullScreen = self.FullScreen,
			Borderless = self.Borderless,
			PopupWindow = self.PopupWindow,
			EventMask = self.EventMask,
			BlankCursor = ui.NoCursor,
			MsgFileNo = ui.MsgFileNo, -- used when opening the 1st window
			ExtraArgs = ui.ExtraArgs,
			Pens = setmetatable(self.PenTable, {
				__index = function(tab, col)
					local key = col
					local res
					if type(col) == "string" then
						-- match url(...) etc.
						if col:match("^(%a+)(%b())") then
							res = display.getPaint(col, display)
							if not res then
								col = "background"
							end
						end
						if not res then
							local a, r, g, b = display:colorToRGB(col)
							if not r then
								a, r, g, b = display:colorToRGB("background")
							end
							res = drawable:allocPen(a, r, g, b)
						end
					else
						res = col
					end
					tab[key] = res
					return res
				end
			})
		}
		self.Drawable = drawable
		self.Application:openWindow(self)
		Group.show(self, d)
		self:layout()
	end
end

-------------------------------------------------------------------------------
--	hide: overrides
-------------------------------------------------------------------------------

function Window:hide()
	self:setValue("Status", "hide")
	if self.Drawable then
		self:remInputHandler(ui.MSG_INTERVAL, self, self.handleHold)
		Group.hide(self)
		local d = self.Drawable
		self.WinRect = { d:getAttrs("xywh") }
		d:close()
		self.Drawable = false
		self.ActiveElement = false
		self.Application:closeWindow(self)
		self.PenTable = false
	end
end

-------------------------------------------------------------------------------
--	addInputHandler(msgtype, object, function): Adds an {{object}} and
--	a {{function}} to the window's chain of handlers for input of the
--	specified type. Multiple input types can be handled by one handler by
--	logically or'ing message types. Input handlers are invoked as follows:
--			message = function(object, message)
--	The handler is expected to return the message, which will in turn pass
--	it on to the next handler in the window's chain.
-------------------------------------------------------------------------------

function Window:addInputHandler(msgtype, object, func)
	local flags = msgtype
	local hnd = { object, func }
	for i = 1, #MSGTYPES do
		local mask = MSGTYPES[i]
		local ih = self.InputHandlers[mask]
		if ih and band(flags, mask) == mask then
			insert(ih, 1, hnd)
			if mask == ui.MSG_INTERVAL and #ih == 1 then
				self:addInterval()
			end
		end
	end
end

-------------------------------------------------------------------------------
--	remInputHandler(msgtype, object, func): Removes an input handler
--	that was previously registered with the window using
--	Window:addInputHandler().
-------------------------------------------------------------------------------

function Window:remInputHandler(msgtype, object, func)
	local flags = msgtype
	for i = 1, #MSGTYPES do
		local mask = MSGTYPES[i]
		local ih = self.InputHandlers[mask]
		if ih and band(flags, mask) == mask then
			for i = 1, #ih do
				local h = ih[i]
				if h[1] == object and h[2] == func then
					remove(ih, i)
					if mask == ui.MSG_INTERVAL and #ih == 0 then
						self:remInterval()
					end
					break
				end
			end
		end
	end
end

-------------------------------------------------------------------------------
--	addInterval(): Adds an interval timer to the window, which will
--	furtheron generate MSG_INTERVAL messages 50 times per second. These
--	messages cause a considerable load to the application, therefore each call
--	to this function should be paired with a matching call to
--	Window:remInterval(), which will cause the interval timer to stop when
--	no clients are needing it anymore.
-------------------------------------------------------------------------------

function Window:addInterval()
	self.IntervalCount = self.IntervalCount + 1
	if self.IntervalCount == 1 then
		self.EventMask = ui.MSG_ALL + ui.MSG_INTERVAL
		if self:checkFlags(FL_SHOW) then
			self.Drawable:setInput(ui.MSG_INTERVAL)
		else
			db.info("adding interval on closed window")
		end
	end
end

-------------------------------------------------------------------------------
--	remInterval(): Decreases the use counter for interval messages and
--	stops sending interval messages to the window when called by the last
--	client that has previously requested an interval timer using
--	Window:addInterval().
-------------------------------------------------------------------------------

function Window:remInterval()
	self.IntervalCount = self.IntervalCount - 1
	assert(self.IntervalCount >= 0)
	if self.IntervalCount == 0 then
		self.EventMask = ui.MSG_ALL
		if self:checkFlags(FL_SHOW) then
			self.Drawable:clearInput(ui.MSG_INTERVAL)
		end
	end
end

-------------------------------------------------------------------------------
--	onChangeStatus(): This method is invoked when the Window's {{Status}} has
--	changed.
-------------------------------------------------------------------------------

function Window:onChangeStatus()
	local status = self.Status
	if status == "show" then
		self:show()
	elseif status == "hide" then
		self:hide()
	end
end

-------------------------------------------------------------------------------
--	askMinMax: overrides
-------------------------------------------------------------------------------

function Window:askMinMax()

	local mw, mh = self.MinWidth, self.MinHeight
	self.MinWidth, self.MinHeight = 0, 0
	
	local w = self:getAttr("Width")
	local h = self:getAttr("Height")
	local minw = self:getAttr("MinWidth")
	local minh = self:getAttr("MinHeight")
	local maxw = self:getAttr("MaxWidth")
	local maxh = self:getAttr("MaxHeight")
	
	local m1, m2, m3, m4 = Group.askMinMax(self, 0, 0, maxw, maxh)
	self.MinWidth, self.MinHeight = mw, mh
	
	mw = minw
	mh = minh
	
	m1, m2 = max(mw, m1), max(mh, m2)
	local x, y, w, h = self.Left, self.Top, self.Width, self.Height
	if w == "fill" then
		w = m1
	end
	if h == "fill" then
		h = m2
	end
	w = type(w) == "number" and max(w, m1)
	h = type(h) == "number" and max(h, m2)
	m3 = (m3 and m3 > 0 and m3 < HUGE) and m3
	m4 = self.FullScreen and ui.HUGE or (m4 and m4 > 0 and m4 < HUGE) and m4
	w = w or maxw == 0 and m1 or w
	h = h or maxh == 0 and m2 or h
	
	local wr = self.WinRect
	if wr then
		return m1, m2, m3, m4, wr[1] or x, wr[2] or y, wr[3] or w, wr[4] or h
	end
	return m1, m2, m3, m4, x, y, w, h
end

-------------------------------------------------------------------------------
--	m1, m2, m3, m4, x, y, w, h = getWindowDimensions(update)
-------------------------------------------------------------------------------

function Window:getWindowDimensions(update)
	local m1, m2, m3, m4, x, y, w, h = self:askMinMax()
	local wm = self.WindowMinMax
	if m1 ~= wm[1] or m2 ~= wm[2] or m3 ~= wm[3] or m4 ~= wm[4] then
		wm[1], wm[2], wm[3], wm[4] = m1, m2, m3, m4
		if update then
			local drawable = self.Drawable
			local w, h = drawable:getAttrs()
			if (m1 and w < m1) or (m2 and h < m2) then -- TODO: m1, m2?
				-- window needs to grow; mark new region as damaged:
				w = max(w, m1 or w)
				h = max(h, m2 or h)
				self:damage(0, 0, w - 1, h - 1)
			end
			drawable:setAttrs { MinWidth = m1, MinHeight = m2,
				MaxWidth = m3, MaxHeight = m4 }
		end
	end
	return m1, m2, m3, m4, x, y, w, h
end

-------------------------------------------------------------------------------
--	getMsg:
-------------------------------------------------------------------------------

function Window:getMsg(msg)
	local m = self.Status == "show" and remove(self.MsgQueue)
	if m then
		msg[-1], msg[0], msg[1], msg[2], msg[3], msg[4], msg[5],
			msg[6], msg[7], msg[8], msg[9], msg[10] = unpack(m, -1, 10)
		return true
	end
end

-------------------------------------------------------------------------------
--	postMsg(msg): This function adds an user message to the Window's message
--	queue.
-------------------------------------------------------------------------------

function Window:postMsg(msg)
	if self.Status == "show" then
		msg[-1] = self
		insert(self.MsgQueue, msg)
	end
end

-------------------------------------------------------------------------------
--	passMsg: overrides
-------------------------------------------------------------------------------

function Window:passMsg(msg)
	local handlers = { unpack(self.InputHandlers[msg[2]]) }
	for i = 1, #handlers do
		local hnd = handlers[i]
		msg = hnd[2](hnd[1], msg)
		if not msg then
			return false
		end
	end
	return msg
end

-------------------------------------------------------------------------------
--	finishPopup:
-------------------------------------------------------------------------------

function Window:finishPopup()
	local w = self.PopupRootWindow or self
	if w.ActivePopup then
		w.ActivePopup:setValue("Selected", false)
	end
end

-------------------------------------------------------------------------------
--	handleInput:
-------------------------------------------------------------------------------

local MsgHandlers =
{
	[ui.MSG_CLOSE] = function(self, msg)
		self:onHide()
		return msg
	end,
	[ui.MSG_FOCUS] = function(self, msg)
		local has_focus = msg[3] == 1 or 
			(self:checkFlags(FL_ACTIVATERMB) and msg[3] == 4)
		self:setValue("WindowFocus", has_focus)
		self:setHiliteElement()
		self:setActiveElement()
		self.Application:setFocusWindow(self, has_focus)
		return msg
	end,
	[ui.MSG_NEWSIZE] = function(self, msg)
		self:addLayout(self, 0, false)
		return false -- do not pass this msg to group
	end,
	[ui.MSG_REFRESH] = function(self, msg)
		self:damage(msg[7], msg[8], msg[9], msg[10])
		return false -- do not pass this msg to group
	end,
	[ui.MSG_MOUSEOVER] = function(self, msg)
		self:setMsgMouseXY(msg)
		self:setHiliteElement()
		return msg
	end,
	[ui.MSG_MOUSEBUTTON] = function(self, msg)
		self:setMsgMouseXY(msg)
		local armb = self:checkFlags(FL_ACTIVATERMB)
		if msg[3] == 1 or (armb and msg[3] == 4) then -- leftdown:
			-- send "Pressed" to window:
			self:setValue("Pressed", true, true)
		elseif msg[3] == 2 or (armb and msg[3] == 8) then -- leftup:
			local ae = self.ActiveElement
			local he = self.HoverElement
			-- release Hold state:
			if ae and ae.Active and ae.Hold and self.HoldTickActive > 0 then
				ae:setValue("Hold", false)
			end   
			if not ae and he and he:checkFocus() then
				-- support releasing a button over a popup item that wasn't
				-- activated before, i.e. button was pressed all the time:
				he:setValue("Active", false, true)
			end
			self:setMovingElement()
			self:setActiveElement()
			-- release window:
			self:setValue("Pressed", false, true)
		end
		return msg
	end,
	[ui.MSG_KEYDOWN] = function(self, msg)
		self:setMsgMouseXY(msg)
		if not self.Application then
			db.warn("window already collapsed")
			return msg
		end
		-- pass message to active popup element:
		if self.ActivePopup then
			-- return
			db.info("propagating keydown to active popup...")
			msg = self.ActivePopup.PopupWindow:passMsg(msg)
			if not msg then
				return false
			end
		end
		local fe = self.FocusElement
		local key = msg[3]
		local qual = msg[6]
		local retrig = key ~= 0 and self.Application:setLastKey(key)

		-- activate window:
		self:setValue("Active", true)
		if qual == 0 and key == 27 and not retrig then
			local pr = self.PopupRootWindow
			if pr then
				pr.ActivePopup:endPopup()
			elseif self.HideOnEscape then
				self:onHide()
			end
			return
		elseif key == 9 then -- tab key:
			if qual == 0 then -- no qualifier:
				self:setFocusElement(self:getNextElement(fe))
			elseif qual >= 1 and qual <= 3 then -- shift qualifier:
				self:setFocusElement(self:getNextElement(fe, true))
			end
			return
		elseif qual == 0 and (key == 61459 or key == 61457) then -- down, right
			local nfe = self:getNextElement(fe)
			if nfe and nfe:checkFlags(ui.FL_CURSORFOCUS) then
				self:setFocusElement(nfe)
				return
			end
		elseif qual == 0 and (key == 61458 or key == 61456) then -- up, left
			local nfe = self:getNextElement(fe, true)
			if nfe and nfe:checkFlags(ui.FL_CURSORFOCUS) then
				self:setFocusElement(nfe)
				return
			end
		elseif qual == 0 and (key == 13 or key == 32) and not retrig then
			if fe and not fe.Active then
				self:setHiliteElement(fe)
				self:setActiveElement(fe)
			end
			return
		end
		if not retrig then
			-- serve elements with keyboard shortcuts:
			local s = self:getShortcutElements(msg[7], msg[6])
			if s then
				for i = 1, #s do
					local e = s[i]
					if not e.Disabled and e.KeyCode then
						self:clickElement(e)
					end
				end
			end
		end
		return msg
	end,
	[ui.MSG_KEYUP] = function(self, msg)
		local key = msg[3]
		self.Application:setLastKey() -- release
		self:setMsgMouseXY(msg)
		-- pass message to active popup element:
		if self.ActivePopup then
			-- return
			db.info("propagating keyup to active popup...")
			msg = self.ActivePopup.PopupWindow:passMsg(msg)
			if not msg then
				return false
			end
		end
		if key == 13 or key == 32 then
			self:setActiveElement()
			self:setHiliteElement(self.HoverElement)
		end
		return msg
	end,
	[ui.MSG_MOUSEMOVE] = function(self, msg)
		self:setMsgMouseXY(msg)
		return msg
	end,
}

function Window:handleInput(msg)
	msg = MsgHandlers[msg[2]](self, msg)
	if msg then
		return Group.passMsg(self, msg)
	end
	return false
end

function Window:setMsgMouseXY(msg)
	local mx, my = msg[4], msg[5]
	self.MouseX, self.MouseY = mx, my
	self.HoverElement = self:getByXY(mx, my)
	return mx, my
end

-------------------------------------------------------------------------------
--	addLayout: marks an element for relayout during the next update
--	cycle. See Area:rethinkLayout() for details on the damage argument.
-------------------------------------------------------------------------------

function Window:addLayout(element, damage, askminmax)
	askminmax = askminmax or false
	local r = self.Relayouts
	local e = r[element]
	if not e then
		e = { element, damage, askminmax }
		insert(r, e)
		r[element] = e
		return
	end
	if damage > e[2] then
		e[2] = damage
	end
	if not e[3] then
		e[3] = askminmax
	end
end

-------------------------------------------------------------------------------
--	draw:
-------------------------------------------------------------------------------

local function sortcopies(a, b) return a[1] > b[1] end

local function insertblitx(t, r1, r2, r3, r4, dir, e)
	insert(t, { r1 * dir, r1, r2, r3, r4, e })
end

local function insertblity(t, r1, r2, r3, r4, dir, e)
	insert(t, { r2 * dir, r1, r2, r3, r4, e })
end

function Window:draw()
	-- handle copies:
	local ca = self.Blits
	local t = { }
	for key, e in pairs(ca) do
		local dx, dy, region = e[1], e[2], e[3]
		if dy == 0 then
			region:forEach(insertblitx, t, dx > 0 and 1 or -1, e)
		else
			region:forEach(insertblity, t, dy > 0 and 1 or -1, e)
		end
		ca[key] = nil
	end
	sort(t, sortcopies)
	local d = self.Drawable
	for i = 1, #t do
		local r = t[i]
		local e = r[6]
		local t = { }
		local dx, dy = e[1], e[2]
		if e[4] then
			d:pushClipRect(e[4], e[5], e[6], e[7])
		end
		d:blitRect(r[2], r[3], r[4], r[5], r[2] + dx, r[3] + dy, t)
		if e[4] then
			d:popClipRect()
		end
		for i = 1, #t, 4 do
			self:damage(t[i], t[i + 1], t[i + 2], t[i + 3])
		end
	end
	self.BlitObjects = { }
	Group.draw(self)
	d:flush()
end

-------------------------------------------------------------------------------
--	update:
-------------------------------------------------------------------------------

function Window:update()
	
	if self.Status == "show" then

		if #self.Relayouts > 0 then

			local updateminmax
			repeat

				local rl = self.Relayouts
				local nrl = { }
				self.Relayouts = nrl

				for i = #rl, 1, -1 do
					local e = rl[i]
					local damage = e[2]
					if e[3] then
						updateminmax = true
					end
					if damage == 0 then
						remove(rl, i)
					else
						-- remember old coordinates:
						e[3], e[4], e[5], e[6] = e[1]:getRect()
					end
				end

				if updateminmax then
					self:getWindowDimensions(true)
				end

				self:layout()

				while #rl > 0 do
					local r = remove(rl)
					local e = r[1]
					local damage = r[2]
					local r1, r2, r3, r4 = e:getRect()
					local changed = r1 and
						(r[3] ~= r1 or r[4] ~= r2 or r[5] ~= r3 or r[6] ~= r4)
					if changed then
						local pg = e:getGroup(true)
						if pg then
							pg:setFlags(FL_REDRAW)
						end
					end

					if damage == 1 then
						 -- unconditionally slate the element for repaint:
						e:setFlags(FL_REDRAW)
					elseif damage == 2 and r1 then -- FL_LAYOUT not realiable
						 -- unconditionally slate the element and all its
						 -- children for repaint:
						if e:checkFlags(FL_SHOW + FL_LAYOUT) then
							e:damage(r1, r2, r3, r4)
						end
					end
				end

				-- if layout changes during layouting, we need another turn.
				-- Next time make sure that minmax values are updated:
				updateminmax = true

			until #nrl == 0

			-- mouse could point to a different element now:
			if self.HoverElement then
				self.HoverElement = self:getByXY(self.MouseX, self.MouseY)
			end

		end

		if self:checkClearFlags(FL_UPDATE) then
			self:draw()
		end

	end

	return self.Status == "show"
end

-------------------------------------------------------------------------------
--	layout: overrides
-------------------------------------------------------------------------------

function Window:layout(_, _, _, _, markdamage)
	self.FreeRegion = false
	local w, h = self.Drawable:getAttrs()
	return Group.layout(self, 0, 0, w - 1, h - 1, markdamage)
end

-------------------------------------------------------------------------------
--	relayout:
-------------------------------------------------------------------------------

function Window:relayout(e, x0, y0, x1, y1, markdamage)
	local temp = { e }
	while not e:checkFlags(FL_ISWINDOW) do
		e = e:getParent()
		insert(temp, e)
	end
	local last = #temp + 1
	local n = 0
	for i = #temp, 2, -1 do
		local e = temp[i]
		if not e:drawBegin() then
			break
		end
		last = i
		n = n + 1
	end
	local res = false
	if last == 2 then
		res = temp[1]:layout(x0, y0, x1, y1, markdamage)
	end
	local j = 0
	for i = last, #temp do
		temp[i]:drawEnd()
		j = j + 1
	end
	assert(j == n)
	
	return res
end

-------------------------------------------------------------------------------
--	setHiliteElement(element): Sets/unsets the element which is being
--	hovered by the mouse pointer.
-------------------------------------------------------------------------------

function Window:setHiliteElement(e)
	local he = self.HiliteElement
	if e ~= he then
		if he then
			he:setValue("Hilite", false)
		end
		if not e or e:checkHilite() then
			self.HiliteElement = e or false
			if e then
				e:setValue("Hilite", true)
			end
		end
	end
end

-------------------------------------------------------------------------------
--	setFocusElement(element): Sets/unsets the element which is marked
--	for receiving the keyboard input.
-------------------------------------------------------------------------------

function Window:setFocusElement(e)
	local fe = self.FocusElement
	if e ~= fe then
		if fe then
			fe:setValue("Focus", false)
		end
		if not e or e:checkFocus() then
			self.FocusElement = e or false
			if e then
				e:setValue("Focus", true)
			end
		end
	end
end

-------------------------------------------------------------------------------
--	setActiveElement(element): Sets/unsets the element which is
--	currently active (or 'in use'). If {{element}} is '''nil''', the currently
--	active element will be deactivated.
-------------------------------------------------------------------------------

function Window:setActiveElement(e)
	local se = self.ActiveElement
	if e ~= se then
		if se then
			self:remInputHandler(ui.MSG_INTERVAL, self, self.handleHold)
			se:setValue("Active", false)
		end
		if not e or (e:instanceOf(Widget) and not e.Disabled) then
			self.ActiveElement = e or false
			if e then
				self.HoldTickActive = self.HoldTickFirst
				self.HoldTickActiveInit = self.HoldTickFirst
				e:setValue("Active", true)
				self:addInputHandler(ui.MSG_INTERVAL, self, self.handleHold)
			end
		end
	end
end

-------------------------------------------------------------------------------
--	handleHold: use interval messages to decrease a counter for hold events:
-------------------------------------------------------------------------------

function Window:handleHold(msg)
	local ae = self.ActiveElement
	if ae and ae.Active then
		self.HoldTickActive = self.HoldTickActive - 1
		if self.HoldTickActive <= 0 then
			self.HoldTickActiveInit = self.HoldTickRepeat
			self.HoldTickActive = self.HoldTickRepeat
			ae:setValue("Hold", true, true)
		end
	end
	return msg
end

-------------------------------------------------------------------------------
--	dblclick = checkDblClickTime(as, au, bs, bu): Check if the two
--	given times (first a, second b) are within the doubleclick interval.
--	Each time is specified in seconds ({{s}}) and microseconds ({{u}}).
--	Returns '''true''' if the two times are indicative of a double click.
-------------------------------------------------------------------------------

local function subtime(as, au, bs, bu)
	if au < bu then
		return as - bs - 1, 1000000 - bu + au
	end
	return as - bs, au - bu
end

function Window:checkDblClickTime(as, au, bs, bu)
	bs, bu = subtime(bs, bu, as, au)
	return subtime(bs, bu, 0, ui.DBLCLICKTIME) < 0
end

-------------------------------------------------------------------------------
--	setDblClickElement(element): Sets/unsets the element which is
--	candidate for double click detection. If the element is set twice in a
--	sufficiently short period of time and the pointing device did not move
--	too much since the first event, the double click is triggered by
--	notifying the {{DblClick}} attribute in the element. See also
--	[[#tek.ui.class.widget : Widget]] for further information.
-------------------------------------------------------------------------------

function Window:setDblClickElement(e)
	local di = self.DblClickCheckInfo
	local de = self.DblClickElement
	if de then
		de:setValue("DblClick", false)
		self.DblClickElement = false
	end
	local d = self.Drawable
	if e and d then
		de = di[1] -- check element
		local ts, tu = Display.getTime()
		if de == e and di[4] then
			local d1 = self.MouseX - di[4]
			local d2 = self.MouseY - di[5]
			if self:checkDblClickTime(di[2], di[3], ts, tu) and
				d1 * d1 + d2 * d2 < ui.DBLCLICKJITTER then
				self.DblClickElement = e
				de:setValue("DblClick", true)
			end
		end
		di[1] = e or false
		di[2] = ts
		di[3] = tu
		di[4] = self.MouseX
		di[5] = self.MouseY
	end
end

-------------------------------------------------------------------------------
--	setMovingElement(element): Sets/unsets the element which is
--	being moved around by the user.
-------------------------------------------------------------------------------

function Window:setMovingElement(e)
	if e ~= self.MovingElement then
		self.MovingElement = e or false
	end
end

-------------------------------------------------------------------------------
--	getParent: overrides
-------------------------------------------------------------------------------

function Window:getParent()
end

-------------------------------------------------------------------------------
--	getSiblings: overrides
-------------------------------------------------------------------------------

function Window:getSiblings()
end

-------------------------------------------------------------------------------
--	getPrev: overrides
-------------------------------------------------------------------------------

function Window:getPrev()
	local c = self:getChildren()
	return c and c[#c]
end

-------------------------------------------------------------------------------
--	getNext: overrides
-------------------------------------------------------------------------------

function Window:getNext()
	local c = self:getChildren()
	return c and c[1]
end

-------------------------------------------------------------------------------
--	getNextElement: Cycles through elements, gets the next element that can
--	receive the focus.
-------------------------------------------------------------------------------

function Window:getNextElement(e, backward)
	local oe = e
	local ne
	repeat
		oe = oe or ne
		if oe then
			local c = e:getChildren()
			if backward then
				ne = c and c[#c] or e:getPrev("recursive")
			else
				ne = c and c[1] or e:getNext("recursive")
			end
		else
			local c = self:getChildren()
			if backward then
				ne = c and c[#c]
			else
				ne = c and c[1]
			end
		end
		if ne and ne:checkFocus() then
			return ne
		end
		e = ne
	until not e or e == oe
end

-------------------------------------------------------------------------------
--	addKeyShortcut:
-------------------------------------------------------------------------------

function Window:addKeyShortcut(keycode, element)
	if type(keycode) == "string" then
		local key, quals = ui.resolveKeyCode(keycode)
		db.info("binding shortcut: %s -> %s", key, element:getClassName())
		local keytab = self.KeyShortcuts[key]
		if not keytab then
			keytab = { }
			self.KeyShortcuts[key] = keytab
		end
		for i = 1, #quals do
			local qual = quals[i]
			local qualtab = keytab[qual]
			if not qualtab then
				qualtab = { }
				keytab[qual] = qualtab
			end
			db.trace("%s : adding qualifier %d", key, qual)
			insert(qualtab, element)
		end
	end
end

-------------------------------------------------------------------------------
--	remKeyShortcut:
-------------------------------------------------------------------------------

function Window:remKeyShortcut(keycode, element)
	if type(keycode) == "string" then
		local key, quals = ui.resolveKeyCode(keycode)
		db.info("removing shortcut: %s -> %s", key, element:getClassName())
		local keytab = self.KeyShortcuts[key]
		if keytab then
			for i = 1, #quals do
				local qual = quals[i]
				local qualtab = keytab[qual]
				if qualtab then
					for idx = 1, #qualtab do
						local e = qualtab[idx]
						if element == e then
							db.trace("%s : removing qualifier %d", key, qual)
							remove(qualtab, idx)
							break
						end
					end
				end
			end
		end
	end
end

-------------------------------------------------------------------------------
--	getShortcutElements:
-------------------------------------------------------------------------------

function Window:getShortcutElements(key, qual)
	local keytab = self.KeyShortcuts[key:lower()]
	return keytab and keytab[qual]
end

-------------------------------------------------------------------------------
--	clickElement(element): This function performs a simulated click on
--	the specified {{element}}; if {{element}} is a string, it will be looked up
--	using Application:getById(). This function is actually a shorthand
--	for Window:setHiliteElement(), followed by Window:setActiveElement() twice
--	(first to enable, then to disable it).
-------------------------------------------------------------------------------

function Window:clickElement(e)
	if type(e) == "string" then
		e = self:getById(e)
		assert(e, "Unknown Id")
	end
	if not e:checkFlags(ui.FL_SETUP) then
		db.info("element not setup: %s", e:getClassName())
		e.Application = self.Application
		e.Window = self
	end
	local he = self.HiliteElement
	self:setHiliteElement(e)
	self:setActiveElement(e)
	self:setActiveElement()
	self:setHiliteElement(he)
end

-------------------------------------------------------------------------------
--	onHide(): This handler is invoked when the window's close button
--	is clicked (or the Escape key is pressed and the {{HideOnEscape}} flag
--	is set). The Window class' implementation of this handler is to hide
--	the window by setting the {{Status}} attribute to {{"hide"}}. When the
--	last window is closed, the application is closed down.
-------------------------------------------------------------------------------

function Window:onHide()
	self:setValue("Status", "hide")
end

-------------------------------------------------------------------------------
--	addBlit:
-------------------------------------------------------------------------------

function Window:addBlit(x0, y0, x1, y1, dx, dy, c1, c2, c3, c4)
	if c1 and x0 and
		intersect(x0 + dx, y0 + dy, x1 + dx, y1 + dy, c1, c2, c3, c4) then
		x0, y0, x1, y1 = intersect(x0, y0, x1, y1, c1, c2, c3, c4)
	end
	if x0 then
		if x1 >= x0 and y1 >= y0 then
			local key
			if c1 then
				key = ("%d:%d:%d:%d:%d:%d"):format(dx, dy, c1, c2, c3, c4)
			else
				key = ("%d:%d"):format(dx, dy)
			end
			local ca = self.Blits
			if ca[key] then
				ca[key][3]:orRect(x0, y0, x1, y1)
			else
				ca[key] = { dx, dy, 
					Region.new(x0, y0, x1, y1), c1, c2, c3, c4 }
			end
		else
			db.warn("illegal blitrect: %s %s %s %s", x0, y0, x1, y1)
		end
	end
end

-------------------------------------------------------------------------------
--	reconfigure:
-------------------------------------------------------------------------------

function Window:reconfigure()
	Group.reconfigure(self)
	if self.Status == "show" then
		for k, v in pairs(self.PenTable) do
			self.PenTable[k] = nil
		end
	end
end


function Window:onSetInvisible()
	-- no effect on windows
	self.Invisible = false
end

return Window
