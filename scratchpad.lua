local Table    = require't.Table'
local t_map,t_count,t_concat = Table.map, Table.count, table.concat
local Test     = require't.Test'
local Http     = require't.Http'
local Request  = require't.Http.Request'
local Buffer   = require't.Buffer'
local Method, Version, Status = require't.Http.Method', require't.Http.Version', require't.Http.Status'
local format   = string.format
local t_assert = require't'.assert

local makeRequest = function()
	return Request( {
		srv = {
			callback = function( req, res )
				tReq = req
				tRes = res
			end
		}
	}, 1 )
end

--local T, Test, Time, format = require"t", require"t.Test", require"t.Time", string.format
--c_test = T.require( 'test.t_htp_req' )
--c_test()
--
--
r = makeRequest( )
-- taken from Wikipedia examples
p = string.rep( 'word ', 2469 ) -- 12345 chars
t = {
	  [ 'Accept' ]                         = "text/plain"
	, [ 'Accept-Charset' ]                 = "utf-8"
	, [ 'Accept-Encoding' ]                = "gzip, deflate"
	, [ 'Accept-Language' ]                = "en-US"
	, [ 'Accept-Datetime' ]                = "Thu, 31 May 2007 20:35:00 GMT"
	, [ 'Content-Length' ]                 = #p
	, [ 'Expect' ]                         = "100-continue"
	, [ 'Connection' ]                     = "keepAlive"
}
u = '/go/wherever/it/wil/be/index.html?alpha=1&beta=2&c=gamma&4=delta'
h = ''
for k,v in pairs(t) do h = h .. k ..': '..v.. '\r\n' end
s = "GET "..u.." HTTP/1.1\r\n" .. h .. '\r\n'..p
r:receive( s )
