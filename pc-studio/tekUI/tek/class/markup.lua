-------------------------------------------------------------------------------
--
--	tek.class.markup
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	OVERVIEW::
--		[[#ClassOverview]] :
--		[[#tek.class : Class]] / Markup ${subclasses(Markup)}
--
--		This class implements a markup parser and converter for producing
--		feature-rich XHTML 1.0 from plain text with special formattings.
--
--	IMPLEMENTS::
--		- Markup.new()
--		- Markup:run()
--
-------------------------------------------------------------------------------

--
--	FORMAT DESCRIPTION::
--
--	==== Block ====
--
--	Text of a consistent indentation level running uninterrupted by empty
--	lines is combined into ''blocks''. Lines in blocks can break at any
--	position; the line breaks are not included in the result:
--
--		This is a block.
--
--		This is another
--		block.
--
--	==== Indentation ====
--
--	The ''indentation depth'' is measured in the consecutive number of
--	''indentation characters'' (tabs or spaces, which may depend on
--	the application) at the beginning of a line; different indentation
--	levels are taken into account accordingly:
--
--	This is a block.
--
--		This is another
--		block.
--
--			This is a third
--			block.
--
--	==== Preformatted ====
--
--	For blocks of code and other kinds of ''preformatted text'' use an
--	indentation that is ''two levels deeper'' than the current level,
--	e.g.:
--
--	Normal text
--
--			/* Code markup */
--
--	Back to normal
--
--	==== Code ====
--
--	Inlined ''code'' is marked up between double braces. It may occur at
--	any position in a line, but it must be the same line in which the
--	opening and the closing of the markup occurs:
--
--	This is {{code_markup}} in running text.
--
--	==== Lists ====
--
--	There are two types of items in ''lists''; ''soft'' and ''bulleted''
--	items. They are recognized by their initiatory character (dash or
--	asterisk), followed by a whitespace at the beginning of a line:
--
--		- soft item
--		* bulleted item
--
--	Soft items are, by the way, a natural means to enforce line breaks:
--
--		- this is a line,
--		- this is another line,
--		- this is a third line.
--
--	Lists follow the same indentation rules as normal text:
--
--		* one
--		* two
--		* three
--			* eins
--			* zwei
--				- ichi
--				- ni
--				- san
--			* drei
--
--	'''NOTE''': Although not strictly required, it is recommended to
--	indent lists by one level. This will help the parser to avoid
--	ambiguities; otherwise, when a regular block follows an unindented
--	list, it would be concatenated with the last item, as empty lines are
--	not sufficient to break out from a list.
--
--	==== Links ====
--
--	Links are enclosed in double squared brackets, according to the
--	template {{[[description][link target]]}}, where {{[description]}}
--	is optional, e.g.:
--
--		* [[home]] -
--		Internal link: Link target is description at the same time.
--
--		* [[Home page][home]] -
--		Internal link with alternate description
--
--		* [[http://www.foo.bar/]] -
--		External link
--
--		* [[You know what cool is][http://www.foo.bar/]] -
--		External link with alternate description
--
--	==== Tables ====
--
--	Lines running uninterrupted with at least one cell separator in each
--	of them automatically form a table. The cell separator is a double
--	vertical bar:
--
--		First cell || Second cell
--		Third cell || Fourth cell
--
--	It is also possible to create empty cells as long as the separators
--	are present. Note, by the way, that cell separators do not
--	necessarily have to be aligned exactly below each other.
--
--	==== Headings ====
--
--	Headings occupy an entire line. They are enclosed by at least one
--	equal sign and a whitspace on each side; the more equal signs, the
--	less significant the section:
--
--		= Heading 1 =
--
--		== Heading 2 ==
--
--		=== Heading 3 ===
--
--	==== Rules ====
--
--	A minimum of four dashes (or equal signs) is interpreted as a horizontal
--	rule. Rules may occur at arbitrary indentation levels, but otherwise they
--	must occupy the whole line:
--
--		before the rule
--		----------------------------------------------
--		after the rule
--
--	A rule comprised of equal signs will be of the "page-break" class; it is
--	up to a browser (or downstream parser) to visualize this in a useful way.
--
--		a rule indicating a page break:
--		==============================================
--
--	==== Emphasis ====
--
--	An emphasized portion of text may occur at any position in a line,
--	but the opening and the closing of the markup must occur in the same
--	line to be recognized.
--
--	The emphasized text is surrounded by at least two ticks on each side;
--	the more ticks, the stronger the emphasis.
--
--		- normal
--		- ''emphasis''
--		- '''strong emphasis'''
--		- ''''very strong emphasis''''
--
--	==== Nodes ====
--
--	The possible templates for nodes are
--		- {{==( link target )==}}
--		- {{==( link target : description )==}}
--	''Nodes'' translate to anchors in a document, and a browser will
--	usually interprete them as jump targets. Visually, nodes translate
--	to headlines also; as usual, the number of equal signs with which
--	the heading is paired determines the significance and therefore
--	size of a headline, e.g.:
--
--		=( Large )=
--		==( Medium )==
--		===( Small : Small node with alternate text )===
--
--	In addition to normal node headlines, there is also a variant which
--	is marked up as code, which is indicated by braces instead of
--	parantheses. This is especially useful for jump targets in technical
--	documentation:
--
--		={ LargeCode : Large code node }=
--		=={ MediumCode : Medium code node }==
--		==={ SmallCode : Small code node }===
--
--	When referencing a node, use a number sign {{#}} followed by the
--	node name. This, by the way, also works when referencing nodes in
--	other documents.
--
--	'''NOTE''': Don't be too adventurous when selecting a node name;
--	if you need blanks or punctuation or if in doubt use the alternate
--	text notation.
--

local Class = require "tek.class"

local type = type
local insert = table.insert
local remove = table.remove
local concat = table.concat
local min = math.min
local max = math.max
local char = string.char
local stdin = io.stdin
local stdout = io.stdout
local ipairs = ipairs

local Markup = Class.module("tek.class.markup", "tek.class")
Markup._VERSION = "Markup 3.3"

-------------------------------------------------------------------------------
--	iterate over lines in a string
-------------------------------------------------------------------------------

local function rd_string(s)
	local pos = 1
	return function()
		if pos then
			local a, e = s:find("[\r]?\n", pos)
			local o = pos
			if not a then
				pos = nil
				return s:sub(o)
			else
				pos = e + 1
				return s:sub(o, a - 1)
			end
		end
	end
end

-------------------------------------------------------------------------------
--	utf8values: iterator over UTF-8 encoded Unicode chracter codes
-------------------------------------------------------------------------------

local function utf8values(s)

	local readc
	local i = 0
	if type(s) == "string" then
		readc = function()
			i = i + 1
			return s:byte(i)
		end
	else
		readc = function()
			local c = s:read(1)
			return c and c:byte(1)
		end
	end

	local accu = 0
	local numa = 0
	local min, buf

	return function()
		local c
		while true do
			if buf then
				c = buf
				buf = nil
			else
				c = readc()
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
				return c
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
					return c
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

-------------------------------------------------------------------------------
--	encodeform: encode for forms (display '<', '>', '&', '"' literally)
-------------------------------------------------------------------------------

function Markup:encodeform(s)
	local tab = { }
	if s then
		for c in utf8values(s) do
			if c == 34 then
				insert(tab, "&quot;")
			elseif c == 38 then
				insert(tab, "&amp;")
			elseif c == 60 then
				insert(tab, "&lt;")
			elseif c == 62 then
				insert(tab, "&gt;")
			elseif c == 91 or c == 93 or c > 126 then
				insert(tab, ("&#%03d;"):format(c))
			else
				insert(tab, char(c))
			end
		end
	end
	return concat(tab)
end

-------------------------------------------------------------------------------
--	encodeurl: encode string to url; optionally specify a string with a
--	set of characters that should be left unmodified, from: $&+,/:;=?@
-------------------------------------------------------------------------------

local function encodefunc(c)
	return ("%%%02x"):format(c:byte())
end

function Markup:encodeurl(s, excl)
	-- reserved chars with special meaning:
	local matchset = "$&+,/:;=?@"
	if excl == true then
		matchset = ""
	elseif excl and type(excl) == "string" then
		matchset = matchset:gsub("[" .. excl:gsub(".", "%%%1") .. "]", "")
	end
	-- unsafe chars are always substituted:
	matchset = matchset .. '"<>#%{}|\\^~[]`]'
	matchset = "[%z\001-\032\127-\255" .. matchset:gsub(".", "%%%1") .. "]"
	return s:gsub(matchset, encodefunc)
end

-------------------------------------------------------------------------------
--	get a SGML/XML tag
-------------------------------------------------------------------------------

function Markup:getTagML(id, tag, open)
	if tag then
		local tab = { tag }
		if id == "link" or id == "emphasis" or id == "code" then
			if not self.brpend and not self.inline then
				insert(tab, 1, ("\t"):rep(self.depth))
			end
			self.inline = true
			self.brpend = false
		else
			if id == "image" then
				if not self.inline then
					insert(tab, 1, ("\t"):rep(self.depth))
				end
				self.brpend = true
			else
				if id == "pre" or id == "preline" then
					self.brpend = false
				elseif open or id ~= "preline" then
					insert(tab, 1, ("\t"):rep(self.depth))
					if not open and self.inline then
						self.brpend = true
					end
				end
				if self.brpend then
					insert(tab, 1, "\n")
				end
				if id ~= "preline" or not open then
					insert(tab, "\n")
				end
				self.inline = false
				self.brpend = false
			end
		end
		return concat(tab)
	end
end

-------------------------------------------------------------------------------
--	get a SGML/XML line of text
-------------------------------------------------------------------------------

function Markup:getTextML(line, id)
	local tab = { }
	if self.brpend then
		insert(tab, "\n")
		insert(tab, ("\t"):rep(self.depth))
	else
		if id ~= "link" and id ~= "emphasis" and id ~= "code" then
			if not self.inline then
				insert(tab, ("\t"):rep(self.depth))
			end
		end
	end
	line:gsub("^(%s*)(.-)(%s*)$", function(a, b, c)
		if a ~= "" then
			insert(tab, " ")
		end
		insert(tab, b)
		if c ~= "" then
			insert(tab, " ")
		end
	end)
	self.brpend = true
	return concat(tab)
end

-------------------------------------------------------------------------------
--	definitions for output as XHTML 1.0 strict
-------------------------------------------------------------------------------

function Markup:out(...)
	self.wrfunc(concat { ... })
end

function Markup:init(docname)
	self.depth = 1
	
	local metadata = { }
	
	for _, k in ipairs { "author", "created" } do
		if self[k] then
			insert(metadata, [[
		<meta name="]] .. k .. [[" content="]] .. self[k] .. [[" />
]])
		end			
	end
	
	metadata = concat(metadata)
	
	return [[
<?xml version="1.0" encoding="utf-8" ?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
	"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en">
	<head>
		<title>]] .. docname .. [[</title>
		<link rel="stylesheet" href="manual.css" />
]] .. metadata .. [[
	</head>
	<body>
]]
end

function Markup:exit()
	return [[
	</body>
</html>
]]
end

function Markup:gettag(id, tag, open)
	return self:getTagML(id, tag, open)
end

function Markup:gettext(line, id)
	return self:encodeform(self:getTextML(line, id))
end

function Markup:getpre(line)
	return self:encodeform(line), '\n'
end

function Markup:getcode(line)
	return self:encodeform(line)
end

function Markup:head(level, text)
	return '<h' .. level .. '>', '</h' .. level .. '>'
end

function Markup:headnode(id, text, len, is_code)
	if is_code then
		return ('<div class="node"><h%d><a name="%s" id="%s"><code>%s</code></a></h%d>'):format(
			len, id, id, text, len), '</div>'
	else
		return ('<div class="node"><h%d><a name="%s" id="%s">%s</a></h%d>'):format(
			len, id, id, text, len), '</div>'
	end
end

function Markup:def(name)
	self.prebr = false
	return '<div class="definition"><dfn>' .. name .. '</dfn>', '</div>'
end

function Markup:indent()
	return '<blockquote>', '</blockquote>'
end

function Markup:list()
	return '<ul>', '</ul>'
end

function Markup:item(bullet)
	if bullet then
		return '<li>', '</li>'
	end
	return '<li style="list-style-type: none">', '</li>'
end

function Markup:block()
	return '<p>', '</p>'
end

function Markup:rule()
	return '<hr />'
end

function Markup:pagebreak()
	return '<hr class="page-break" />'
end

function Markup:pre()
	return '<pre>', '</pre>'
end

function Markup:code()
	return '<code>', '</code>'
end

function Markup:emphasis(len, text)
	if len == 1 then
		return '<em>', '</em>'
	elseif len == 2 then
		return '<strong>', '</strong>'
	else
		return '<em><strong>', '</strong></em>'
	end
end

function Markup:link(link)
	--local isurl = link:match("^%a*://.*$")
	local func = link:match("^(.*)%(%)$")
	if func then
		return '<a href="#' .. func .. '"><code>', '</code></a>'
	else
		local url, anch = link:match("^([^#]*#?)([^#]*)$")
		link = url .. anch:gsub("[^a-zA-Z%d%_%-%.%:]", "")
		return '<a href="' .. link .. '">', '</a>'
	end
end

function Markup:table(border)
	return '<table>', '</table>'
end

function Markup:row()
	self.column = 0
	return '<tr>', '</tr>'
end

function Markup:cell(border)
	self.column = self.column + 1
	return '<td class="column' .. self.column .. '">', '</td>'
end

function Markup:image(link, extra)
	return '<img src="' .. self:encodeurl(link, true) .. '" ' ..
		(extra or "") .. ' alt="image" />'
end

function Markup:document()
end

-------------------------------------------------------------------------------
--	run(): This function starts the conversion.
-------------------------------------------------------------------------------

function Markup:run()

	local function doid(id, ...)
		if self[id] then
			return self[id](self, ...)
		end
	end

	local function doout(id, ...)
		doid("out", doid(id, ...))
	end

	local function push(id, ...)
		local opentag, closetag = doid(id, ...)
		doout("gettag", id, opentag, true)
		insert(self.stack, { id = id, closetag = closetag })
		self.depth = self.depth + 1
	end

	local function pop()
		local e = remove(self.stack)
		if e then
			self.depth = self.depth - 1
			doout("gettag", e.id, e.closetag)
			return e.id
		end
	end

	local function top()
		local e = self.stack[#self.stack]
		if e then
			return e.id
		end
	end

	local function popuntil(...)
		local i
		repeat
			i = pop()
			for _, v in ipairs { ... } do
				if v == i then
					return
				end
			end
		until not i
	end

	local function popwhilenot(...)
		local i
		repeat
			i = top()
			for _, v in ipairs { ... } do
				if v == i then
					return
				end
			end
			pop()
		until not i
	end

	local function popwhile(...)
		local cont
		repeat
			local id = top()
			cont = false
			for _, v in ipairs { ... } do
				if v == id then
					local i = pop()
					cont = true
					break
				end
			end
		until not cont
	end

	local function popif(...)
		local i = top()
		for _, v in ipairs { ... } do
			if v == i then
				pop()
				return
			end
		end
	end

	local function checktop(...)
		local i = top()
		for _, v in ipairs { ... } do
			if v == i then
				return true
			end
		end
	end

	local function pushfragment(...)
		local line = concat { ... }
		if not self.context.fragments then
			self.context.fragments = { }
		end
		self.id = (self.id or 0) + 1
		insert(self.context.fragments, 1, { line = line, id = self.id })
		self.context.topid = self.id
	end

	local function popfragment()
		self.context.firstfragment = false
		local frag = remove(self.context.fragments, 1)
		if frag then
			self.line = frag.line
			if frag.id == self.context.topid then
				self.context.firstfragment = self.context.parentfirstfragment
			end
		else
			self.line = false
		end
		return self.line
	end

	local function pushcontext(id, line)
		insert(self.cstack, self.context)
		if id then
			self.context = { id = id, fragments = { } }
			pushfragment(line)
			insert(self.cstack, self.context)
		end
	end

	local function popcontext()
		self.context = remove(self.cstack) or false
		return self.context
	end

	-- parse

	self.lnr = 1
	self.previndent = 0
	self.stack = { }
	self.depth = 0
	self.in_table = false
	self.prebr = false
	self.is_dynamic_content = false

	doout("init", self.docname)
	push("document")

	local linematch = ("^(%s*)(.-)%%s*$"):format(self.indentchar)

	local feature = { }
	self.features:gsub("%a", function(f)
		feature[f] = true
	end)

	for line in self.rdfunc(self.input) do

		self.indentlevel = 0
		line = line:gsub(linematch, function(s, t)
			if t ~= "" then
				self.indentlevel = s:len()
			else
				self.indentlevel = self.previndent
			end
			return t
		end)

		if feature["t"] then
			self.istabline = line:find("||", 1, 1) or false
			if self.istabline then
				popif("block")
			end
			if self.in_table then
				popuntil("row")
				if not self.istabline then
					popuntil("table")
					self.in_table = false
				end
			end
		end

		if self.indentlevel < self.previndent then
			if not self.preindent or self.indentlevel < self.preindent then
				local i = self.indentlevel
				while i < self.previndent do
					i = i + 1
					if top() == "pre" then
						i = i + 1
					end
					popuntil("indent", "pre")
				end
				self.preindent = false
			end
		elseif self.indentlevel == self.previndent + 1 then
			popif("block")
			push("indent")
		elseif feature["p"] and self.indentlevel >= self.previndent + 2 then
			if not self.preindent then
				self.preindent = self.previndent + 2
				popif("block")
				push("pre")
			end
		end

		if not self.preindent then

			if feature["d"] then
				-- def ( SYNOPSIS )
				line = line:gsub("^(%u[%u%d%s_]+)::$", function(name)
					popwhile("def", "item", "list", "block", "pre")
					push("def", name)
					return ""
				end)
			end

			if feature["f"] then
				-- function ( INCLUDE(name) )
				line = line:gsub("^%s*(INCLUDE_?%a*)%(%s*(.*)%s*%)%s*$",
					function(key, line)
					if key == "INCLUDE" or key == "INCLUDE_STATIC" then
						local args = { }
						line:gsub(",?([^,]*)", function(a)
							a = a:match("^%s*(%S.-)%s*$")
							if a then
								insert(args, a)
							end
						end)
						popwhilenot("document")
						push("func", key == "INCLUDE", args)
						return ""
					end
				end)
				-- argument ( ======== )
				line = line:gsub("^%s*========+%s*$", function()
					popwhilenot("func")
					push("argument")
					return ""
				end)
			end

			if feature["n"] then
				-- headnode ( ==( id : text )== )
				line = line:gsub("^%s*(=+[%(%{])%s+(.+)%s+:%s+(.+)%s+([%)%}]=+)%s*$",
					function(s1, id, text, s2)
					local l = min(s1:len() - 1, s2:len() - 1, 5)
					popwhilenot("document")
					id = id:gsub("[^a-zA-Z%d%_%-%.%:]", "")
					push("headnode", id, text, l, s2:sub(1, 1) == "}")
					return ""
				end)
				-- headnode ( ==( text )== )
				line = line:gsub("^%s*(=+[%(%{])%s+(.*)%s+([%)%}]=+)%s*$",
					function(s1, text, s2)
					local l = min(s1:len() - 1, s2:len() - 1, 5)
					popwhilenot("document")
					local id = text:gsub("[^a-zA-Z%d%_%-%.%:]", "")
					push("headnode", id, text, l, s2:sub(1, 1) == "}")
					return ""
				end)
			end

			if feature["s"] then
				-- rule ( ----.. )
				line = line:gsub("^%s*(%-%-%-%-+)%s*$", function(s)
					popwhile("block", "list", "item")
					push("rule")
					pop()
					return ""
				end)
				line = line:gsub("^%s*(%=%=%=%=+)%s*$", function(s)
					popwhile("block", "list", "item")
					push("pagebreak")
					pop()
					return ""
				end)
			end

		end

		--

		self.cstack = { }
		self.context = { parentfirstfragment = true }
		pushfragment(line)
		pushcontext()

		if line == "" then
			popwhile("block")
		end

		while popcontext() do
			while popfragment() do
				line = self.line

				if self.preindent then

					if self.prepend then
						push("preline")
						doout("getpre", "")
						pop()
						self.prepend = false
					end

					if line == "" then
						self.prepend = true
					else
						push("preline")
						doout("getpre", (" "):rep(self.indentlevel -
							self.preindent) .. line)
						pop()
					end

				else
					self.prepend = false

					if line ~= "" then

						if feature["l"] then
							-- list/item ( * ... )
							local _, _, b, a = line:find("^([%*%-])%s+(.*)$")
							if b and a then
								local inlist = checktop("item", "list")
								popwhile("item", "block")
								if not inlist then
									push("list")
								end
								if b == "*" then
									push("item", true)
								else
									push("item")
								end
								pushcontext("item", a)
								break
							end
						end

						if feature["t"] then
							-- table cells
							local _, pos, a, b =
								line:find("^%s*(.-)%s*||%s*(.*)%s*$")
							if pos then
								if self.context.id == "table" then
									popuntil("cell")
								else
									self.context.id = "table"
									if not self.in_table then
										self.in_table = true
										push("table")
									end
									push("row")
								end
								pushfragment(b)
								push("cell")
								pushcontext("cell", a)
								break
							elseif self.context.id == "table" then
								popuntil("cell")
								push("cell")
								pushcontext("cell", line)
								break
							end
						end

						if feature["c"] then
							-- code
							local _, _, a, text, b =
								line:find("^(.-){{(.-)}}(.*)%s*$")
							if text then
								if a == "" then
									if not checktop("block", "item", "cell") then
										push("block")
									end
									if self.context.firstfragment then
										doout("gettext", "")
									end
									push("code")
									if b == "" then b = " " end
									pushfragment(b)
									pushcontext("code", text)
								else
									pushfragment("{{", text, "}}", b)
									pushfragment(a)
									pushcontext()
								end
								break
							end
						end

						if feature["e"] then
							-- emphasis
							local _, _, a, x, text, y, b =
								line:find("^(.-)(''+)(.-)(''+)(.*)$")
							if text then
								if a == "" then
									x, y = x:len(), y:len()
									local len = min(x, y, 4)
									if not checktop("block", "item", "cell") then
										push("block")
									end
									if self.context.firstfragment then
										doout("gettext", "")
									end
									push("emphasis", len - 1)
									if b == "" then b = " " end
									pushfragment(b)
									pushcontext("emphasis", text)
								else
									pushfragment(x, text, y, b)
									pushfragment(a)
									pushcontext()
								end
								break
							end
						end

						if feature["a"] then
							-- [[link]], [[title][link]], function(), [[link : title]]
							if self.context.id ~= "link"
								and self.context.id ~= "code" then
								local a, title, link, b
								a, link, title, b = -- [[link : title]]
									line:match("^(.-)%[%[([^%]]-)%s+:%s+([^%]]-)%]%](.*)%s*$")
								if not link then
									a, title, link, b = -- [[text][...]]
										line:match("^(.-)%[%[([^%]]-)%]%[([^%]]-)%]%](.*)%s*$")
								end
								if not link then -- [[....]]
									a, link, b = line:match("^(.-)%[%[([^%]]-)%]%](.*)%s*$")
									if link then
										title = link:match("^#?(.*)$")
									end
								end
								if not link then -- class:function()
									a, link, b = line:match("^(.-)(%a[%w_:.]-%(%))(.*)%s*$")
								end
								if not link then -- prot://foo/bar
									a, link, b = line:match(
										"^(.-)(%a*://[%w_%-%.,:;/%?=~]*)(.*)%s*$")
								end
								if link then
									if a == "" then
										if not checktop("block", "item", "cell") then
											push("block")
										end
										if self.context.firstfragment then
											doout("gettext", "")
										end
										push("link", link)
										if b == "" then b = " " end
										pushfragment(b)
										pushcontext("link", title or link)
									else
										pushfragment("[[", title or link, "][", link, "]]", b)
										pushfragment(a)
										pushcontext()
									end
									break
								end
							end
						end

						if feature["i"] then
							-- imglink (@@...@@), (@<...<@), (@>...>@)
							line = line:gsub("@([@<>])(.*)([@<>])@", function(s1, link, s2)
								local extra
								if s1 == s2 and s1 == "<" then
									extra = 'style="float: left"'
								elseif s1 == s2 and s1 == ">" then
									extra = 'style="float: right"'
								end
								push("image", link, extra)
								pop()
								return ""
							end)
						end

						if feature["h"] then
							-- head ( = ... = )
							line = line:gsub("(=+)%s+(.*)%s+(=+)",
								function(s1, text, s2)
								local l = min(s1:len(), s2:len(), 5)
								popwhile("block", "item", "list")
								push("head", l)
								return text
							end)
						end

						-- output
						if line ~= "" then
							if not checktop("item", "block", "cell", "pre",
								"head", "emphasis", "link", "code") then
								popwhile("item", "list", "pre", "code")
								push("block")
							end
							if top() == "code" then
								doout("getcode", line, top())
							else
								doout("gettext", line, top())
							end
						end
						popif("emphasis", "head", "link", "code")
					end
				end
			end
		end

		self.previndent = self.indentlevel
		self.lnr = self.lnr + 1
	end

	popuntil()
	doout("exit")

	return self.is_dynamic_content
end

-------------------------------------------------------------------------------
--
--	parser = Markup:new(args): Creates a new markup parser. {{args}} can be a
--	table of initial arguments that constitute its behavior. Supported fields
--	in {{args}} are:
--
--	{{indentchar}} || Character recognized for indentation, default {{"\t"}}
--	{{input}}      || Filehandle or string, default {{io.stdin}}
--	{{rdfunc}}     || Line reader, by default reads using {{args.input:lines}}
--	{{wrfunc}}     || Writer func, by default {{io.stdout.write}}
--	{{docname}}    || Name of document, default {{"Manual"}}
--	{{features}}   || Feature codes, default {{"hespcadlint"}}
--
--	The parser supports the following features, which can be combined into
--	a string and passed to the constructor in {{args.features}}:
--
--	Code   || Description        || rough HTML equivalent
--	{{h}}  || Heading            || {{<h1>}}, {{<h2>}}, ...
--	{{e}}  || Emphasis           || {{<strong>}}
--	{{s}}  || Separator          || {{<hr>}}
--	{{p}}  || Preformatted block || {{<pre>}}
--	{{c}}  || Code               || {{<code>}}
--	{{a}}  || Anchor, link       || {{<a href="...">}}
--	{{d}}  || Definition         || {{<dfn>}}
--	{{l}}  || List               || {{<ul>}}, {{<li>}}
--	{{i}}  || Image              || {{<img>}}
--	{{n}}  || Node header        || {{<a name="...">}}
--	{{t}}  || Table              || {{<table>}}
--	{{f}}  || Function           || (undocumented, internal use only)
--
--	After creation of the parser, conversion starts by calling Markup:run().
--
-------------------------------------------------------------------------------

function Markup.new(class, self)
	self = self or { }
	self.indentchar = self.indentchar or "\t"
	self.input = self.input or stdin
	self.rdfunc = self.rdfunc or
		type(self.input) == "string" and rd_string or self.input.lines
	self.wrfunc = self.wrfunc or function(s) stdout:write(s) end
	self.features = self.features or "hespcadlint"
	self.docname = self.docname or "Manual"
	self.author = self.author or false
	self.created = self.created or false

	self.column = false
	self.inline = false
	self.brpend = false
	self.prepend = false
	self.line = false
	self.id = false
	self.cstack = false
	self.context = false
	self.lnr = false
	self.indentlevel = 0
	self.istabline = false
	self.preindent = false
	self.previndent = 0
	self.preline = false
	self.stack = { }
	self.depth = 0
	self.in_table = false
	self.prebr = false
	self.is_dynamic_content = false

	return Class.new(class, self)
end

return Markup
