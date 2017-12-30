-------------------------------------------------------------------------------
--
--	tek.class
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	OVERVIEW::
--		[[#ClassOverview]] : Class ${subclasses(Class)}
--
--		This module implements inheritance and the creation of objects
--		from classes.
--
--	IMPLEMENTS::
--		- object:getClass() - Returns the class of an object, or the super
--		class of a class
--		- object:getClassName() - Returns the class name of an object or class
--		- object:getSuper() - Returns the super class of an object or class
--		- object:instanceOf() - Checks if an object descends from a class
--		- Class.module() - Creates a new class from a superclass
--		- Class.new() - Creates and returns a new object
--		- Class.newClass() - Creates a child class from a super class
--		- object:setClass() - Changes the class of an object, or the super
--		class of a class
--
-------------------------------------------------------------------------------

local assert = assert
local error = error
local require = require
local tostring = tostring
local getmetatable = getmetatable
local setmetatable = setmetatable

-- use proxied object model:
local PROXY = false
-- in proxied mode, trace uninitialized variable accesses:
local DEBUG = false

--[[ header token for documentation generator:
module "tek.class"
local Class = _M
]]

local Class = { }
Class._NAME = "tek.class"
Class._VERSION = "Class 9.0"
Class.__index = Class

if PROXY then

	function Class.new(class, self)
		self = self or { }

		local mt = { __class = class }

		if DEBUG then
			function mt.__index(tab, key)
				local val = mt[key]
				if not val then
					error(("Uninitialized read: %s.%s"):format(
						tab:getClassName(), key), 2)
				end
				return val
			end
			function mt.__newindex(tab, key, val)
				error(("Uninitialized write: %s.%s=%s"):format(
					tab:getClassName(), key,
					tostring(val)), 2)
				mt[key] = val
			end
		else
			mt.__index = mt
			mt.__newindex = mt
		end

		setmetatable(mt, class)
		return setmetatable(self, mt)
	end

	function Class:getClass()
		local mt = getmetatable(self)
		return mt.__class or mt
	end

	function Class:setClass(class)
		local mt = getmetatable(self)
		mt.__class = class
		setmetatable(mt, class)
	end

	function Class:getSuper()
		return getmetatable(self.__class or self)
	end

else

-------------------------------------------------------------------------------
--	object = Class.new(class[, object]):
--	Creates and returns a new object of the given {{class}}. Optionally,
--	it just prepares the specified {{object}} (a table) for inheritance and
--	attaches the {{class}}' methods and data.
-------------------------------------------------------------------------------

	function Class.new(class, self)
		return setmetatable(self or { }, class)
	end

-------------------------------------------------------------------------------
--	class = object:getClass():
--	This function returns the class of an {{object}}. If applied to a
--	class instead of an object, it returns its super class, e.g.:
--			superclass = Class.getClass(class)
-------------------------------------------------------------------------------

	function Class:getClass()
		return getmetatable(self)
	end

-------------------------------------------------------------------------------
--	object:setClass(class): Changes the class of an object
--	or the super class of a class.
-------------------------------------------------------------------------------

	function Class:setClass(class)
		setmetatable(self, class)
	end

-------------------------------------------------------------------------------
--	object:getSuper(): Gets the super class of an object or class.
--	For example, to forward a call to a {{method}} to its super class,
--	without knowing its class:
--			self:getSuper().method(self, ...)
-------------------------------------------------------------------------------

	function Class:getSuper()
		return getmetatable(self.__index or getmetatable(self))
	end

end

-------------------------------------------------------------------------------
--	class = Class.newClass(superclass[, class]):
--	Derives a new class from the specified {{superclass}}. Optionally,
--	an existing {{class}} (a table) can be specified. If a {{_NAME}} field
--	exists in this class, it will be used. Otherwise, or if a new class is
--	created, {{class._NAME}} will be composed from {{superclass._NAME}} and
--	an unique identifier. The same functionality can be achieved by calling
--	a class like a function, so the following invocations are equivalent:
--			class = Class.newClass(superclass)
--			class = superclass()
--	The second notation allows a super class to be passed as the second
--	argument to Lua's {{module}} function, which will set up a child class
--	inheriting from {{superclass}} in the module table.
-------------------------------------------------------------------------------

function Class.newClass(superclass, class)
	local class = class or { }
	class.__index = class
	class.__call = newClass
	class._NAME = class._NAME or superclass._NAME ..
		tostring(class):gsub("^table: 0x(.*)$", ".%1")
	return setmetatable(class, superclass)
end

-------------------------------------------------------------------------------
--	name = object:getClassName(): This function returns the {{_NAME}}
--	attribute of the {{object}}'s class. It is also possible to apply this
--	function to a class instead of an object.
-------------------------------------------------------------------------------

function Class:getClassName()
	return self._NAME
end

-------------------------------------------------------------------------------
--	is_descendant = object:instanceOf(class):
--	Returns '''true''' if {{object}} is an instance of or descends from the
--	specified {{class}}. It is also possible to apply this function to a class
--	instead of an object, e.g.:
--			Class.instanceOf(Button, Area)
-------------------------------------------------------------------------------

function Class:instanceOf(ancestor)
	assert(ancestor)
	while self ~= ancestor do
		self = getmetatable(self)
		if not self then
			return false
		end
	end
	return true
end

-------------------------------------------------------------------------------
--	class, superclass = Class.module(classname, superclassname): Creates a new
--	class from the specified superclass. {{superclassname}} is being loaded
--	and its method Class:newClass() invoked with the {{_NAME}} field set to
--	{{classname}}. Both class and superclass will be returned.
-------------------------------------------------------------------------------

function Class.module(classname, superclassname)
	local superclass = require(superclassname)
	return superclass:newClass { _NAME = classname }, superclass
end

return Class
