#!/usr/bin/env lua

-- 
--	linux keymap converter
--	note: tested with some i386 keymaps only
-- 

-------------------------------------------------------------------------------

local qualifiers =
{
	[0] = "plain",
	[1] = "shift",
	[2] = "altgr",
	[4] = "control",
-- 	[8] = "alt",
	[16] = "shiftl",
	[32] = "shiftr",
	[64] = "ctrll",
	[128] = "ctrlr",
	[256] = "caps",
	plain = 0,
	shift = 1,
	altgr = 2,
-- 	control = 4,
	alt = 2,		-- !!
	shiftl = 16,
	shiftr = 32,
-- 	ctrll = 64,
-- 	ctrlr = 128,
	caps = 256,
}

local qualnames =
{
	[0] = "0",
	[1] = "TKEYQ_SHIFT",
	[2] = "TKEYQ_RALT",
	[4] = "TKEYQ_CTRL",
	[8] = "TKEYQ_LALT",
	[16] = "TKEYQ_LSHIFT",
	[32] = "TKEYQ_RSHIFT",
	[64] = "TKEYQ_LCTRL",
	[128] = "TKEYQ_RCTRL",
}

local function gettekqualname(qualnr, keycode)
	if keycode == 42 then
		return "TKEYQ_LSHIFT"
	elseif keycode == 54 then
		return "TKEYQ_RSHIFT"
	elseif keycode == 29 then
		return "TKEYQ_LCTRL"
	elseif keycode == 97 then
		return "TKEYQ_RCTRL"
	elseif keycode == 56 then
		return "TKEYQ_LALT"
	elseif keycode == 100 then
		return "TKEYQ_RALT"
	end
	return qualnames[qualnr]
end


