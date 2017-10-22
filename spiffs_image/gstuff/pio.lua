return {
  acts={
    lft=function(k) gpio[k]=true end, 
    rgt=function(k) gpio[k]=false end, 
    lpg=function(k) gpio[k]=true end, 
    rpg=function(k) gpio[k]=false end, 
    ok=function(k) top_menu(); end, 
  },

  ind=function(d, y, k) 
    if gpio[k] then
	s="Off";
    else
	s="On";
    end
    d.print(100, y, s ); 
  end
}
