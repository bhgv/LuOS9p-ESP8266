-------------------------------------------------------------------------------
--
--	tek.ui.layout.default
--	Written by Timm S. Mueller <tmueller at schulze-mueller.de>
--	See copyright notice in COPYRIGHT
--
--	OVERVIEW::
--		[[#ClassOverview]] :
--		[[#tek.class : Class]] /
--		[[#tek.ui.class.layout : Layout]] /
--		DefaultLayout
--
--		This class implements a [[#tek.ui.class.group : Group]]'s ''default''
--		layouting strategy. The standard strategy is to adapt a Group's
--		contents dynamically to the free space available. It takes into
--		account an element's {{HAlign}}, {{VAlign}}, {{Width}}, and {{Height}}
--		attributes.
--
--	OVERRIDES::
--		- Layout:askMinMax()
--		- Layout:layout()
--		- Class.new()
--
-------------------------------------------------------------------------------

local db = require "tek.lib.debug"
local ui = require "tek.ui".checkVersion(112)
local Layout = require "tek.ui.class.layout"
local band = ui.band
local assert = assert
local unpack = unpack or table.unpack
local insert = table.insert
local remove = table.remove
local floor = math.floor
local min = math.min
local max = math.max
local tonumber = tonumber
local HUGE = ui.HUGE

local DefaultLayout = Layout.module("tek.ui.layout.default", "tek.ui.class.layout")
DefaultLayout._VERSION = "Default Layout 9.2"

local INDICES =
{
	{ 1, 2, 3, 4, 5, 6 },
	{ 2, 1, 4, 3, 6, 5 }
}

local ALIGN =
{
	{ "HAlign", "VAlign", "right", "bottom", "Width", "Height" },
	{ "VAlign", "HAlign", "bottom", "right", "Height", "Width" }
}

local FL_CHANGED = ui.FL_CHANGED

-------------------------------------------------------------------------------
--	new
-------------------------------------------------------------------------------

function DefaultLayout.new(class, self)
	self = self or { }
	self.XYWH = { }
	self.TempRect = { }
	self.TempMinMax = { }
	self.Weights = { }
	return Layout.new(class, self)
end

-------------------------------------------------------------------------------
--	getSameSize: tell if the group is in 'samesize' mode on the
--	given axis
-------------------------------------------------------------------------------

local function getSameSize(group, axis)
	local ss = group.SameSize
	return ss == true or (axis == 1 and ss == "width") or
		(axis == 2 and ss == "height")
end

-------------------------------------------------------------------------------
--	width, height, orientation = getStructure: get Group's structural
--	parameters
-------------------------------------------------------------------------------

local function getStructure(group)
	local gw = tonumber(group.Columns)
	local gh = tonumber(group.Rows)
	local nc = #group.Children
	if gw then
		return 1, gw, floor((nc + gw - 1) / gw)
	elseif gh then
		return 2, floor((nc + gh - 1) / gh), gh
	elseif group.Orientation == "horizontal" then
		return 1, nc, 1
	end
	return 2, 1, nc
end

-------------------------------------------------------------------------------
--	calcWeights: Calculates the group's weights array and stores it in the
--	layouter
-------------------------------------------------------------------------------

function DefaultLayout:calcWeights(group)
	local wx, wy = { }, { }
	local cidx = 1
	local _, gw, gh = getStructure(group)
	for y = 1, gh do
		for x = 1, gw do
			local c = group.Children[cidx]
			if not c then
				break
			end
			if not c.Invisible then
				local w = c.Weight
				if w then
					wx[x] = (wx[x] or 0) + w
					wy[y] = (wy[y] or 0) + w
				end
			end
			cidx = cidx + 1
		end
	end
	self.Weights[1], self.Weights[2] = wx, wy
end

-------------------------------------------------------------------------------
--	layoutAxis
-------------------------------------------------------------------------------

function DefaultLayout:layoutAxis(group, free, i1, i3, n, isgrid, pad, mab, 
	minmax)

	local fw, tw = 0, 0
	local list = { }
	local tmm = self.TempMinMax
	local wgh = self.Weights

	for i = 1, n do
		local mins, maxs = tmm[i1][i], tmm[i3][i]
		local free = maxs and (maxs > mins)
		local weight = wgh[i1][i]
		list[i] = { free, mins, maxs, weight, nil }
		if free then
			if weight then
				tw = tw + weight
			else
				fw = fw + 0x100
			end
		end
	end

	if tw < 0x10000 then
		if fw == 0 then
			tw = 0x10000
		else
			fw, tw = (0x10000 - tw) * 0x100 / fw, 0x10000
		end
	else
		fw = 0
	end

	local ss = getSameSize(group, i1) and
		(minmax[i1] - mab[i1] - mab[i3] - pad[i1] - pad[i3]) / n

	local e = { unpack(list) }

	local it = 0
	while #e > 0 do

		local rest = free
		local newfree = free
		it = it + 1

		local e2 = { }

		repeat

			local delta = 0
			local c = remove(e, 1)

			if c[1] then -- free
				if c[4] then -- weight
					delta = free * (c[4] / 0x100) * (tw / 0x100) / 0x10000
				else
					delta = free * 0x100 * (fw / 0x100) / 0x10000
				end
				delta = floor(delta)
			end

			if delta == 0 and it > 1 then
				delta = rest
			end

			local olds = c[5] or ss or c[2] or 0 -- size, mins
			local news = max(olds + delta, ss or c[2] or 0) -- mins
			if not (ss and isgrid) and c[3] and news > c[3] then -- maxs
				news = c[3] -- maxs
			end
			c[5] = news

			delta = news - olds
			newfree = newfree - delta
			rest = rest - delta

			if not c[3] or c[3] >= HUGE or c[5] < c[3] then -- maxs
				-- redo in next iteration:
				insert(e2, c)
			end

		until #e == 0

		if newfree < 1 then
			break
		end

		free = newfree
		e = e2

	end

	return list

end

-------------------------------------------------------------------------------
--	layout
-------------------------------------------------------------------------------

function DefaultLayout:layout(group, r1, r2, r3, r4, markdamage)

	local ori, gs1, gs2 = getStructure(group)

	if gs1 > 0 and gs2 > 0 then

		local isgrid = gs1 > 1 and gs2 > 1
		local i1, i2, i3, i4, i5, i6 = unpack(INDICES[ori])

		local c, isz, osz, m3, m4, mm, a
		local cidx = 1
		local A = ALIGN[ori]

		if i1 == 2 then
			gs1, gs2 = gs2, gs1
			r1, r2, r3, r4 = r2, r1, r4, r3
		end

		local gm = { group:getMargin() }
		local gr = { group:getRect() }
		local gp = { group:getPadding() }
		local minmax = { group.MinMax:get() }
		local goffs = gm[i1] + gp[i1]
		local xywh = self.XYWH

		if band(group.Flags, FL_CHANGED) ~= 0 then
			self:calcWeights(group)
		end

		-- layout on outer axis:
		local olist = self:layoutAxis(group,
			r4 - r2 + 1 - minmax[i2], i2, i4, gs2, isgrid, gp, gm, minmax)

		-- layout on inner axis:
		local ilist = self:layoutAxis(group,
			r3 - r1 + 1 - minmax[i1], i1, i3, gs1, isgrid, gp, gm, minmax)

		local children = group.Children

		-- size on outer axis:
		local oszmax = gr[i4] - gr[i2] + 1 - gp[i2] - gp[i4]

		-- starting position on outer axis:
		xywh[i6] = r2 + gm[i2] + gp[i2]

		-- loop outer axis:
		for oidx = 1, gs2 do

			if gs2 > 1 then
				oszmax = olist[oidx][5] -- size
			end

			-- starting position on inner axis:
			xywh[i5] = r1 + goffs

			-- loop inner axis:
			for iidx = 1, gs1 do

				c = children[cidx]
				if not c then
					return
				end
				
				if not c.Invisible then

					-- x0, y0 of child rectangle:
					xywh[1] = xywh[5]
					xywh[2] = xywh[6]

					-- element minmax:
					mm = { c.MinMax:get() }

					-- max per inner and outer axis:
					m3, m4 = mm[i3], mm[i4]

					-- inner size:
					isz = ilist[iidx][5] -- size

					a = c:getAttr(A[5])
					if a == "free" or a == "fill" then
						m3 = gr[i3] - gr[i1] + 1 - gp[i1] - gp[i3]
					end

					if m3 < isz then
						a = c:getAttr(A[1])
						if a == "center" then
							xywh[i1] = xywh[i1] + floor((isz - m3) / 2)
						elseif a == A[3] then
							-- opposite side:
							xywh[i1] = xywh[i1] + isz - m3
						end
						isz = m3
					end

					-- outer size:
					a = c:getAttr(A[6])
					if a == "fill" or a == "free" then
						osz = oszmax
					else
						osz = min(olist[oidx][5], m4)
						-- align if element does not fully occupy outer size:
						if osz < oszmax then
							a = c:getAttr(A[2])
							if a == "center" then
								xywh[i2] = xywh[i2] + floor((oszmax - osz) / 2)
							elseif a == A[4] then
								-- opposite side:
								xywh[i2] = xywh[i2] + oszmax - osz
							end
						end
					end

					-- x1, y1 of child rectangle:
					xywh[i3] = xywh[i1] + isz - 1
					xywh[i4] = xywh[i2] + osz - 1

					-- enter recursion:
					c:layout(xywh[1], xywh[2], xywh[3], xywh[4], markdamage)

					-- punch a hole for the element into the background:
					c:punch(group.FreeRegion)

					-- update x0:
					xywh[i5] = xywh[i5] + ilist[iidx][5] -- size

				end
				
				-- next child index:
				cidx = cidx + 1
			end

			-- update y0:
			xywh[i6] = xywh[i6] + olist[oidx][5] -- size
		end
	end

end

-------------------------------------------------------------------------------
--	askMinMax
-------------------------------------------------------------------------------

function DefaultLayout:askMinMax(group, m1, m2, m3, m4)

	local m = self.TempRect
	m[1], m[2], m[3], m[4] = 0, 0, 0, 0
	local ori, gw, gh = getStructure(group)

	if gw > 0 and gh > 0 then

		local cidx = 1
		local minx, miny, maxx, maxy = { }, { }, { }, { }
		local tmm = { minx, miny, maxx, maxy }
		self.TempMinMax = tmm
		local children = group.Children

		for y = 1, gh do
			for x = 1, gw do
				local c = children[cidx]
				if c then
					cidx = cidx + 1
					
					if not c.Invisible then

						local mm1, mm2, mm3, mm4 = c:askMinMax(m1, m2, m3, m4)
						
						local cw = c:getAttr("Width")
						if cw == "fill" then
							mm3 = nil
						elseif cw == "free" then
							mm3 = HUGE
						end

						local ch = c:getAttr("Height")
						if ch == "fill" then
							mm4 = nil
						elseif ch == "free" then
							mm4 = HUGE
						end

						mm3 = mm3 or ori == 2 and mm1
						mm4 = mm4 or ori == 1 and mm2

						minx[x] = max(minx[x] or 0, mm1)
						miny[y] = max(miny[y] or 0, mm2)

						if mm3 and (not maxx[x] or mm3 > maxx[x]) then
							maxx[x] = max(mm3, minx[x])
						end

						if mm4 and (not maxy[y] or mm4 > maxy[y]) then
							maxy[y] = max(mm4, miny[y])
						end
					end
				end
			end
		end

		local gs = gw
		for i1 = 1, 2 do
			local i3 = i1 + 2
			local ss = getSameSize(group, i1)
			local numfree = 0
			local remainder = 0
			for n = 1, gs do
				local mins = tmm[i1][n] or 0
				local maxs = tmm[i3][n]
				if ss then
					if not maxs or (maxs > mins) then
						numfree = numfree + 1
					else
						remainder = remainder + mins
					end
					m[i1] = max(m[i1], mins)
				else
					m[i1] = m[i1] + mins
				end
				if maxs then
					maxs = max(maxs, mins)
					tmm[i3][n] = maxs	-- !
					m[i3] = (m[i3] or 0) + maxs
				elseif ori == i1 then
					-- if on primary axis, we must reserve at least min:
					m[i3] = (m[i3] or 0) + mins
				end
			end
			if ss then
				if numfree == 0 then
					m[i1] = m[i1] * gs
				else
					m[i1] = m[i1] * numfree + remainder
				end
				if m[i3] and m[i1] > m[i3] then
					m[i3] = m[i1]
				end
			end
			gs = gh
		end

	end

	return m[1], m[2], m[3], m[4]
end

return DefaultLayout
