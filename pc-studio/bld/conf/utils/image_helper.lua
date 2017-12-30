
local ui = require "tek.ui"



return {
  image = nil,
  pixmap = nil,
  W = nil,
  H = nil,
  isAlpha = nil,

  loadImage = function(self, path)
    local im = ui.loadImage(path)
    if not im then 
      self.pixmap = nil
      self.image = nil
      self.W, self.H, self.isAlpha = nil, nil, nil

      return nil 
    end
    
    local px = im:getPixmap()
    self.pixmap = px
    self.image = im

    self.W, self.H, self.isAlpha = px:getAttrs()
    
    return im
  end,

  getPixel = function(self, x, y)
    local px = self.pixmap
    if 
        (not (x and y and px)) or
        x >= self.W or y >= self.H
    then 
      return nil 
    end
    
    return px:getPixel(x, y)
  end,

  setPixel = function(self, x, y, clr)
    local px = self.pixmap
    if 
        (not (x and y and clr and px)) or
        x >= self.W or y >= self.H
    then 
      return nil 
    end

    return px:setPixel(x, y, clr)
  end,

}
