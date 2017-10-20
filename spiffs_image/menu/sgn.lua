local stf=dofile "gstuff/pwm.lua"

local menu_sgn={
	{name="Signal 0", ind_t=stf.ind, act=stf.acts, par=6},
	{name="Signal 1", ind_t=stf.ind, act=stf.acts, par=5},
	{name="..to main menu"}
	}
stf=nil

return menu_sgn
