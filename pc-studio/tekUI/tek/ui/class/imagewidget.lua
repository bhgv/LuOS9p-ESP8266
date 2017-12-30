-------------------------------------------------------------------------------
--
--	tek.ui.class.imagewidget
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
--		ImageWidget ${subclasses(ImageWidget)}
--
--		This class implements widgets with an image.
--		Images are instances of the Image class, which handles pixmaps
--		and simple vector graphics.
--		They can be obtained by ui.loadImage(), ui.getStockImage(),
--		or by directly instantiating the Image class or derivations thereof.
--
--	ATTRIBUTES::
--		- {{Image [ISG]}} (image object)
--
--	IMPLEMENTS::
--		- ImageWidget:onSetImage() - Handler for the {{Image}} attribute
--
--	OVERRIDES::
--		- Object.addClassNotifications()
--		- Area:askMinMax()
--		- Area:draw()
--		- Area:layout()
--		- Object.new()
--
-------------------------------------------------------------------------------

local ui = require "tek.ui".checkVersion(112)
local Widget = ui.require("widget", 25)
local Region = ui.loadLibrary("region", 10)
local floor = math.floor
local max = math.max
local tonumber = tonumber
local type = type
local unpack = unpack or table.unpack

local ImageWidget = Widget.module("tek.ui.class.imagewidget", "tek.ui.class.widget")
ImageWidget._VERSION = "ImageWidget 15.2"

-------------------------------------------------------------------------------
--	addClassNotifications: overrides
-------------------------------------------------------------------------------

function ImageWidget.addClassNotifications(proto)
	ImageWidget.addNotify(proto, "Image", ui.NOTIFY_ALWAYS, 
		{ ui.NOTIFY_SELF, "onSetImage" })
	return Widget.addClassNotifications(proto)
end

ImageWidget.ClassNotifications = 
	ImageWidget.addClassNotifications { Notifications = { } }

-------------------------------------------------------------------------------
--	new: overrides
-------------------------------------------------------------------------------

function ImageWidget.new(class, self)
	self = self or { }
	self.EraseBG = false
	self.HAlign = self.HAlign or "center"
	self.VAlign = self.VAlign or "center"
	if type(self.Image) == "string" then
		self.Image = ui.getStockImage(self.Image)
	end
	self.Image = self.Image or false
	self.ImageAspectX = self.ImageAspectX or 1
	self.ImageAspectY = self.ImageAspectY or 1
	self.ImageData = { } -- layouted x, y, width, height
	self.ImageWidth = self.ImageWidth or false
	self.ImageHeight = self.ImageHeight or false
	self.Region = false
	self = Widget.new(class, self)
	self:setImage(self.Image)
	return self
end

-------------------------------------------------------------------------------
--	askMinMax: overrides
-------------------------------------------------------------------------------

function ImageWidget:askMinMax(m1, m2, m3, m4)
	local d = self.ImageData
	
	local mw = self:getAttr("MinWidth")
	local mh = self:getAttr("MinHeight")
	local iw = self.ImageWidth
	local ih = self.ImageHeight
	local ax = self.ImageAspectX
	local ay = self.ImageAspectY
	
	if iw then
		iw = max(iw, mw)
	else
		iw = floor((ih or mh) * ax / ay)
	end
	
	if ih then
		ih = max(ih, mh)
	else
		ih = floor((iw or mw) * ay / ax)
	end
	
	return Widget.askMinMax(self, m1 + iw, m2 + ih, m3 + iw, m4 + ih)
end

-------------------------------------------------------------------------------
--	setImage:
-------------------------------------------------------------------------------

function ImageWidget:setImage(img)
	self.Image = img
	self:setFlags(ui.FL_CHANGED + ui.FL_REDRAW)
	if img then
		local iw, ih = img:askWidthHeight(false, false)
		if iw ~= self.ImageWidth or ih ~= self.ImageHeight then
			self.ImageWidth = iw
			self.ImageHeight = ih
			self:rethinkLayout(1, true)
		end
	end
end

-------------------------------------------------------------------------------
--	layout: overrides
-------------------------------------------------------------------------------

function ImageWidget:layout(r1, r2, r3, r4, markdamage)
	local res = Widget.layout(self, r1, r2, r3, r4, markdamage)
	if self:checkClearFlags(ui.FL_CHANGED) or res then
		local r1, r2, r3, r4 = self:getRect()
		local p1, p2, p3, p4 = self:getPadding()

		local x, y = r1, r2
		local rw = r3 - x + 1
		local rh = r4 - y + 1
		local w = rw - p1 - p3
		local h = rh - p2 - p4
		local id = self.ImageData
		local iw, ih
		
		if self.ImageWidth then
			-- given size:
			iw, ih = self.ImageWidth, self.ImageHeight
		else
			-- can stretch:
			iw, ih = self.Application.Display:fitMinAspect(w, h,
				self.ImageAspectX, self.ImageAspectY, 0)
		end
		
		if iw ~= rw or ih ~= rh then
			self.Region = Region.new(x, y, r3, r4)
		elseif self.Image and self.Image[4] then -- transparent?
			self.Region = Region.new(x, y, r3, r4)
		else
			self.Region = false
		end
		x = x + p1
		y = y + p2
		if iw ~= rw or ih ~= rh then
			
			local ha = self.HAlign
			if ha == "center" then
				x = x + floor((w - iw) / 2)
			elseif ha == "right" then
				x = x + w - iw
			end
			local va = self.VAlign
			if va == "center" then
				y = y + floor((h - ih) / 2)
			elseif va == "bottom" then
				y = y + h - ih
			end
			if self.Image and not self.Image[4] then -- transparent?
				self.Region:subRect(x, y, x + iw - 1, y + ih - 1)
			end
		end
		id[1], id[2], id[3], id[4] = x, y, iw, ih
	end
	return res
end

-------------------------------------------------------------------------------
--	draw: overrides
-------------------------------------------------------------------------------

function ImageWidget:draw()
	if Widget.draw(self) then
		local d = self.Window.Drawable
		local R = self.Region
		local img = self.Image
		if R then
			d:setBGPen(self:getBG())
			R:forEach(d.fillRect, d)
		end
		if img then
			local x, y, iw, ih = unpack(self.ImageData)
			local pen = self.FGPen
			img:draw(d, x, y, x + iw - 1, y + ih - 1, 
				pen ~= "transparent" and pen)
		end
		return true
	end
end

-------------------------------------------------------------------------------
--	onSetImage(): This handler is invoked when the element's {{Image}}
--	attribute has changed.
-------------------------------------------------------------------------------

function ImageWidget:onSetImage()
	self:setImage(self.Image)
end

return ImageWidget
