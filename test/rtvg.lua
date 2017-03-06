local makeWord = function( )
	local wrd = {}
	for i=1,math.random(1,12) do
		table.insert( wrd, string.char( math.random( 32, 123) ) )
	end
	return table.concat( wrd, '' )
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
		local str = {}
		for i=1,math.random(1, n or 12) do
			table.insert( str, makeWord( ) )
		end
		return table.concat( str, ' ' )
	end,
	getFloat = function( self )
		return math.random( ) * math.random( 1, self.max )
	end,
	getInteger = function( self )
		return math.random( 1, self.max )
	end,
	getTable = function( self )
		local tbl = { }
		for i=1,math.random(1, 12) do
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
				keys = { m.getTable, m.getString, m.getFunction, m.getBoolean, m.getCoroutine, m.getFloat },
				vals = { m.getTable, m.getString, m.getFunction, m.getBoolean, m.getCoroutine, m.getFloat, m.getInteger }
			}
			rtvg.aKeys  = { [ rtvg.keys[ math.random( 1, #rtvg.keys ) ]( rtvg ) ] = true }
			rtvg.aVals  = { [ rtvg.vals[ math.random( 1, #rtvg.vals ) ]( rtvg ) ] = true }

			return setmetatable( rtvg, { __index = m } )
		end
	}
)
