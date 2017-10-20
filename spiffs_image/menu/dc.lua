local stf=dofile "gstuff/pwm.lua"

local menu_dc={
	{name="DC 1", act=stf.acts, par=9, ind_t=stf.ind },
	{name="DC 2", act=stf.acts, par=8, ind_t=stf.ind },
	{name="DC 3", act=stf.acts, par=7, ind_t=stf.ind },
	{name="..to main menu"}
}
stf=nil

return menu_dc
