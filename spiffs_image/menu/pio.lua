local stf=dofile "gstuff/pio.lua"

local menu_pio={
	{name="PIO 0", act=stf.acts, par=0, ind_t=stf.ind },
	{name="PIO 1", act=stf.acts, par=1, ind_t=stf.ind },
	{name="PIO 2", act=stf.acts, par=2, ind_t=stf.ind },
	{name="PIO 3", act=stf.acts, par=3, ind_t=stf.ind },
	{name="PIO 4", act=stf.acts, par=4, ind_t=stf.ind },
	{name=" ..main menu"}
}
stf=nil

return menu_pio
