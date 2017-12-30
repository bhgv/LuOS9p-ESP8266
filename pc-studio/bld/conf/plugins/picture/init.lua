-------------------------
-- copyright by bhgv 2016
-------------------------

local imhlp = require "conf.utils.image_helper"

g = require "conf.gen.gcode"  


local function header(frq)
  g:start()
  g:set_param("absolute")
  g:set_param("metric")

  g:spindle_freq(frq)
  g:spindle_on(true)
end


local function footer(z_wlk)
  g:walk_to{z = z_wlk}
  g:walk_to{x = 0, y = 0}
  g:walk_to{z = 0}
  g:spindle_on(false)
  
  g:finish()
end


local get_pix_map = function(pars)
  --print("tst=", pars["<ImageLoader>"])
  if not (pars["<ImageLoader>"] and imhlp:loadImage(pars["<ImageLoader>"])) then
    return
  end
  
  local i, j
  local map = {max = 0}
  local ln
  
  local mix_max = 0
  
  for i = 0, imhlp.H-1 do
    ln = {}
    for j = 0, imhlp.W-1 do
      local px = imhlp:getPixel(j, i)
      
      local b = px & 255
      px = px >> 8 
      local g = px & 255
      px = px >> 8
      local r = px & 255
      
      local mix = 0xff - math.ceil((r + g + b) / 3)
--      local mix = math.ceil((r + g + b) / 3)
      table.insert(ln, mix)
      if mix > mix_max then mix_max = mix end
    end
    table.insert(map, ln)
    --print(table.concat(ln, " "))
  end
  map.max = mix_max
  return map
end




local old_state = {}

local process = function(st, z, pos)
          if 
              old_state.z ~= z 
              or old_state.st ~= st 
--              or (st == "end_pt" and old_state.st == "work")
          then
            if old_state.st == "walk" then
              g:walk_to(old_state.pos)
            else
              g:work_to(old_state.pos)
--              st = "work"
            end
            if --[[st == "walk" and]] old_state.z <= z then
              g:walk_to{z = z}
            else
              g:work_to{z = z}
            end
            old_state.z = z
            old_state.st = st
          end
          old_state.pos = pos
end


local slice = function(map, clr_cur_level)
    local sl = {}
    local i, j, ln, clr
    
    --print ("----------------------------")
    for i = 1, #map do
      ln = map[i]
      local sl_ln = {}
      local last_j = nil
      local first_j = nil
      for j = 1, #ln do
        clr = ln[j]
        if clr >= clr_cur_level then
          sl_ln[j] = 1
          last_j = j
          if not first_j then
            first_j = j
          end
        else
          sl_ln[j] = 0
        end
        --sl_ln[j] = clr > clr_cur_level
      end
      if last_j then
        sl_ln[last_j] = 2
      end
      if first_j then
        sl_ln[first_j] = 2
      end
      sl[i] = sl_ln
      --print(table.concat(sl_ln, " "))
    end
    return sl
end


local image2gcode = function(map, pars)
  local z_wlk = tonumber(pars["Walk Z"]) or 10
  local z_end = tonumber(pars.Depth) or 5
  local frq = tonumber(pars.Frequency) or 40
  local n_pas = tonumber(pars["Passes (Z)"]) or 10
  
  local cnc_w = tonumber(pars.Width) or 50
  local cnc_h = tonumber(pars.Height) or 50
  local mm_per_cut = tonumber(pars["Mm per pass (Y)"]) or .5
  
  local clr_stp = tonumber(pars["Color->Z step (0-1)"]) or .5
  if clr_stp < 0 then clr_stp = 0 end
  if clr_stp > 1 then clr_stp = 1 end
  local clr_quantum = clr_stp * 0xff
    
  local dx, dy = cnc_w/imhlp.W, cnc_h/imhlp.H
  
  local dz = z_end / n_pas
  local z, i, j, d_mm, ln, pas, clr_cur_level
  local pt
  
  header(frq)
  
  old_state = {st="walk", z=0, pos={x = 0, y = 0}}
  process("walk", z_wlk, {x = 0, y = (imhlp.H - 1)*dy} )

  for pas = 1, n_pas, 1 do
    z = pas * dz
    clr_cur_level = math.floor(pas * clr_quantum)
    local sl = slice(map, clr_cur_level)
    
    for i = 1,#sl do
      ln = sl[i]
      
      local ln_from, ln_to
      local iter = 1
      
      for d_mm = 0, dy, mm_per_cut do
        local start_ln = true
        
        if iter == 1 then
          ln_from = 1
          ln_to = #ln
        else
          ln_from = #ln
          ln_to = 1
        end
        
        process( "walk", z_wlk, {x = ln_from*dx, y = (imhlp.H - i)*dy - d_mm} )
        
        for j = ln_from, ln_to, iter do
          pt = ln[j]
          
          if pt == 0 then
            process( "walk", z_wlk, {x = j*dx, y = (imhlp.H - i)*dy - d_mm} )
          elseif pt == 1 or start_ln then
            process( "work", -z, {x = j*dx, y = (imhlp.H - i)*dy - d_mm} )
            start_ln = false
          else -- pt == 2
            process( "work", -z, {x = j*dx, y = (imhlp.H - i)*dy - d_mm} )
            if not start_ln then
              process( "walk", z_wlk, {x = j*dx, y = (imhlp.H - i)*dy - d_mm - d_mm} )
--              process( "end_pt", -z, {x = j*dx, y = (imhlp.H - i)*dy - d_mm - d_mm} )
              start_ln = false
              break
            else
              start_ln = false
            end
          end
          
        end -- for j
        
        if not start_ln then
          iter = -iter
        end
      end -- for d_mm
    end -- for i
  end -- for pas
  
  process( "walk", z_wlk, {x = 0, y = 0} )
  footer(z_wlk)
end




return {
  name = "Image (*.ppm) to g-code",
  type = "plugin", 
  subtype = "CAM",
  gui = "button",
  image = nil,
  symbol = "\u{e02c}",
  
  exec = function(self, pars)
    --for k,v in pairs(pars) do
    --  print("exec ", k, v)
    --end
    local map = get_pix_map(pars)
    if map then
      image2gcode(map, pars)
    end
  end,
}
