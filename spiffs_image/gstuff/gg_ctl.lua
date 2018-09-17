return {
  acts_uni={
    lft=function(k) k=k-1; end, 
    rgt=function(k) k=k+1; end, 
    lpg=function(k) k=k-10; end, 
    rpg=function(k) k=k+10; end, 
    ok=function(k) gui.gotop(); end, 
  },

  acts_light={
    ekey=function(k)  end, 
    ok=function(k) gui.gotop(); end, 
  },

  ind=function(y, k, l) 
    oled.print(94, y, string.format("%.1f%s", k, l ) ); 
  end
}
