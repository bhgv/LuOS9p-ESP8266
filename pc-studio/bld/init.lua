
db = require "tek.lib.debug"
--db.level = db.INFO

Log = require "conf.log"

--rs232 = require('periphery').Serial

--PORT = nil

--require "ed"


MK = require "conf.controllers"


--ui = require "tek.ui"
--Visual = require "tek.lib.visual" --ui.loadLibrary("visual", 4)
--print ("Visual", Visual)


--Sender = require("conf.sender")
--Sender:start()

local PWD = os.getenv("PWD") or "."

Flags = {
  Home_path = PWD,
  Plugins_path = PWD .. "/conf/plugins",
  Plugins = {
    Groups = {
      Stuff = {},
    },
  },
  
  Transformations = {
    CurOp = "none",
    Move = {x=0, y=0, z=0},
    Rotate = 0,
    Scale = {x=1.0, y=1.0, z=1.0},
    Mirror = {h=false, v=false},
  },
  
  DispScale = 100,
  AutoRedraw = true,
  DisplayMode = "drag",
  DisplayProection = "xy",
  screenShift = {x=0, y=0,},

  isEdited = false,
}

Plugins = {
  Gui = {
    Headers = {},
    PlugPars = {},
  },
}

GFilters = {
}

--local plugin_sys = require "conf.utils.plugin_system_engine"

--plugin_sys:collect_plugins()


GUI = require "conf.gui"

