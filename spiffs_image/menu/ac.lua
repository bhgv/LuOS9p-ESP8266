local stf=dofile "gstuff/pio.lua"

local menu_ac={
	{name="AC 1", act=stf.acts, par='AC1', ind_t=stf.ind },
	{name="AC 2", act=stf.acts, par='AC2', ind_t=stf.ind },
	{name="AC 3", act=stf.acts, par='AC3', ind_t=stf.ind },
	{name=" ..main menu"}
}
stf=nil

return menu_ac
