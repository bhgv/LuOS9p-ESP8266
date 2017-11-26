local stf=dofile "gstuff/menu.lua"

local menu_main = {
	{name="Ip=" .. net.localip("*s"), },
	{name="run Sta", act={ok=function() net.sta("TP-LINK", ""); end, }, },
	{name="run AP", act={ok=function() net.ap("esp_tst", ""); end, }, },
	{name="exit HTTPd", act={ok=function() menus_loop=true; gui.exit(); end, }, },
	{name=" ..redraw"}
};
stf=nil

return menu_main
