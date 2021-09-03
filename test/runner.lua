---
-- \file      test/runner.lua
-- \brief     Test runner for lua-t unit tests call by
--            lua runner.lua FilePattern:TestPattern
-- \author    tkieslich
-- \copyright See Copyright notice at the end of t.h

local T, Suite, Oht, Table =
     require"t", require"t.Test.Suite", require"t.OrderedHashTable", require"t.Table"

local prxTblIdx            ,o_setElement   =
      Table.proxyTableIndex, Oht.setElement

local suites = {
	"t_ael",
	"t_buf"                 , "t_buf_seg",
	"t_net_adr"             , "t_net_ifc",
	"t_net_sck_create"      , "t_net_sck_bind",
	"t_net_sck_connect"     , "t_net_sck_listen",
	"t_net_sck_dgram_recv"  , "t_net_sck_dgram_send",
	"t_net_sck_stream_recv" , "t_net_sck_stream_send",
	"t_oht"                 , "t_set",
	"t_t"                   , "t_csv",
	"t_tbl"                 , "t_tbl_equals",
	"t_tst"                 ,
	--"t_pck_range"           , "t_pck_cmb",
	--"t_pck_bytes"           , "t_pck_bits",
	--"t_pck_fmt"             , "t_pck_mix",
	"t_htp_rsp"             , "t_htp_req",
}

local failures  = Suite( {} )
local collect_failures = function( s_name, results )
	for name, result in pairs(results) do
		if "FAIL" == result.status then
			o_setElement( failures[ prxTblIdx ], s_name ..":".. name, result )
		end
	end
end

local run = function( )
	local runnerCount, runnerTime = 0, 0
	for _,suite in ipairs( suites ) do
		print( ("   --- EXECUTING Suite: <%s>   ---------"):format( suite ) )
		local suiteResult, suiteTime, failed = Suite( T.require( suite ) )
		runnerCount, runnerTime = runnerCount + #suiteResult, runnerTime  + suiteTime
		--t = suiteResult --> push test suite into global scope
		collect_failures( suite, suiteResult )
		print( ("%d tests executed in: %.3f seconds\n\n"):format( #suiteResult, suiteTime/1000) )
	end
	print( ("Executed %d tests in %.3f seconds"):format( runnerCount, runnerTime/1000 ) )
	if #failures>0 then
		print( ("%d tests failed"):format( #failures ) )
	end
end
f=failures

run( )
