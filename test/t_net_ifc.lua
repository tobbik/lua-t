---
-- \file      test/t_net_ifc.lua
-- \brief     test Net.Interface functionality
-- \author    tkieslich
-- \copyright See Copyright notice at the end of src/t.h

local   Test      = require( 't.Test' )
local   Interface = require( 't.Net.Interface' )
local   t_equals  = require( 't.Table' ).equals
local   t_type    = require( 't' ).type

return {
	getInterfaceList = function( self )
		Test.describe( "Interface.list() returns lists of Interfaces" )
		local ifs  = Interface.list()
		assert( type(ifs) == 'table', ("Interface.list() should have returned a <table> but was <%s>"):format( t_type(ifs) ) )
	end,

	getDefaultInterface = function( self )
		Test.describe( "Interface.default() returns main active Interfaces" )
		local ifc  = Interface.default()
		assert( ifc.flags, "Default interface should have a flags table" )
		assert( ifc.flags, "Default interface should have a flags table" )
		assert( ifc.flags.IFF_UP, "Default interface should have a flags.IFF_UP set" )
		if 'lo' ~= ifc.name then
			assert( ifc.flags.IFF_BROADCAST, "Default interface should have a flags.IFF_BROADCAST set" )
			assert( ifc.flags.IFF_MULTICAST, "Default interface should have a flags.IFF_MULTICAST set" )
		end
	end,

	getSpecificInterface = function( self )
		Test.describe( "Interface.get(<name>) returns appropriate Interface" )
		local ifs  = Interface.list()
		for name, ifc in pairs(ifs) do
			assert(name == ifc.name, ("Name in interface <%s> should equal key<%s> in interface list"):format( ifc.name, name) )
			local n_ifc = Interface.get( name )
			-- don't compare flags, if run over ssh the {rx,tx}_packets value change
			for key,_ in pairs( {AF_INET6=true, AF_INET=true, flags=true}) do
				assert( t_equals( n_ifc[key], ifc[key] ), ("Interfaces[%s] should be equal"):format( key) )
			end
		end
	end,
}
