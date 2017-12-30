-------------------------------------------------------------------------------
--
--	tek.class.object
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	OVERVIEW::
--		[[#ClassOverview]] :
--		[[#tek.class : Class]] / 
--		Object ${subclasses(Object)}
--
--		This class implements notifications.
--
--	IMPLEMENTS::
--		- Object.addClassNotifications() - Collects class notifications
--		- Object:addNotify() - Adds a notification to an object
--		- Object:doNotify() - Invokes a notification in the addressed object
--		- Object.init() - (Re-)initializes an object
--		- Object:remNotify() - Removes a notification from an object
--		- Object:setValue() - Sets an attribute, triggering notifications
--
--	OVERRIDES::
--		- Class.new()
--
-------------------------------------------------------------------------------

local Class = require "tek.class"
local db = require "tek.lib.debug"
local support = require "tek.lib.support" -- comment out to use pure Lua
local assert = assert
local error = error
local insert = table.insert
local ipairs = ipairs
local pairs = pairs
local pcall = pcall
local remove = table.remove
local select = select
local setmetatable = setmetatable
local type = type
local unpack = unpack or table.unpack

local Object = Class.module("tek.class.object", "tek.class")
Object._VERSION = "Object 14.3"

-------------------------------------------------------------------------------
--	constants & class data:
-------------------------------------------------------------------------------

-- denotes that any value causes an object to be notified:
Object.NOTIFY_ALWAYS = { }

-- denotes insertion of the object itself:
Object.NOTIFY_SELF = function(a, n) a[a[-3]] = a[-1] return 1  end

-- denotes insertion of the value that triggered the notification:
Object.NOTIFY_VALUE = function(a, n) a[a[-3]] = a[0] return 1  end

-- denotes insertion of the attribute's previous value:
Object.NOTIFY_OLDVALUE = function(a, n) a[a[-3]] = a[-2] return 1 end

-- denotes insertion of logical negation of the value:
Object.NOTIFY_TOGGLE = function(a, n) a[a[-3]] = not a[0] return 1 end

-- denotes insertion of the value, using the next argument as format string:
Object.NOTIFY_FORMAT = function(a, n) a[a[-3]] = n:format(a[0]) return 2 end

-- denotes insertion of a function value:
Object.NOTIFY_FUNCTION = function(a, n) a[a[-3]] = n return 2 end

-------------------------------------------------------------------------------
--	new: overrides
-------------------------------------------------------------------------------

function Object.new(class, self)
	self = self or { }
	self.Notifications = class.ClassNotifications
	return Class.new(class, class.init(self or { }))
end

-------------------------------------------------------------------------------
--	object = Object.init(object): This function is called during Object.new()
--	before passing control to {{superclass.new()}}. The original intention was
--	that resources were to be claimed in {{new()}}, whereas defaults were to
--	be (re-)initialized during {{init()}}. As of 1.04, this usage pattern is
--	discouraged; all initializations should take place in {{new()}} now, as
--	this differentiation was insufficient to cover complex reinitializations,
--	and it interferes with fields like {{Area.TrackDamage}}, which are now
--	initialization-only.
-------------------------------------------------------------------------------

function Object.init(self)
	return self
end

-------------------------------------------------------------------------------
--	Object:doNotify(func, ...): Performs a notification in the object by
--	calling the specified function and passing it the given arguments. If
--	{{func}} is a a function value, it will be called directly. Otherwise, it
--	will be used as a key for looking up the function.
-------------------------------------------------------------------------------

function Object:doNotify(func, ...)
	if type(func) == "function" then
		return func(self, ...)
	end
	func = self[func]
	if type(func) == "function" then
		return func(self, ...)
	end
end

-------------------------------------------------------------------------------
--	Object:setValue(key, value[, notify]): Sets an {{object}}'s {{key}} to
--	the specified {{value}}, and, if {{notify}} is not '''false''', triggers
--	notifications that were previously registered with the object. If
--	{{value}} is '''nil''', the key's present value is reset. To enforce a
--	notification regardless of its value, set {{notify}} to '''true'''.
--	For details on registering notifications, see Object:addNotify().
-------------------------------------------------------------------------------

local function doNotifications(a, ...)
	for j = 1, select("#", ...) do
		local n = select(j, ...)
		a[-3] = 1 -- reset argument counter
		local i = 1
		while i <= #n do
			local v = n[i]
			if type(v) == "function" then
				i = i + v(a, n[i + 1])
			else
				a[a[-3]] = v
				i = i + 1
			end
			a[-3] = a[-3] + 1
		end
		local object = remove(a, 1)
		local func = remove(a, 1)
		object:doNotify(func, unpack(a, 1, a[-3] - 3))
	end
end

function Object:setValue(key, val, notify)
	local oldval = self[key]
	if val == nil then
		val = oldval
	end
	local n = self.Notifications[key]
	if n and notify ~= false then
		if val ~= oldval or notify then
			self[key] = val
			local args = { [-2] = oldval, [-1] = self, [0] = val }
			local n2 = n[Object.NOTIFY_ALWAYS]
			for i = 1, 2 do
				if n2 then
					assert(#n2 > 0)
					doNotifications(args, unpack(n2))
				end
				n2 = n[val]
			end
		end
	else
		self[key] = val
	end
end

-------------------------------------------------------------------------------
--	Object:addNotify(attr, val, dest):
--	Adds a notification to an object. {{attr}} is the name of an attribute to
--	react on setting its value. {{val}} is the value that triggers the
--	notification. For {{val}}, the placeholder {{ui.NOTIFY_ALWAYS}} can be
--	used to react on any change of the value.
--	{{action}} is a table describing the action to take when the
--	notification occurs; it has the general form:
--			{ object, method, arg1, ... }
--	{{object}} denotes the target of the notification, i.e. the {{self}}
--	that will be passed to the {{method}} as its first argument.
--	{{ui.NOTIFY_SELF}} is a placeholder for the object causing the
--	notification (see below for the additional placeholders {{ui.NOTIFY_ID}},
--	{{ui.NOTIFY_WINDOW}}, and {{ui.NOTIFY_APPLICATION}}). {{method}} can be
--	either a string denoting the name of a function in the addressed object,
--	or {{ui.NOTIFY_FUNCTION}} followed by a function value. The following
--	placeholders are supported in the [[Object][#tek.ui.class.object]] class:
--		* {{ui.NOTIFY_SELF}} - the object causing the notification
--		* {{ui.NOTIFY_VALUE}} - the value causing the notification
--		* {{ui.NOTIFY_TOGGLE}} - the logical negation of the value
--		* {{ui.NOTIFY_OLDVALUE}} - the attributes's value prior to setting it
--		* {{ui.NOTIFY_FORMAT}} - taking the next argument as a format string
--		for formatting the value
--		* {{ui.NOTIFY_FUNCTION}} - to pass a function in the next argument
--	The following additional placeholders are supported if the notification is
--	triggered in a child of the [[Element][#tek.ui.class.element]] class:
--		* {{ui.NOTIFY_ID}} - to address the [[Element][#tek.ui.class.element]]
--		with the Id given in the next argument
--		* {{ui.NOTIFY_WINDOW}} - to address the
--		[[Window][#tek.ui.class.window]] the object is connected to
--		* {{ui.NOTIFY_APPLICATION}} - to address the
--		[[Application][#tek.ui.class.application]] the object is connected to
--		* {{ui.NOTIFY_COROUTINE}} - like {{ui.NOTIFY_FUNCTION}}, but the
--		function will be launched as a coroutine by the
--		[[Application][#tek.ui.class.application]]. See also
--		Application:addCoroutine() for further details.
--	In any case, the {{method}} will be invoked as follows:
--			method(object, arg1, ...)
--
--	If the destination object or addressed method cannot be determined,
--	nothing else besides setting the attribute will happen.
--
--	Notifications should be removed using Object:remNotify() when they are
--	no longer needed, to reduce overhead and memory use.
------------------------------------------------------------------------------

local copytable
copytable = support and support.copyTable or 
function (src, dst)
	for key, val in pairs(src) do
		if type(val) == "table" then
			val = copytable(val, { })
		end
		dst[key] = val
	end
	return dst
end

local function p_addnotify(n, attr, val, dest)
	n[attr] = n[attr] or { }
	local t = n[attr][val] or { }
	n[attr][val] = t
	t[#t + 1] = dest -- table.insert would not consider metatables
end

function Object:addNotify(attr, val, dest)
	assert(dest)
	local n = self.Notifications
	if not pcall(p_addnotify, n, attr, val, dest) then
 		db.trace("copy on write notifications : %s.%s=%s",
			self:getClassName(), attr, val)
		n = copytable(n, { })
		self.Notifications = n
		p_addnotify(n, attr, val, dest)
	end
end

-------------------------------------------------------------------------------
--	success = Object:remNotify(attr, val, dest):
--	Removes a notification from an object and returns '''true''' if it
--	was found and removed successfully. You must specify the exact set of
--	arguments as for Object:addNotify() to identify a notification for removal.
-------------------------------------------------------------------------------

function Object:remNotify(attr, val, dest)
	local n = self.Notifications
	n = n and n[attr]
	local n2 = n and n[val]
	if n2 then
		for i = 1, #n2 do
			if n2[i] == dest then
				remove(n2, i)
				if #n2 == 0 then
					n[val] = nil
				end
				return
			end
		end
	end
	db.error("Notification not found : %s[%s]", attr, val)
	return false
end

-------------------------------------------------------------------------------
--	Object.addClassNotifications(proto): class method for collecting the
--	standard or default notifications of a class. These notifications
--	remain in place, and are not removed or replaced during runtime.
-------------------------------------------------------------------------------

local romtab = { __newindex = function() error("read-only table") end }

local readonly

function readonly(tab, maxdepth, depth)
	depth = depth or 0
	if not maxdepth or depth < maxdepth then
		setmetatable(tab, romtab)
		for key, val in pairs(tab) do
			if type(val) == "table" then
				readonly(val, maxdepth, depth + 1)
			end
		end
	end
	return tab
end

function Object.addClassNotifications(proto)
	return readonly(proto.Notifications, 3)
end

Object.ClassNotifications = 
	Object.addClassNotifications { Notifications = { } }

return Object
