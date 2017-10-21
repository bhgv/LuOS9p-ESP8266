local stf=dofile "gstuff/dac.lua"

local menu_dac={
	{name="DAC", act=stf.acts, ind_t=stf.ind },
	{name=" ..main menu"}
}
stf = nil

return menu_dac
