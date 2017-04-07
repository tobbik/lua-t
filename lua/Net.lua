local core       = require( "t.core" )
local interface  = require( "t.Net.Interface" )
local address    = require( "t.Net.Address" )
local socket     = require( "t.Net.Socket" )

local Net        = core.net

Net.Interface    = interface
Net.Address      = address
Net.Socket       = socket

return Net
