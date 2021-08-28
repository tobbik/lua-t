local words, chars = {}, {}
local letterFrequency = {
	['E'] = 12.02,
	['T'] =  9.10,
	['A'] =  8.12,
	['O'] =  7.68,
	['I'] =  7.31,
	['N'] =  6.95,
	['S'] =  6.28,
	['R'] =  6.02,
	['H'] =  5.92,
	['D'] =  4.32,
	['L'] =  3.98,
	['U'] =  2.88,
	['C'] =  2.71,
	['M'] =  2.61,
	['F'] =  2.30,
	['Y'] =  2.11,
	['W'] =  2.09,
	['G'] =  2.03,
	['P'] =  1.82,
	['B'] =  1.49,
	['V'] =  1.11,
	['K'] =  0.69,
	['X'] =  0.17,
	['Q'] =  0.11,
	['J'] =  0.10,
	['Z'] =  0.07,
	--['œ'] =  0.01
}

for l,f in pairs( letterFrequency ) do
	for i=1,math.floor(f*100) do
		table.insert( chars, l:lower() )
	end
end

local createWord = function( )
	local wrd, bend, ceil, min = {}, 5, 15, 3
	-- f(x) = x^5 / x^4       for more shorter than long words
	local length = math.floor(   math.pow( math.random( 1, ceil-min ), bend )  /  math.pow(ceil-min, bend-1) )  + min
	for i=1,length do
		table.insert( wrd, chars[ math.random( 1, #chars ) ] )
	end
	return table.concat( wrd, '' )
end

local words = {}
for n=1,10000 do
	table.insert( words, createWord() )
end

local m = {
	getKey = function( self )
		local key  = self.keys[ math.random( 1, #self.keys ) ]( self )
		while nil ~= self.aKeys[ key ] do
			key = self.keys[ math.random( 1, #self.keys ) ]( self )
		end
		self.aKeys[ key ] = true
		return key
	end,
	getVal = function( self )
		local val  = self.vals[ math.random( 1, #self.vals ) ]( self )
		while nil ~= self.aVals[ val ] do
			val = self.vals[ math.random( 1, #self.vals ) ]( self )
		end
		self.aVals[ val ] = true
		return val
	end,
	getKeys = function( self, n )
		local tbl = {}
		for k=1,n do
			table.insert( tbl, self:getKey( ) )
		end
		return tbl
	end,
	getVals = function( self, n )
		local tbl = {}
		for k=1,n do
			table.insert( tbl, self:getVal( ) )
		end
		return tbl
	end,
	getHash = function( self, n )
		local hsh = {}
		for k=1,n do
			hsh[ self:getKey( ) ] = self:getVal( )
		end
		return hsh
	end,
	getFunction = function( self )
		local f = load( "return function(x) return x end" )
		return f( )
	end,
	getCoroutine = function( self )
		local c = load( "return coroutine.create( function(x) return x end )" )
		return c( )
	end,
	getBoolean = function( self )
		return math.random( 1, self.max ) % 2 == 1
	end,
	getString = function( self, n )
		local str   = { }
		local count = 0
		n = n or math.random( 1000, 3000 )
		while count<n do
			local word = words[ math.random( 1, #words ) ]
			count = count + #word
			if count <= n then
				table.insert( str, word )
			else
				table.insert( str, word:sub( 1,  #word-(count-n) ) )
			end
		end
		return table.concat( str, '' )
	end,
	getWords = function( self, n )
		local str = {}
		for i=1,math.random(1, n or 12) do
			table.insert( str, words[ math.random( 1, #words ) ] )
		end
		return table.concat( str, ' ' )
	end,
	getFloat = function( self )
		return math.random( ) * math.random( 1, self.max )
	end,
	getInteger = function( self )
		return math.random( 1, self.max )
	end,
	getTable = function( self, size )
		local tbl = { }
		for i=1,math.random(1, size or 12) do
			table.insert( tbl, math.random( 1, self.max ) )
		end
		return tbl
	end
}

return setmetatable(
	  { }
	, {
		__call  = function( self, max )
			local rtvg =  {
				max  = max or 10000000,
				keys = { m.getTable, m.getWords, m.getFunction, m.getBoolean, m.getCoroutine, m.getFloat },
				vals = { m.getTable, m.getWords, m.getFunction, m.getBoolean, m.getCoroutine, m.getFloat, m.getInteger }
			}
			rtvg.aKeys  = { [ rtvg.keys[ math.random( 1, #rtvg.keys ) ]( rtvg ) ] = true }
			rtvg.aVals  = { [ rtvg.vals[ math.random( 1, #rtvg.vals ) ]( rtvg ) ] = true }

			return setmetatable( rtvg, { __index = m } )
		end
	}
)