local keycodes = 
{
	Escape = "TKEYC_ESC",
	Up = "TKEYC_CRSRUP",
	Down = "TKEYC_CRSRDOWN",
	Left = "TKEYC_CRSRLEFT",
	Right = "TKEYC_CRSRRIGHT",
	PageUp = "TKEYC_PAGEUP",
	PageDown = "TKEYC_PAGEDOWN",
	scroll_forward = "TKEYC_PAGEDOWN",
	Insert = "TKEYC_INSERT",
	Pause = "TKEYC_PAUSE",
	Return = "TKEYC_RETURN",
	Home = "TKEYC_POSONE",
	End = "TKEYC_POSEND",
	Tab = "TKEYC_TAB",
	Delete = "TKEYC_BCKSPC",		--!!
	Remove = "TKEYC_DEL",
	
-- 	Compose = "TKEYC_PRINT",		--print/s-abf?
-- 	BackSpace = "TKEYC_BCKSPC",	--?
-- 	Break = ,						--?
-- 	Scroll_Backward = ,	-- shift+pageup
-- 	Scroll_Forward = ,	-- shift+pagedown
	
	F1 = "TKEYC_F1",
	F2 = "TKEYC_F2",
	F3 = "TKEYC_F3",
	F4 = "TKEYC_F4",
	F5 = "TKEYC_F5",
	F6 = "TKEYC_F6",
	F7 = "TKEYC_F7",
	F8 = "TKEYC_F8",
	F9 = "TKEYC_F9",
	F10 = "TKEYC_F10",
	F11 = "TKEYC_F11",
	F12 = "TKEYC_F12",
	
	space = " ",

	exclam = "!",
	quotedbl = '"',
	twosuperior = "²",
	section = "§",
	threesuperior = "³",
	dollar = "$",
	percent = "%",
	ampersand = "&",
	slash = "/",
	braceleft = "{",
	parenleft = "(",
	bracketleft = "[",
	parenright = ")",
	bracketright = "]",
	equal = "=",
	braceright = "}",
	question = "?",
	backslash = "\\",
	apostrophe = "'",
	comma = ",",
	semicolon = ";",
	period = ".",
	colon = ":",
	minus = "-",
	underscore = "_",
	division = "÷",
	multiplication = "×",
	
	periodcentered = "·",
-- 	ordfeminine = "ħ",
-- 	masculine = "",
	
	abovedot = "·",
	acute = "´",
	doubleacute = "˝",
	grave = "`",
	breve = "˘",
	caron = "ˇ",
	cedilla = "¸",
	circumflex = "ˆ",
	diaeresis = "¨",
	ogonek = "˛",

	at = "@",
	euro = "€",
	currency = "€",
	plus = "+",
	asterisk = "*",
	asciitilde = "~",
	asciicircum = "^",
	degree = "°",
	numbersign = "#",
	cent = "¢",
	mu = "µ",
	less = "<",
	greater = ">",
	bar = "|",
	onehalf = "½",
	exclamdown = "¡",
	questiondown = "¿",
	tilde = "~",
	plusminus = "±",
	
	copyright = "©",
	registered = "®",
	paragraph ="¶",
	
	dead_acute = "´",
	dead_caron = "ˇ",
	dead_grave = "`",
	dead_circumflex = "^",
	dead_tilde = "~",
	dead_diaeresis = "¨",
	guillemotleft = "«",
	guillemotright = "»",
	
	zero = "0",
	one = "1",
	two = "2",
	three = "3",
	four = "4",
	five = "5",
	six = "6",
	seven = "7",
	eight = "8",
	nine = "9",
	
	KP_0 = "0",
	KP_1 = "1",
	KP_2 = "2",
	KP_3 = "3",
	KP_4 = "4",
	KP_5 = "5",
	KP_6 = "6",
	KP_7 = "7",
	KP_8 = "8",
	KP_9 = "9",
	KP_Subtract = "-",
	KP_Add = "+",
	KP_Comma = ",",
	KP_Divide = ",",
	KP_Comma = ",",
	KP_Period = ".",
	KP_Enter = "TKEYC_RETURN",
	KP_Multiply = "*",
	
	Shift = 1,
	AltGr = 2,
-- 	Control = 4,
	Alt = 2,
	
	q = "q",
	w = "w",
	e = "e",
	r = "r",
	t = "t",
	z = "z",
	u = "u",
	i = "i",
	o = "o",
	p = "p",
	a = "a",
	s = "s",
	d = "d",
	f = "f",
	g = "g",
	h = "h",
	j = "j",
	k = "k",
	l = "l",
	y = "y",
	x = "x",
	c = "c",
	v = "v",
	b = "b",
	n = "n",
	m = "m",
	
	Q = "Q",
	W = "W",
	E = "E",
	R = "R",
	T = "T",
	Z = "Z",
	U = "U",
	I = "I",
	O = "O",
	P = "P",
	A = "A",
	S = "S",
	D = "D",
	F = "F",
	G = "G",
	H = "H",
	J = "J",
	K = "K",
	L = "L",
	Y = "Y",
	X = "X",
	C = "C",
	V = "V",
	B = "B",
	N = "N",
	M = "M",
	
	aacute = "á",	
	Aacute = "Á",	
	adiaeresis = "ä",
	Adiaeresis = "Ä",
	ae = "æ",
	AE = "Æ",
	agrave = "à",
	Agrave = "À",
	aogonek = "ą",
	Aogonek = "Ą",
	aring = "å",
	Aring = "Å",
	cacute = "ć",	
	Cacute = "Ć",	
	ccaron = "č",
	Ccaron = "Č",
	ccedilla = "ç",
	Ccedilla = "Ç",
	dstroke = "đ",
	Dstroke = "Đ",
	eacute = "é",	
	Eacute = "É",	
	egrave = "è",	
	Egrave = "È",	
	eogonek = "ę",
	Eogonek = "Ę",
	eth = "ð",
	ETH = "Ð",
	iacute = "í",	
	Iacute = "Í",	
	idiaeresis = "ï",
	Idiaeresis = "Ï",
	igrave = "ì",
	Igrave = "Ì",
	lstroke = "ł",
	Lstroke = "Ł",
	macron = "¯",
	nacute = "ń",
	Nacute = "Ń",
	ntilde = "ñ",
	Ntilde = "Ñ",
	oacute = "ó",	
	Oacute = "Ó",
	ooblique = "ø",
	Ooblique = "Ø",
	odiaeresis = "ö",
	Odiaeresis = "Ö",
	odoubleacute = "ő",	
	Odoubleacute = "Ő",	
	oslash = "ø",
	sacute = "ś",
	Sacute = "Ś",
	scaron = "š",
	Scaron = "Š",
	ssharp = "ß",
	sterling = "£",
	thorn = "þ",
	THORN = "Þ",
	uacute = "ú",	
	Uacute = "Ú",
	ucircumflex = "û",
	Ucircumflex = "Û",
	udiaeresis = "ü",
	Udiaeresis = "Ü",
	udoubleacute = "ű",	
	Udoubleacute = "Ű",
	ugrave = "ù",	
	Ugrave = "Ù",
	zabovedot = "ż",
	Zabovedot = "Ż",
	zacute = "ź",
	Zacute = "Ź",
	zcaron = "ž",
	Zcaron = "Ž",
	
	[0] = 0,
}

-------------------------------------------------------------------------------

function out(...)
	io.stdout:write(...)
end

