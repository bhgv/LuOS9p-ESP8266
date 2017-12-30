#!/usr/bin/env lua

local ui = require "tek.ui"
local List = require "tek.class.list"
local L = ui.getLocale("tekui-demo", "schulze-mueller.de")
local insert = table.insert

local Window = ui.Window:newClass { _NAME = "_demo_list_window" }

function Window:calcDensity()
	local A = self:getById("the-input-area"):getText()
	local P = self:getById("the-input-population"):getText()
	A = A:gsub("%.", "")
	P = P:gsub("%.", "")
	local A = tonumber(A)
	local P = tonumber(P)
	if A and P and A > 0 then
		local D = math.floor(P / A)
		self:getById("the-input-density"):setValue("Text", tostring(D))
	end
end

function Window:setRecord(fields)
	self:getById("the-input"):setValue("Text", fields[1] or "")
	self:getById("the-input-name"):setValue("Text", fields[2] or "")
	self:getById("the-input-area"):setValue("Text", fields[3] or "")
	self:getById("the-input-population"):setValue("Text", fields[4] or "")
	self:getById("the-input-capital"):setValue("Text", fields[5] or "")
	self:getById("the-input-language"):setValue("Text", fields[6] or "")
	self:getById("the-input-density"):setValue("Text", fields[7] or "")
end

function Window:collectRecord()
	local line = { }
	insert(line, self:getById("the-input"):getText())
	insert(line, self:getById("the-input-name"):getText())
	insert(line, self:getById("the-input-area"):getText())
	insert(line, self:getById("the-input-population"):getText())
	insert(line, self:getById("the-input-capital"):getText())
	insert(line, self:getById("the-input-language"):getText())
	insert(line, self:getById("the-input-density"):getText())
	return line
end

-------------------------------------------------------------------------------
--	Create demo window:
-------------------------------------------------------------------------------

