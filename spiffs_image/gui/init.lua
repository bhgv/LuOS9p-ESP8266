

local menu = "";
local m_cur_pos = 1;

local m_str_step = 10

local menu_cur = {};

local draw_menu = function(d)
    local i, yc;
    local y = 2; --m_str_step;
    local ln;
    local bg = 1
    if #menu_cur > 6 and m_cur_pos >= 6/2 then
	if m_cur_pos > #menu_cur - 6/2 then
	    bg = #menu_cur - 6 + 1
	else
	    bg = m_cur_pos - 6/2 + 1
	end
    end
    for i = bg,#menu_cur do
		ln = menu_cur[i];
--		yc = math.ceil(y+(m_str_step/2));
		if i == m_cur_pos then
--			d.frame(0, y, 128, m_str_step)
			d.hline(5, y+m_str_step, 128-10)
			d.box(2, y+3, 4, 4);
--		else
--			d.frame(2, y+3, 4, 4);
		end
		d.print(11, y+1, ln.name)
		if ln.ind_t ~= nil then
			ln.ind_t(d, y+1, ln.par);
		end
		y = y + m_str_step;
		if y >= 64-m_str_step then
			break;
		end
    end
end

local draw = function(draw_foo)
	local d = disp;
	d.cls();
	draw_foo(d);
    d.draw();
end

local new_cur_m=function(m)
  menu_cur=dofile(m .. ".lua");
end

local new_menu=function(m) 
	menu=m; 
	menu_cur=nil;
	--collectgarbage("collect");
	new_cur_m(menu);
	m_cur_pos=1;
	draw(draw_menu);
end

local sub_menu=function(m)
    menu_cur=nil;
    cur_ln=nil;
    act=nil;
    --collectgarbage("collect");
    new_cur_m(m);
	m_cur_pos = 1;
	draw(draw_menu);
end

function top_menu()
	new_menu(menu)
end

local menu_up=function()
	m_cur_pos=m_cur_pos-1
	if m_cur_pos<1 then 
	    m_cur_pos=1; 
	else
	    draw(draw_menu);
	end
	return m_cur_pos;
end

local menu_down=function()
	m_cur_pos=m_cur_pos+1
	if m_cur_pos>#menu_cur then 
	    m_cur_pos=#menu_cur; 
	else
	    draw(draw_menu);
	end
	return m_cur_pos;
end

local menu_pos=function(pos)
	local old = m_cur_pos;
	if pos then
	    m_cur_pos = pos;
	    draw(draw_menu);
	end
	return old;
end






local gui_controller = function()
  local b = gpio.pins;
  local ok, lft, rgt, up, dwn, lpg, rpg, ekey =
      1<<8, 1<<9, 1<<10, 1<<11, 1<<12, 1<<13, 1<<14, 1<<15;
    
  local i=m_cur_pos;
  local cur_ln=menu_cur[i];
  local m=cur_ln.menu
  local act=cur_ln.act
  local par=cur_ln.par

  if(b & ok) == 0 then
    --collectgarbage("collect");
    if i == #menu_cur then
      new_menu(menu);
    elseif m ~= nil then
		sub_menu(m);
    elseif act ~= nil and act.ok ~= nil then
	    act.ok(par);
    end
  end
  if(b & lft) == 0 then
    if act ~= nil and act.lft ~= nil then
	  act.lft(par);
	  draw(draw_menu);
    elseif m ~= nil then
		sub_menu(m);
    end
  end
  if(b & rgt) == 0 then
    if act ~= nil and act.rgt ~= nil then
	  act.rgt(par);
	  draw(draw_menu);
    elseif m ~= nil then
		sub_menu(m);
    end
  end
  if(b & up) == 0 then
    menu_up();
  end
  if(b & dwn) == 0 then
    menu_down();
  end
  if(b & lpg) == 0 then
    if act ~= nil and act.lpg ~= nil then
	  act.lpg(par);
	  draw(draw_menu);
    elseif m ~= nil then
		sub_menu(m);
    end
  end
  if(b & rpg) == 0 then
    if act ~= nil and act.rpg ~= nil then
	  act.rpg(par);
	  draw(draw_menu);
    elseif m ~= nil then
		sub_menu(m);
    end
  end
  if(b & ekey) == 0 then
  end
end




self = {
    new_menu=new_menu,

    menu_up=menu_up,

    menu_down=menu_down,

    menu_pos=menu_pos,
    
    main_loop=function(m)
	    if m then
		new_menu(m);
	    end
	    tloop.run(gui_controller);
	end,
};


return self;
