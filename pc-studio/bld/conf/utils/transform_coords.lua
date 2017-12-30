




return function (ipt)
  local transf = _G.Flags.Transformations 
  local x, y, z = 
                ipt.x, 
                ipt.y, 
                ipt.z
  
  --scale
  local tr_sc = transf.Scale
  x, y, z = x*tr_sc.x, y*tr_sc.y, z*tr_sc.z

  --rotate
  local ang = transf.Rotate
  if ang ~= 0 then
  x, y = 
        (x * math.cos(ang) - y * math.sin(ang)),
        (x * math.sin(ang) + y * math.cos(ang))
  end

  --move
  local tr_mv = transf.Move
  local dx, dy, dz =
                tr_mv.x or 0,
                tr_mv.y or 0,
                tr_mv.z or 0
                
  x = x + dx
  y = y + dy
  z = z + dz
  
  
  return {x=x, y=y, z=z}
end
