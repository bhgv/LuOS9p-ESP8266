-- test ascii85 library

local ascii85=require"ascii85"

print(ascii85.version)
print""

function test(s)
 --print""
 --print(string.len(s),s)
 local a=ascii85.encode(s)
 --print(string.len(a),a)
 local b=ascii85.decode(a)
 --print(string.len(b),b)
 print(string.len(s),b==s,a,s)
 assert(b==s)
end

for i=0,13 do
 local s=string.sub("Lua-scripting-language",1,i)
 test(s)
end

function test(p)
 print("testing prefix "..string.len(p))
 for i=0,255 do
  local s=p..string.char(i)
  local a=ascii85.encode(s)
  local b=ascii85.decode(a)
  assert(b==s,i)
 end
end

print""
test""
test"x"
test"xy"
test"xyz"

print""
print(ascii85.version)

-- eof
