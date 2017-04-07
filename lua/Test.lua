local core = require( "t.core" )
local case = require( "t.Test.Case" )

local Test = core.tst

Test.Case  = case

return Test
--[[
return setmetatable(
	Test,
	{
		__call = function( self, t )
			local obj = setmetatable( { }, core.tst._meta )
			obj[ core.proxyTableIndex ] = t or { }
			t = obj[ core.proxyTableIndex ]
			for key, func in pairs( t ) do
--]]				
				
			
