#!/usr/bin/env lua

local db = require "tek.lib.debug"
local ui = require "tek.ui"
local rdargs = require "tek.lib.args".read

-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

function main(arg)

	local ARGTEMPLATE = "FILE/M,-f=FULLSCREEN/S,-w=WIDTH/N,-h=HEIGHT/N,-ss=SOFTSCROLL/S,-fs=FONTSIZE/N,-vd=VISUALDEBUG/S,-nbc=NOBLINK/S,-cm=CURSORMODE,-fn=FONTNAME,-c=CENTER/S,--help=HELP/S"

	local args = rdargs(ARGTEMPLATE, arg)
	if not args or args.help then
		print(ARGTEMPLATE)
		return
	end

	local app = ui.Application:new
	{
		AuthorStyleSheets = "editor"
	}
	
	local editwindow = ui.EditWindow:new
	{
		Title = "Editor.lua",
		FullScreen = args.fullscreen,
		Center = args.center,
		FileName = args.file,
		FontName = args.fontname,
		FontSize = args.fontsize,
		DragScroll = args.softscroll and 3 or false,
		HardScroll = not args.softscroll,
		BlinkCursor = not args.noblink,
		CursorStyle = args.cursormode or "bar+line",
	}

	app:addMember(editwindow)

	while editwindow.Running do

		editwindow.Width = args.width or editwindow.FullScreen and 800
		editwindow.Height = args.height or editwindow.FullScreen and 600

		editwindow:setValue("Status", "show")
		
		app:show()
		
		if args.visualdebug then
			editwindow.Drawable:setAttrs { Debug = true }
		end
		
		app:run()
		app:hide()
	end

	app:cleanup()
	
end

-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

main(arg)
