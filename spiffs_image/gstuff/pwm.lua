return {
  acts={
    lft=function(k) pwm[k]=pwm[k]-1 end, 
    rgt=function(k) pwm[k]=pwm[k]+1 end, 
    lpg=function(k) pwm[k]=pwm[k]-10 end, 
    rpg=function(k) pwm[k]=pwm[k]+10 end, 
  },

  ind=function(d, y, k) 
    d.print(100, y, string.format("%d%%", math.ceil(pwm[k]) ) ); 
  end
}