local function _utf8values(readc, data)
	local accu = 0
	local numa = 0
	local min, buf
	local i = 0

	return function()
		local c
		while true do
			if buf then
				c = buf
				buf = nil
			else
				c = readc(data)
			end
			if not c then
				return
			end
			if c == 254 or c == 255 then
				break
			end
			if c < 128 then
				if numa > 0 then
					buf = c
					break
				end
				i = i + 1
				return i, c
			elseif c < 192 then
				if numa == 0 then break end
				accu = accu * 64 + c - 128
				numa = numa - 1
				if numa == 0 then
					if accu == 0 or accu < min or
						(accu >= 55296 and accu <= 57343) then
						break
					end
					c = accu
					accu = 0
					i = i + 1
					return i, c
				end
			else
				if numa > 0 then
					buf = c
					break
				end
				if c < 224 then
					min = 128
					accu = c - 192
					numa = 1
				elseif c < 240 then
					min = 2048
					accu = c - 224
					numa = 2
				elseif c < 248 then
					min = 65536
					accu = c - 240
					numa = 3
				elseif c < 252 then
					min = 2097152
					accu = c - 248
					numa = 4
				else
					min = 67108864
					accu = c - 252
					numa = 5
				end
			end
		end
		accu = 0
		numa = 0
		return 65533 -- bad character
	end
end

local function utf8values(text)
	local i = 0
	return _utf8values(function() 
		i = i + 1
		return text:byte(i)
	end)
end

local function utf8len(text)
	local len = 0
	local c0
	if text then
		for pos, c in utf8values(text) do
			if not c0 then
				c0 = c
			end
			len = len + 1
		end
	end
	return len, c0
end

-------------------------------------------------------------------------------

local function printerr(...)
	io.stderr:write(...)
	io.stderr:write("\n")
end

local function openfile(fname)
	if fname:match("%.gz$") then
		fname = fname:match("^(.*)%.gz$")
	end
	local fd, msg
	for i = 1, 2 do
		fd, msg = io.open(fname)
		if fd and fname:match("%.gz$") then
			fd:close()
			fd, msg = io.popen("zcat '"..fname.."'")
		end
		if fd then
			break
		end
		fname = fname .. ".gz"
	end
	return fd, msg
end

local readlines

local inclnames = { "%s", "%s.inc", "include/%s", "include/%s.inc" }

local function processfile(path, incl, state)
	while true do
		path = path:match("^(.-)/*$")
		for i = 1, #inclnames do
			local pat = inclnames[i]:format(incl)
			local fname = path == "" and pat or path .. "/" .. pat
			local fd, msg = openfile(fname)
			if fd then
				readlines(fd, fname, state)
				fd:close()
				return
			end
		end
		if path == "" then
			break
		end
		path, part = path:match("^(.-/?)([^/]*)$")
	end
	printerr(">>> could not open ", incl)
	return os.exit(1)
end

