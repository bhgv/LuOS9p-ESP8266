
local ui = require "tek.ui"


local image_loader = require "conf.gui.plugins.lib.image_loader"
local file_loader = require "conf.gui.plugins.lib.file_loader"
local radiobuttons = require "conf.gui.plugins.lib.radiobuttons"
local lua_editor = require "conf.gui.plugins.lib.editor"


local function complexcontrol(tmpl, v)
  local wgt
  
      if tmpl:match("^<ImageLoader>") then
        wgt = image_loader(tmpl, v)
        
      elseif tmpl:match("^<FILE>") then
        wgt = file_loader(tmpl, v)
        
      elseif tmpl:match("^<RADIO>") then
        wgt = radiobuttons(tmpl, v)
        
      elseif tmpl:match("^<LUA EDITOR>") then
        wgt = lua_editor(tmpl, v)
        
      end


  return wgt
end



return complexcontrol

