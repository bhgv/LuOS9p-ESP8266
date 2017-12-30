
local List = require "tek.class.list"


local NumberedList = List:newClass({_NAME="_numberedlist"})

--[[
function NumberedList.new(class, self)
  local o = List:new(self)
  return o
end
]]

function NumberedList:getItem(lnr)
  local itm = List.getItem(self, lnr)
  if itm then
    itm[1][1] = tostring(lnr)
  end
  return itm
end

return NumberedList
