local stf=dofile "gstuff/pwm.lua"

local menu_led={
	{name="Led 1", ind_t=stf.ind, act=stf.acts, par=10},
	{name="Led 2", ind_t=stf.ind, act=stf.acts, par=11},
	{name="Led 3", ind_t=stf.ind, act=stf.acts, par=12},
	{name="Led 4", ind_t=stf.ind, act=stf.acts, par=13},
	{name="Led 5", ind_t=stf.ind, act=stf.acts, par=14},
	{name="Led 6", ind_t=stf.ind, act=stf.acts, par=15},
	{name=" ..main menu"}
}
stf=nil

return menu_led
