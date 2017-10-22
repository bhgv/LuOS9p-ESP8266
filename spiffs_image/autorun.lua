
p=print

for k in pairs(os) do _G[k]=os[k] end

g=gpio


gui=dofile "gui/init.lua"
--gui=require "gui"


gui.menu_loop "menu/main"
