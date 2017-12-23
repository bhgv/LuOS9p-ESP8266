
local sel_foo = function(k) 
    _G.sel_mnu=k; 
    --print("sel_mnu =", k);
    gui.exit(); 
end

local sel_acts = {
    ok = sel_foo,
}

local menu_sel = {
	{name="run HTTPd", par=1, act=sel_acts, },
	{name="sel Ap/Sta", par=2, act=sel_acts, },
	{name="run Dev Menu", par=3, act=sel_acts, },
	{name="run Solder", par=4, act=sel_acts, },
	{name="exit Select", par=5, act=sel_acts, },
	{name=" ..redraw"}
};

return menu_sel
