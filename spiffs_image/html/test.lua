
--if pass and pass > 5 then return "" end

if not pass then
 pass = 1
 return "<br>test str " .. _GET()
elseif pass == 1 then
 pass = 2
 i = 1
 return "<br> adc[2] = " .. adc[2]
else
 pass = pass + 1

 if _GET() >= i then
  local k,v = _GET(i)
  i = i + 1
  return "<br>" .. k .. " = " .. v
 else
  pass = nil
  i = nil
  return ""
 end
end
