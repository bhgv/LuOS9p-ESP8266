
local ui = require "tek.ui"

--print(ui.ProgDir)


local ico_new = ui.loadImage("conf/icons/new32.ppm")
local ico_open = ui.loadImage("conf/icons/open32.ppm")
local ico_save = ui.loadImage("conf/icons/save32.ppm")
--local ico_open = ui.loadImage("conf/gui/back1.png")
--local ico_open = ui.getStockImage("file")
--local ico_open = ui.getStockImage("arrowup")


return ui.Group:new
{
  Orientation = "horisontal",
  Children = 
  {
    ui.ImageWidget:new 
    {
	Image = ico_new,
	Width = 32,
	Height = 32,
	Mode = "button",
	Style = "padding: 2",
	onClick = function(self)
		ui.ImageWidget.onClick(self)
--		self:setValue("Image", self.Pressed and RadioImage2 or RadioImage1)
	end
    },
    ui.ImageWidget:new 
    {
	Image = ico_open,
	Width = 32,
	Height = 32,
	Mode = "button",
	Style = "padding: 2",
	onClick = function(self)
		ui.ImageWidget.onClick(self)
--		self:setValue("Image", self.Pressed and RadioImage2 or RadioImage1)

		local app = self.Application
		app:addCoroutine(function()
				--    local pathfield = app:getById("pathfield")
				--    local filefield = app:getById("filefield")
				--    local statusfield = app:getById("statusfield")
            --local gparser = require "gcodeparser"
				    local status, path, select = app:requestFile
				    {
				--	BasePath = app:getById("basefield"):getText(),
					Path = "/home/orangepi/el", --pathfield:getText(),
					SelectMode = --app:getById("multiselect").Selected and
				--		    "multi",
				--		    or 
						    "single",
					DisplayMode = --app:getById("selectmode-all").Selected and
						    "all" 
				--		    or "onlydirs"
				    }
				    if status == "selected" then
              GFNAME = path .. "/" .. select[1]
              app:getById("status main"):setValue("Text", "Opening " .. GFNAME)
              --print(status, path, table.concat(select, ", "))
              --print(GFNAME)
              --print(io.open(GFNAME, "r"))
              local f = io.open(GFNAME, "r")
              if f ~= nil then
                local txt = f:read("*a")
                --print(txt)
                f:close()
                local o = gparser:do_parse(txt)
                --print (#o, o)
                for i = 1,#o do
                  print(i .. ") ", o[i])
                end
              end
              app:getById("status main"):setValue("Text", GFNAME)
				--	pathfield:setValue("Text", path)
				--	app:getById("filefield"):setValue("Text",
				--		    table.concat(select, ", "))
				    end
				end 
		)
	end
    },
    ui.ImageWidget:new 
    {
	Image = ico_save,
	Width = 32,
	Height = 32,
	Mode = "button",
--	Class = "button",
	Style = "padding: 2",
--	ImageAspectX = 2,
--	ImageAspectY = 3,
	onClick = function(self)
--		ui.ImageWidget.onClick(self)
--		self:setValue("Image", self.Pressed and RadioImage2 or RadioImage1)
	end
    },
  }
}

