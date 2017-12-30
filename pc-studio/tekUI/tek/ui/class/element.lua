-------------------------------------------------------------------------------
--
--	tek.ui.class.element
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	OVERVIEW::
--		[[#ClassOverview]] :
--		[[#tek.class : Class]] /
--		[[#tek.class.object : Object]] /
--		Element ${subclasses(Element)}
--
--		This class implements the connection to a global environment and
--		the registration by Id.
--
--	ATTRIBUTES::
--		- {{Application [G]}} ([[#tek.ui.class.application : Application]])
--			The Application the element is registered with.
--			This attribute is set during Element:setup().
--		- {{Class [ISG]}} (string)
--			The name of the element's style class, which can be referenced
--			in a style sheet. Multiple classes can be specified by separating
--			them with spaces, e.g. {{"button knob warn"}}. Setting this
--			attribute invokes the Element:onSetClass() method.
--		- {{Id [IG]}} (string)
--			An unique Id identifying the element. If present, this Id will be
--			registered with the Application during Element:setup().
--		- {{Parent [G]}} (object)
--			Parent object of the element. This attribute is set during
--			Element:connect().
--		- {{Properties [G]}} (table)
--			A table of properties, resulting from element and user style
--			classes, overlaid with individual and direct formattings, and
--			finally from hardcoded element properties. This table is
--			initialized in Element:decodeProperties().
--		- {{Style [ISG]}} (string)
--			Direct style formattings of the element, overriding class-wide
--			formattings in a style sheet. Example:
--					"background-color: #880000; color: #ffff00"
--			Setting this attribute invokes the Element:onSetStyle() method.
--		- {{Window [G]}} ([[#tek.ui.class.window : Window]])
--			The Window the element is registered with. This
--			attribute is set during Element:setup().
--
--	IMPLEMENTS::
--		- Element:addStyleClass() - Appends a style class to an element
--		- Element:cleanup() - Unlinks the element from its environment
--		- Element:connect() - Connects the element to a parent element
--		- Element:decodeProperties() - Decodes the element's style properties
--		- Element:disconnect() - Disconnects the element from its parent
--		- Element:getAttr() - Gets a named attribute from an element
--		- Element:getById() - Gets a registered element by Id
--		- Element:getPseudoClass() - Gets an element's pseudo class
--		- Element:onSetClass() - Gets invoked on changes of {{Class}}
--		- Element:onSetStyle() - Gets invoked on changes of {{Style}}
--		- Element:setup() - Links the element to an Application and Window
--
--	OVERRIDES::
--		- Object.addClassNotifications()
--		- Class.new()
--
-------------------------------------------------------------------------------

local Object = require "tek.class.object"
local ui = require "tek.ui".checkVersion(112)
local db = require "tek.lib.debug"

local assert = assert
local concat = table.concat
local getmetatable = getmetatable
local insert = table.insert
local pairs = pairs
local setmetatable = setmetatable
local sort = table.sort
local tonumber = tonumber
local type = type

local Element = Object.module("tek.ui.class.element", "tek.class.object")
Element._VERSION = "Element 20.2"

-------------------------------------------------------------------------------
--	Placeholders for notification arguments:
-------------------------------------------------------------------------------

-- inserts the Window:
Element.NOTIFY_WINDOW = function(a, n)
	a[a[-3]] = a[-1].Window
	return 1
end

-- inserts the Application:
Element.NOTIFY_APPLICATION = function(a, n)
	a[a[-3]] = a[-1].Application
	return 1
end

-- inserts an object of the given Id:
Element.NOTIFY_ID = function(a, n)
	a[a[-3]] = a[-1].Application:getById(n)
	return 2
end

-- denotes insertion of a function value as a new coroutine:
Element.NOTIFY_COROUTINE = function(a, n)
	a[a[-3]] = function(...) a[-1].Application:addCoroutine(n, ...) end
	return 2
end

-------------------------------------------------------------------------------
--	addClassNotifications: overrides
-------------------------------------------------------------------------------

function Element.addClassNotifications(proto)
	Element.addNotify(proto, "Style", Element.NOTIFY_ALWAYS,
		{ Element.NOTIFY_SELF, "onSetStyle" })
	Element.addNotify(proto, "Class", Element.NOTIFY_ALWAYS,
		{ Element.NOTIFY_SELF, "onSetClass" })
	return Object.addClassNotifications(proto)
end

Element.ClassNotifications =
	Element.addClassNotifications { Notifications = { } }

-------------------------------------------------------------------------------
--	init: overrides
-------------------------------------------------------------------------------

function Element.new(class, self)
	self = self or { }
	self.Application = false
	self.Class = self.Class or false
	self.Id = self.Id or false
	self.Parent = false
	self.Properties = false
	self.Style = self.Style or false
	self.Window = false
	return Object.new(class, self)
end

-------------------------------------------------------------------------------
--	success = Element:connect(parent): Attempts to connect the element to the
--	given {{parent}} element; returns a boolean indicating whether the
--	connection succeeded.
-------------------------------------------------------------------------------

function Element:connect(parent)
	self.Parent = parent
	return true
end

-------------------------------------------------------------------------------
--	Element:disconnect(): Disconnects the element from its parent.
-------------------------------------------------------------------------------

function Element:disconnect()
	self.Parent = false
end

-------------------------------------------------------------------------------
--	connectProperties: Connect an element's element style properties (internal)
-------------------------------------------------------------------------------

local function connectProperties(self, stylesheets)
	local class = self:getClass()
	local ups
	local topclass
	while class ~= Element do
		for i = 1, #stylesheets + 1 do
			local s
			if i <= #stylesheets then
				s = stylesheets[i][class._NAME]
			else
				s = class.Properties
			end
			if s then
				if not topclass then
					topclass = s
				end
				if ups then
					if getmetatable(ups) == s then
						-- already connected
						return topclass
					elseif i <= #stylesheets then
						setmetatable(ups, s)
						s.__index = s
					end
				end
				ups = s
			end
		end
		class = class:getSuper()
	end
	return topclass
end

-------------------------------------------------------------------------------
--	decodeUserClasses: internal
-------------------------------------------------------------------------------

local function mergeprops(stylesheets, record, class, key)
	class:gsub("(%S+)", function(c)
		local key = key .. c
		for i = 1, #stylesheets do
			local source = stylesheets[i][key]
			if source then
				for key, val in pairs(source) do
					if not record[key] then
						record[key] = val
					end
				end
			end
		end
	end)
end

local function decodeUserClasses(self, stylesheets, props)
	local class = self.Class
	if class then
		local classname = self._NAME
		local cachekey = classname .. "." .. class
		-- do we have this combination in our cache?
		local record = stylesheets[0][cachekey]
		if not record then
			-- create new record and cache it:
			record = { }
			stylesheets[0][cachekey] = record
			-- elementclass.class is more specific than just .class,
			-- so they must be treated in that order:
			mergeprops(stylesheets, record, class, classname .. ".")
			mergeprops(stylesheets, record, class, ".")
			record.__index = record
		end
		if props then
			props = setmetatable(record, props)
		else
			props = record
		end
	end
	return props
end

-------------------------------------------------------------------------------
--	decodeIndividualFormats: internal
-------------------------------------------------------------------------------

local function decodeIndividualFormats(self, stylesheets, props)
	local individual_formats
	local id = self.Id
	if id then
		individual_formats = { }
		for i = #stylesheets, 1, -1 do
			local s = stylesheets[i]["#" .. id]
			if s then
				for key, val in pairs(s) do
					individual_formats[key] = val
				end
			end
		end
	end
	if self.Style then
		individual_formats = individual_formats or { }
		for key, val in 
			(self.Style .. ";"):gmatch("%s*([%w%-]+)%s*:%s*([^;]-)%s*;") do
			ui.unpackProperty(individual_formats, key, val, "")
		end
	end
	if individual_formats then
		if props then
			props = setmetatable(individual_formats, props)
		else
			props = individual_formats
		end
	end
	return props
end

-------------------------------------------------------------------------------
--	Element:decodeProperties(stylesheets): This function decodes the element's
--	style properties and places them in the {{Properties}} table.
-------------------------------------------------------------------------------

local empty = { }

function Element:decodeProperties(stylesheets)

	-- connect element style classes:
	local props = connectProperties(self, stylesheets)
	if props then
		props.__index = props
	end
	
	-- overlay with user classes:
	props = decodeUserClasses(self, stylesheets, props)

	-- overlay with individual and direct formattings:
	props = decodeIndividualFormats(self, stylesheets, props)
	
	self.Properties = props or empty
end

-------------------------------------------------------------------------------
--	Element:setup(app, window): This function attaches an element to the
--	specified [[#tek.ui.class.application : Application]] and
--	[[#tek.ui.class.window : Window]], and registers the element's Id (if any)
--	at the application.
-------------------------------------------------------------------------------

function Element:setup(app, window)
-- 	assert(app and app:instanceOf(ui.Application), "No Application")
-- 	assert(window and window:instanceOf(ui.Window), "No Window")
-- 	assert(not self.Application, 
-- 		("%s: Application already set"):format(self:getClassName()))
-- 	assert(not self.Window,
-- 		("%s: Window already set"):format(self:getClassName()))
	self.Application = app
	self.Window = window
	if self.Id then
		app:addElement(self)
	end
end

-------------------------------------------------------------------------------
--	Element:cleanup(): This function unlinks the element from its
--	[[#tek.ui.class.application : Application]] and
--	[[#tek.ui.class.window : Window]].
-------------------------------------------------------------------------------

function Element:cleanup()
	if self.Id then
		self.Application:remElement(self)
	end
	self.Application = false
	self.Window = false
end

-------------------------------------------------------------------------------
--	Element:getById(id): Gets the element with the specified {{id}}, that was
--	previously registered with the [[#tek.ui.class.application : Application]].
--	This function is a shortcut for Application:getById(), applied to
--	{{self.Application}}.
-------------------------------------------------------------------------------

function Element:getById(id)
	return self.Application:getById(id)
end

-------------------------------------------------------------------------------
--	ret1, ... = Element:getAttr(attribute, ...): This function gets a named
--	{{attribute}} from an element, returning '''nil''' if the attribute is
--	unknown.
-------------------------------------------------------------------------------

function Element:getAttr()
end

-------------------------------------------------------------------------------
--	Element:addStyleClass(styleclass) - Appends a style class to an element's
--	{{Class}} attribute, if it is not already present.
-------------------------------------------------------------------------------

function Element:addStyleClass(styleclass)
	if styleclass and styleclass ~= "" then
		local class = self.Class
		if class then
			for c in class:gmatch("%S+") do
				if c == styleclass then
					return
				end
			end
			class = class .. " " .. styleclass
		else
			class = styleclass
		end
		self.Class = class
	end
	return self.Class
end

-------------------------------------------------------------------------------
--	Element:onSetStyle(): This method is invoked when the {{Style}}
--	attribute has changed.
-------------------------------------------------------------------------------

function Element:onSetStyle()
	self.Application:decodeProperties(self)
end

-------------------------------------------------------------------------------
--	Element:onSetClass(): This method is invoked when the {{Class}}
--	attribute has changed. The implementation in the Element class invokes
--	Element:onSetStyle().
-------------------------------------------------------------------------------

function Element:onSetClass()
	self:onSetStyle()
end

-------------------------------------------------------------------------------
--	pclass = Element:getPseudoClass(): Get an element's pseudo class
-------------------------------------------------------------------------------

function Element:getPseudoClass()
	return ""
end

return Element