local window = Window:new
{
-- 	Orientation = "vertical",
	Id = "list-window",
	Title = L.LIST_TITLE,
	Status = "hide",
	HideOnEscape = true,
	SizeButton = true,
	Children =
	{
		ui.Group:new
		{
			Orientation = "vertical",
			Children =
			{
				ui.ListView:new
				{
					VSliderMode = "auto",
					HSliderMode = "auto",
					Headers = { "ISO-2/TLD", "Name", "Area km²", "Population", "Capital", "Official language", "Density/km²" },
					Child = ui.Lister:new
					{
						Id = "the-list",
						SelectMode = "multi",
						ListObject = List:new
						{
							Items =
							{
								{ { "AL", "Republika e Shqipërisë", "28.748", "2.831.741", "Tiranë", "Shqip", "99" } },
								{ { "AO", "República de Angola", "1.246.700", "20.900.000", "Luanda", "português", "11" } },
								{ { "BG", "Република България", "110.994", "7.364.570", "София", "български език", "66" } },
								{ { "BR", "República Federativa do Brasil", "8.514.215", "192.380.000", "Brasília", "português", "22" } },
								{ { "CL", "República de Chile", "755.696", "16.634.603", "Santiago de Chile", "español", "22" } },
--								{ { "CN", "中华人民共和国", "9.571.302", "1.349.585.838", "北京市", "普通话", "140" } },
								{ { "DE", "Bundesrepublik Deutschland", "357121", "81890000", "Berlin", "Deutsch", "226" } },
								{ { "DK", "Kongeriget Danmark", "43094", "5602628", "København", "dansk", "130" } },
								{ { "EC", "República del Ecuador", "283.561", "15.223.680", "Quito", "español", "54" } },
								{ { "EE", "Eesti Vabariik", "45.227", "1.339.662", "Tallinn", "eesti keel", "30" } },
								{ { "FI", "Suomen tasavalta", "338.432", "5.429.894", "Helsinki", "suomi", "16" } },
								{ { "FR", "République française", "547.026", "64.667.000", "Paris", "Français", "97" } },
								{ { "GH", "Republic of Ghana", "238.537", "25.241.998", "Accra", "English", "107" } },
								{ { "GR", "Ελληνική Δημοκρατία", "131.957", "10.815.197", "Αθήνα", "Νέα Ελληνικά", "82" } },
								{ { "HN", "República de Honduras", "112.090", "7.989.415", "Tegucigalpa", "español", "68" } },
								{ { "HT", "Repiblik d Ayiti", "27.750", "9.801.664", "Port-au-Prince", "kreyòl, français", "353" } },
								{ { "ID", "Republik Indonesia", "3.287.469", "1.210.569.573", "Jakarta", "Bahasa Melayu", "126" } },
--								{ { "IN", "भारत गणराज्य", "1.904.569", "237.556.363", "नई दिल्ली", "हिन्दी", "368" } },
								{ { "JA", "Jamaica", "10.991", "2.804.332", "Kingston", "English", "251" } },
--								{ { "JP", "日本国", "377.835", "126.659.683", "東京", "日本語", "337" } },
								{ { ".ki", "Ribaberiki Kiribati", "811", "103.000", "South Tarawa", "te taetae ni Kiribati", "127" } },
								{ { "KZ", "Қазақстан Республикасы", "2.724.900", "16.934.100", "Астана", "Қазақ тілі, русский язык", "6" } },
								{ { "LA", "ສາທາລະນະລັດ ປະຊາທິປະໄຕ ປະຊາຊົນລາວ", "236.800", "6.200.894", "ວຽງຈັນ", "ພາສາລາວ", "27" } },
								{ { "LV", "Latvijas Republika", "64.589", "2.023.800", "Rīga", "Latviešu valoda", "35" } },
								{ { "MG", "Repoblikan’i Madagasikara", "587.295", "22.005.222", "Antananarivo", "Malagasy, français", "38" } },
								{ { "MW", "Dziko la Malaŵi", "118.480", "14.212.000", "Lilongwe", "Chichewa, English", "120" } },
								{ { "NO", "Kongeriket Norge", "385.1991", "5.063.709", "Oslo", "Norsk", "13" } },
--								{ { "NP", "संघीय लोकतान्त्रिक गणतन्त्रात्मक नेपाल", "147.181", "26.494.504", "काठमांडौ", "नेपाली", "180" } },
								{ { "PL", "Rzeczpospolita Polska", "312.679", "38.501.000", "Warszawa", "język polski", "123" } },
								{ { "PY", "Tetã Paraguái", "406.752", "6.375.830", "Asunción", "Guaraní, Español", "16" } },
								{ { "RO", "România", "238.391", "20.121.641", "București", "limba română", "432" } },
								{ { "RW", "Repubulika y’u Rwanda", "26.338", "11.400.000", "Kigali", "Kinyarwanda, Français, English", "84" } },
								{ { "SK", "Slovenská republika", "49.034", "5.404.322", "Bratislava", "slovenský jazyk", "110" } },
								{ { "SL", "Republic of Sierra Leone", "71.740", "5.612.685", "Freetown", "Englisch", "74" } },
								{ { "TR", "Türkiye Cumhuriyeti", "814.578", "75.627.384", "Ankara", "Türkçe", "91" } },
--								{ { "TW", "中華民國", "36.179", "23.127.845", "臺北市", "普通话", "639" } },
								{ { "UG", "Jamhuri ya Uganda", "241.040", "34.509.205", "Kampala", "Kiswahili, English", "167" } },
								{ { "UZ", "Oʻzbekiston Respublikasi", "447.400", "30.183.400", "Toshkent", "Oʻzbek tili", "66" } },
								{ { "VE", "República Bolivariana de Venezuela", "916.445", "28.833.845", "Caracas", "español", "30" } },
								{ { "VN", "Cộng hoà Xã hội Chủ nghĩa Việt Nam", "331.698", "91.519.289", "Hà Nội", "tiếng Việt", "280" } },
							}
						},
						onSelectLine = function(self)
							ui.Lister.onSelectLine(self)
							local line = self:getItem(self.SelectedLine)
							if line then
								self.Window:setRecord(line[1])
							end
						end,
					}
				},
				ui.Group:new
				{
					Columns = 2,
					Children =
					{
						ui.Text:new
						{
							Class = "caption",
							Width = "auto",
							Text = "ISO/TLD:"
						},
						ui.Input:new
						{
							Id = "the-input",
							onEnter = function(self)
								ui.Input.onEnter(self)
								self:activate("next")
							end,
						},
						ui.Text:new
						{
							Class = "caption",
							Width = "auto",
							Text = "Name:"
						},
						ui.Input:new
						{
							Id = "the-input-name",
							onEnter = function(self)
								ui.Input.onEnter(self)
								self:activate("next")
							end,
						},
						ui.Text:new
						{
							Class = "caption",
							Width = "auto",
							Text = "Area km²:"
						},
						ui.Input:new
						{
							Id = "the-input-area",
							onEnter = function(self)
								ui.Input.onEnter(self)
								self.Window:calcDensity()
								self:activate("next")
							end,
						},
						ui.Text:new
						{
							Class = "caption",
							Width = "auto",
							Text = "Population:"
						},
						ui.Input:new
						{
							Id = "the-input-population",
							onEnter = function(self)
								ui.Input.onEnter(self)
								self.Window:calcDensity()
								self:activate("next")
							end,
						},
						ui.Text:new
						{
							Class = "caption",
							Width = "auto",
							Text = "Captial:"
						},
						ui.Input:new
						{
							Id = "the-input-capital",
							onEnter = function(self)
								ui.Input.onEnter(self)
								self:activate("next")
							end,
						},
						ui.Text:new
						{
							Class = "caption",
							Width = "auto",
							Text = "Official Language:"
						},
						ui.Input:new
						{
							Id = "the-input-language",
							onEnter = function(self)
								ui.Input.onEnter(self)
								self:activate("next")
							end,
						},
						ui.Text:new
						{
							Class = "caption",
							Width = "auto",
							Text = "Density/km²:"
						},
						ui.Input:new
						{
							Id = "the-input-density",
							onEnter = function(self)
								ui.Input.onEnter(self)
								self:activate("next")
							end,
						},
						ui.Text:new
						{
							Class = "caption",
							Width = "auto",
						},
						ui.Button:new
						{
							Id = "the-input-apply",
							Text = "Commit",
							Width = "auto",
							onClick = function(self)
								local line = { self.Window:collectRecord() }
								local list = self:getById("the-list")
								list:changeItem(line, list.CursorLine)
								list:rethinkLayout(true, 1)
							end
						}
					}
				}
			}
		},
			
		ui.Group:new
		{
			Orientation = "vertical",
			Width = "auto",
			Children =
			{
				ui.Button:new { Id = "new-button", Text = "_New",
					onClick = function(self)
						ui.Button.onClick(self)
						local list = self:getById("the-list")
						self.Window:setRecord({ })
						local input = self:getById("the-input")
						list:addItem("")
						list:setValue("CursorLine", list:getN())
						input:activate()
					end
				},
				ui.Button:new { Id = "insert-button", Text = "_Insert",
					onClick = function(self)
						ui.Button.onClick(self)
						local list = self:getById("the-list")
						local cl = list.CursorLine
						if cl > 0 then
							local input = self:getById("the-input")
							list:addItem("", list.CursorLine)
							self.Window:setRecord({ })
							input:activate()
						end
					end
				},
				ui.Button:new { Text = "D_elete",
					onClick = function(self)
						ui.Button.onClick(self)
						self = self:getById("the-list")
						local sl = self.SelectedLines
						-- delete from last to first.
						local t = { }
						for lnr in pairs(self.SelectedLines) do
							table.insert(t, lnr)
						end
						if #t > 0 then
							table.sort(t, function(a, b) return a > b end)
							local cl = t[#t]
							for _, lnr in ipairs(t) do
								self:remItem(lnr)
							end
							cl = math.min(self:getN(), cl)
							self:setValue("SelectedLine", cl)
							self:setValue("CursorLine", cl)
						end
					end
				},
				ui.Button:new { Text = "_Up",
					onClick = function(self)
						ui.Button.onClick(self)
						self = self:getById("the-list")
						local cl = self.CursorLine
						local entry = self:remItem(cl)
						if entry then
							self:addItem(entry, cl - 1)
							self:setValue("CursorLine", math.max(1, cl - 1))
						end
					end
				},
				ui.Button:new { Text = "_Down",
					onClick = function(self)
						ui.Button.onClick(self)
						self = self:getById("the-list")
						local cl = self.CursorLine
						local entry = self:remItem(cl)
						if entry then
							self:addItem(entry, cl + 1)
							self:setValue("CursorLine", cl + 1)
						end
					end
				}
			}
		}
					
					
	}
}

-------------------------------------------------------------------------------
--	Started stand-alone or as part of the demo?
-------------------------------------------------------------------------------

if ui.ProgName:match("^demo_") then
	local app = ui.Application:new()
	ui.Application.connect(window)
	app:addMember(window)
	window:setValue("Status", "show")
	app:run()
else
	return
	{
		Window = window,
		Name = L.LIST_TITLE,
		Description = L.LIST_DESCRIPTION
	}
end
