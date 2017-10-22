return {
  acts={
    lft=function(k) gpio[k]=true end, 
    rgt=function(k) gpio[k]=false end, 
    lpg=function(k) gpio[k]=true end, 
    rpg=function(k) gpio[k]=false end, 
    ok=function(k) gui.gotop(); end, 
  },

  ind=function(y, k) 
    if gpio[k] then
	s="Off";
    else
	s="On";
    end
    oled.print(100, y, s ); 
  end
}
