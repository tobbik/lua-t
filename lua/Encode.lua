local core     = require( "t.core" )
local base64   = require( "t.Encode.Base64" )
local rc4      = require( "t.Encode.RC4" )
local crc      = require( "t.Encode.Crc" )

local Encode   = core.buf

Encode.Base64  = base64
Encode.RC4     = rc4
Encode.Crc     = crc

return Encode
