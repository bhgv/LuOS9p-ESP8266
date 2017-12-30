-------------------------------------------------------------------------------
--
--	tek.class.list
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	OVERVIEW::
--		[[#ClassOverview]] :
--		[[#tek.class : Class]] / List ${subclasses(List)}
--
--		This class implements a list container.
--
--	IMPLEMENTS::
--		- List:addItem() - Adds an item to the list
--		- List:changeItem() - Replaces an item in the list
--		- List:clear() - Removes all items from the list
--		- List:getItem() - Returns the item at the specified position
--		- List:getN() - Returns the number of items in the list
--		- List:remItem() - Removes an item from the list
--
--	ATTRIBUTES::
--		- {{Items}} [I] (table)
--			Table of initial list items, indexed numerically
--
--	OVERRIDES::
--		- Class.new()
--
-------------------------------------------------------------------------------

local Class = require "tek.class"
local insert = table.insert
local max = math.max
local min = math.min
local remove = table.remove

local List = Class.module("tek.class.list", "tek.class")
List._VERSION = "List 3.2"

-------------------------------------------------------------------------------
--	new: overrides
-------------------------------------------------------------------------------

function List.new(class, self)
	self = self or { }
	self.Items = self.Items or { }
	return Class.new(class, self)
end

-------------------------------------------------------------------------------
--	List:getN(): Returns the number of elements in the list.
-------------------------------------------------------------------------------

function List:getN()
	return #self.Items
end

-------------------------------------------------------------------------------
--	List:getItem(pos): Returns the item at the specified position.
-------------------------------------------------------------------------------

function List:getItem(lnr)
	return self.Items[lnr]
end

-------------------------------------------------------------------------------
--	pos = List:addItem(entry[, pos]): Adds an item to the list, optionally
--	inserting it at the specified position. Returns the position at which the
--	item was added.
-------------------------------------------------------------------------------

function List:addItem(entry, lnr)
	local numl = self:getN() + 1
	if not lnr then
		insert(self.Items, entry)
		lnr = numl
	else
		lnr = min(max(1, lnr), numl)
		insert(self.Items, lnr, entry)
	end
	return lnr
end

-------------------------------------------------------------------------------
--	entry = List:remItem(pos): Removes an item at the specified position
--	in the list. The item is returned to the caller.
-------------------------------------------------------------------------------

function List:remItem(lnr)
	if lnr > 0 and lnr <= self:getN() then
		return remove(self.Items, lnr)
	end
end

-------------------------------------------------------------------------------
--	success = List:changeItem(entry, pos): Changes the item at the specified
--	position in the list. Returns '''true''' if an item was changed.
-------------------------------------------------------------------------------

function List:changeItem(entry, lnr)
	local numl = self:getN()
	if lnr > 0 and lnr <= numl then
		self.Items[lnr] = entry
		return true
	end
end

-------------------------------------------------------------------------------
--	List:clear(): Removes all items from the list.
-------------------------------------------------------------------------------

function List:clear()
	self.Items = { }
end

return List