local function doline(l, fname, state)
	local incl = l:match('^include%s+"(.*)"$')
	if incl then
		local path, part = fname:match("^(.-/?)([^/]*)$")
		processfile(path, incl, state)
		return
	end
	if l:match("^strings as usual$") then
		printerr(">>> strings as usual")
		return
	end
	if l:match("^alt_is_meta$") then
		printerr(">>> alt_is_meta")
		return
	end
	
	local string, text = l:match('^string%s+(.*)%s*=%s*"(.*)"$')
	if string then
		printerr(">>> string: ", string, " = ", text)
		return
	end
	
	local keymaps = l:match("^keymaps%s+(.*)$")
	if keymaps then
		local column = 0
		local qual_of_column = { }
		local column_of_qual = { }
		for s in keymaps:gmatch("([^,]+),?") do
			local from, to = s:match("^(%d+)%-(%d+)$")
			if from then
				for i = from, to do
					qual_of_column[column] = i
					column_of_qual[i] = column
					column = column + 1
				end
			else
				local i = tonumber(s)
				qual_of_column[column] = i
				column_of_qual[i] = column
				column = column + 1
			end
		end
		state.qual_of_column = qual_of_column
		state.column_of_qual = column_of_qual
		printerr(">>> keymaps: ", keymaps, " maxcolumn: ", #state.qual_of_column)
		return
	end

	local charset = l:match('^charset%s*"(.*)"$')
	if charset then
		printerr(">>> charset: ", charset)
		return
	end
	
	local qual, keycode, syms = l:match("^(.-)%s*keycode%s+(%d+)%s*=%s*(.*)$")
	if keycode then
		keycode = tonumber(keycode)
		if qual ~= "" then -- have qualifier
			local qval = 0
			local quals = { }
			for q in qual:gmatch("%S+") do
				local qcode = qualifiers[q:lower()]
				if not qcode then
					return
				end
				assert(qcode, "unknown qualifier '" .. q .. "'")
				table.insert(quals, qcode)
				qval = qval + qcode
			end
			local key = state.map[keycode] or { }
			key[qval] = { sym=syms, quals = quals }
			state.map[keycode] = key
			return
		end
		local cmn = 0
		for sym in syms:gmatch("%S+") do
			local qval = state.qual_of_column[cmn]
			local key = state.map[keycode] or { }
			key[qval] = { sym=sym, quals = { qval } }
			state.map[keycode] = key
			cmn = cmn + 1
		end
		return
	end
	if l:match("^compose") then
-- 		printerr(">>> compose: ", l)
		return
	end
	printerr(">>> unhandled: ", l)
end

function readlines(fd, fname, state)
	local multiline
	for l in fd:lines() do
-- 		printerr(l)
		l = l:match("^([^#!]*)[#!]?.*$")
		l = l:match("^%s*(.-)%s*$")
		if multiline then
			l = multiline .. l
		end
		if l ~= "" then
			local lm = l:match("(.*)\\$")
			if lm then
				multiline = lm
			elseif l ~= "" then
				if multline then
					l = multiline
				end
				multiline = nil
				doline(l, fname, state)
			end
		end
	end
end

local function getkeycode(keyc)
	if keyc ~= 0 then
		local utf8hex = keyc:match("^U%+(%x+)$")
		if utf8hex then
			keyc = tonumber(utf8hex, 16)
		elseif keyc:match("^0x") then
			keyc = tonumber(keyc, 16)
		else
			keyc = keycodes[keyc]
			local len, code = utf8len(keyc)
			if len == 1 then
				keyc = code
			end
		end
	end
	return keyc
end

local function dump(state)
	
	out("struct RawKeyInit { TUINT index; struct RawKey rawkey; } rawkeyinit[] =\n")
	out("{\n")
	
	for kc, rec in pairs(state.map) do
		
		for i = 0, 511 do
			local res = rec.result or { qual = 0, keyc = 0, { } }
			rec.result = res
			if rec[i] then
				local sym = rec[i].sym --:lower()
				local qual = rec[i].quals
				assert(qual)
				local quals = table.concat(qual, ",")
				local plussym = sym:match("^+(.*)$")
				if plussym then
					sym = plussym
				end
				if qualifiers[sym:lower()] then
					-- primary sym _is_ qualifier
					assert(i == 0)
					assert(keycodes[sym], sym)
					res.qual = keycodes[sym]
				elseif i == 0 then
					res.qual = 0
					res.keyc = sym
					if sym:match("^[a-z]$") then
						local qrec = { keyc = sym:upper(), qualnames = qualnames[1] }
						res[1][1] = qrec
					end
				else
					local qstring = { }
					for i = 1, #qual do
-- 						table.insert(qstring, qualnames[qual[i]])
						local qname = gettekqualname(qual[i], kc)
						table.insert(qstring, qname)
					end
					local qrec = { keyc = sym, qualnames = table.concat(qstring, "|") }
					table.insert(res[1], qrec)
				end
			end
		end

		local res = rec.result
		out("\t{ ", kc, ", { ")
			
-- 		out(qualnames[res.qual], ", ")
		local qname = gettekqualname(res.qual, kc)
		out(qname, ", ")
		
		local keyc = getkeycode(res.keyc)
		out(keyc or 0, " /* ", res.keyc, " */, { ")
		for i = 1, #res[1] do
			local qrec = res[1][i]
			local qkc = getkeycode(qrec.keyc)
			if qkc then
				out("{ ", qrec.qualnames, ", ")
				out(qkc, " /* ", qrec.keyc, " */ }, ")
			else
				state.unknown[qrec.keyc] = qrec.keyc
			end
		end
		out("} } },\n")
		
	end
	out("};\n")
	
	local t = { }
	for u in pairs(state.unknown) do
		table.insert(t, u)
	end
	table.sort(t)
	printerr(">>> unknown: ", table.concat(t, " "))
end

local function main(arg)
	local fd = arg[1] and openfile(arg[1])
	if not fd then
		printerr("could not open file.")
		printerr("example usage:")
		printerr("$ keymap2c.lua /usr/share/keymaps/i386/qwerty/pl.map")
		printerr("redirect output to e.g. src/display_rawfb/keymap.c")
		return os.exit(1)
	end
	local state = { qual_of_column = { }, column_of_qual = { }, map = { }, unknown = { } }
	for i = 0, 511 do
		state.qual_of_column[i] = i
		state.column_of_qual[i] = i
	end
	readlines(fd, arg[1], state)
	fd:close()
	printerr(">>> done.")
	
	dump(state)
	
end

main(arg)
