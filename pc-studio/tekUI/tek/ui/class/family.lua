-------------------------------------------------------------------------------
--
--	tek.ui.class.family
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	OVERVIEW::
--		[[#ClassOverview]] :
--		[[#tek.class : Class]] /
--		[[#tek.class.object : Object]] /
--		Family ${subclasses(Family)}
--
--		This class implements a container for child objects.
--
--	ATTRIBUTES::
--		- {{Children [IG]}} (table)
--			Array of child objects
--
--	FUNCTIONS::
--		- Family:addMember() - Adds an element to a Family
--		- Family:remMember() - Removed an element from a Family
--
-------------------------------------------------------------------------------

local Object = require "tek.class.object"

local insert = table.insert
local remove = table.remove

local Family = Object.module("tek.ui.class.family", "tek.class.object")
Family._VERSION = "Family 2.7"

-------------------------------------------------------------------------------
--	Class implementation:
-------------------------------------------------------------------------------

function Family.new(class, self)
	self = self or { }
	self.Children = self.Children or { }
	return Object.new(class, self)
end

-------------------------------------------------------------------------------
--	success = addMember(child[, pos]): Invokes the {{child}}'s
--	[[connect()][#Element:connect]] method to check for its ability to
--	integrate into the family, and if successful, inserts it into the
--	Family's list of children. Optionally, the child is inserted into the
--	list at the specified position.
-------------------------------------------------------------------------------

function Family:addMember(child, pos)
	if child:connect(self) then
		if pos then
			insert(self.Children, pos, child)
		else
			insert(self.Children, child)
		end
		return true
	end
end

-------------------------------------------------------------------------------
--	success = remMember(child): Looks up {{child}} in the family's list of
--	children, and if it can be found, invokes its
--	[[disconnect()][#Element:disconnect]] method and removes it from the list.
-------------------------------------------------------------------------------

function Family:remMember(child)
	local c = self.Children
	for pos = 1, #c do
		local e = c[pos]
		if e == child then
			child:disconnect(self)
			remove(self.Children, pos)
			return true
		end
	end
end

return Family
