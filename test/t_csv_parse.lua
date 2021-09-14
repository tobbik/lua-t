---
-- \file    test/t_csv_parse.lua
-- \brief   Test for the CSV/TSV parser
local Test     = require't.Test'
local Csv      = require't.Csv'

local verifyCsv = function( csv, source, result )
	for row,n in csv:rows( Csv.split( source, "\n" ) ) do
		for i,v in pairs( result ) do
			assert( result[n][i] == row[i], ("For key <%s> expected <%s> but got <%s>"):format(i, result[n][i], row[i]) )
		end
	end
end

return {
	Basic = function( self )
		Test.describe("Basic straight forward CSV parsing")
		local csv = Csv({headers=true})
		local src = [[
a,b,c
1,2,3]]
		local res = {
			{ "1","2","3", a="1", b="2", c="3" }
		}
		verifyCsv( csv, src, res )
	end,

	BasicCrlf = function( self )
		Test.describe("Basic straight forward CSV parsing with CRLF LineBreaks")
		local csv = Csv({headers=true})
		local src =
			"a,b,c\r\n" ..
			"1,2,3\r\n"
		local res = {
			{ "1","2","3", a="1", b="2", c="3" }
		}
		verifyCsv( csv, src, res )
	end,

	EmptyStrings = function( self )
		Test.describe("Parsing empty strings enclosed in quotes")
		local csv = Csv({headers=true})
		local src = [[
a,b,c
1,"",""
2,3,4]]
		local res = {
			{ "1","","", a="1", b="", c="" },
			{ "2","3","4", a="2", b="3", c="4" },
		}

		verifyCsv( csv, src, res )
	end,

	EmptyStringsCrlf = function( self )
		Test.describe("Parsing empty strings enclosed in quotes and CRLF line breaks")
		local csv = Csv({headers=true})
		local src = 
			"a,b,c\r\n" ..
			'1,"",""\r\n' ..
			"2,3,4\r\n"
		local res = {
			{ "1","","", a="1", b="", c="" },
			{ "2","3","4", a="2", b="3", c="4" },
		}

		verifyCsv( csv, src, res )
	end,

	CommaInQuotes = function( self )
		Test.describe("Parsing for Commas (Separators) within Quotes")
		local csv = Csv({headers=true})
		local src = [[
first,last,address,city,zip
John,Doe,120 any st.,"Anytown, WW",08123
]]
		local res = { {
			"John", "Doe","120 any st.","Anytown, WW","08123",
			first   = "John",
			last    = "Doe",
			address = "120 any st.",
			city    = "Anytown, WW",
			zip     = "08123"
		} }
		verifyCsv( csv, src, res )
	end,

	EscapedQuotes = function( self )
		Test.describe("Parsing for escaped quotes within Quotes")
		local csv = Csv({headers=true})
		local src = [[
a,b
1,"ha ""ha"" ha"
3,4
]]
		local res = {
			{"1",'ha "ha" ha', a="1",b='ha "ha" ha'},
			{"3", "4", a="3", b="4"}
		}
		verifyCsv( csv, src, res )
	end,

	Utf8 = function( self )
		Test.describe("Parsing UTF8 encoded content")
		local csv = Csv({headers=true})
		local src = [[
a,b,c
1,2,3
4,5,ʤ
]]
		local res = {
			{ "1","2","3", a="1", b="2", c="3" },
			{ "4","5","ʤ", a="4", b="5", c="ʤ" }
		}
		verifyCsv( csv, src, res )
	end,

	QuotesAndNewlines = function( self )
		Test.describe("Parsing escaped quotes and enclosed newlines")
		local csv = Csv({headers=true})
		local src = [[
a,b
1,"ha 
""ha"" 
ha"
3,4
]]
		local res = {
			{ "1", 'ha \n"ha" \nha', a="1", b='ha \n"ha" \nha'},
			{"3", "4", a="3", b="4"}
		}
		verifyCsv( csv, src, res )
	end,

	NewlinesInQuotes = function( self )
		Test.describe("Parsing newlines enclosed in quotes")
		local csv = Csv({headers=true})
		local src = [[
a,b,c
1,2,3
"Once upon 
a time",5,6
7,8,9
]]
		local res = {
			{ "1","2","3", a="1", b="2", c="3" },
			{ "Once upon \na time", "5","6", a="Once upon \na time", b="5", c="6" },
			{ "7","8","9", a="7", b="8", c="9" }
		}
		verifyCsv( csv, src, res )
	end,

	QuotesAndNewlinesCrlf = function( self )
		Test.describe("Parsing escaped quotes and enclosed newlines with CRLF linebreaks")
		local csv = Csv({headers=true})
		local src = 'a,b,c\r\n'..
			         '1,2,3\r\n' ..
			         '"Once upon \r\n' ..
			         'a ""time"" \r\n' ..
			         'in ""space""?",5,6\r\n' ..
			         '7,8,9\r\n'
		local res = {
			{ "1","2","3", a="1", b="2", c="3" },
			{ 'Once upon \r\na "time" \r\nin "space"?',"5","6", a='Once upon \r\na "time" \r\nin "space"?', b="5", c="6" },
			{ "7","8","9", a="7", b="8", c="9" }
		}
		verifyCsv( csv, src, res )
	end,

	DoublequoteInsanity = function( self )
		Test.describe("Parsing multiple escaped quotes right close to each other")
		local csv = Csv({headers=true})
		local src = [[
a,b,c
,"foo", """""x"""
]]
		local res = {
			{ nil,"foo",'""x"', a=nil, b="foo", c='""x"' },
		}
		verifyCsv( csv, src, res )
	end,

	DoublequoteNewlineInsanity = function( self )
		Test.describe("Parsing multiple escaped quotes right close to each other with enclosed newlines")
		local csv = Csv({headers=true})
		local src = [[
a,b,c,d
,"foo", """
""x""""
",bar
]]
		local res = {
			{ nil,"foo",'""x""\n', "bar", a=nil, b="foo", c='""x""\n', d="bar" },
		}
		verifyCsv( csv, src, res )
	end,

	EmptyStringsAndNilValues = function( self )
		Test.describe("Parsing distinguishes between empty string and empty fields")
		local csv = Csv({headers=true})
		local src = [[
a,b,c,d
1,,"",4
]]
		local res = {
			{ "1", nil, "", "4",a="1", b=nil, c="", d="4"}
		}
		verifyCsv( csv, src, res )
	end,

	LeadingSeparatorAsNil = function( self )
		Test.describe("Parsing line starting with separator as NULL(nil) field")
		local csv = Csv({headers=true})
		local src = [[
a,b,c,d
,2,3,4
]]
		local res = {
			{ nil, "2", "3", "4",a=nil, b="2", c="3", d="4"}
		}
		verifyCsv( csv, src, res )
	end,

	EndingSeparatorAsNil = function( self )
		Test.describe("Parsing line ending with separator as NULL(nil) field")
		local csv = Csv({headers=true})
		local src = [[
a,b,c,d
1,2,3,
]]
		local res = {
			{ "1", "2", "3", nil, a="1", b="2", c="3", d=nil}
		}
		verifyCsv( csv, src, res )
	end,

	EndingSeparatorAsNilCrlf = function( self )
		Test.describe("Parsing line ending with separator as NULL(nil) field with CRLF linebreaks")
		local csv = Csv({headers=true})
		local src = 
			"a,b,c,d\r\n" ..
			"1,2,3,\r\n"
		local res = {
			{ "1", "2", "3", nil, a="1", b="2", c="3", d=nil}
		}
		verifyCsv( csv, src, res )
	end,

	EndingEmptyString = function( self )
		Test.describe("Parsing line ending with empty string field")
		local csv = Csv({headers=true})
		local src = [[
a,b,c,d
1,2,3,""
]]
		local res = {
			{ "1", "2", "3", "", a="1", b="2", c="3", d=""}
		}
		verifyCsv( csv, src, res )
	end,

	EndingEmptyStringCrlf = function( self )
		Test.describe("Parsing line ending with empty string with CRLF linebreaks")
		local csv = Csv({headers=true})
		local src = 
			'a,b,c,d\r\n' ..
			'1,2,3,""\r\n'
		local res = {
			{ "1", "2", "3", "", a="1", b="2", c="3", d=""}
		}
		verifyCsv( csv, src, res )
	end,

	Json = function( self )
		Test.describe("Parsing doubleQuoted quotes within an JSON value")
		local csv = Csv({headers=true})
		local src = [==[
id,prop0,prop1,geojson
,value0,,"{""type"": ""Point"", ""coordinates"": [102.0, 0.5]}"
,value0,0.0,"{""type"": ""LineString"", ""coordinates"": [[102.0, 0.0], [103.0, 1.0], [104.0, 0.0], [105.0, 1.0]]}"
,value0,{u'this': u'that'},"{""type"": ""Polygon"", ""coordinates"": [[[100.0, 0.0], [101.0, 0.0], [101.0, 1.0], [100.0, 1.0], [100.0, 0.0]]]}"
]==]
		local res = {
			{ nil, "value0", nil,'{"type": "Point", "coordinates": [102.0, 0.5]}',
			  id=nil,prop0="value0",prop1=nil,geojson='{"type": "Point", "coordinates": [102.0, 0.5]}' },
			{ nil, "value0", '0.0','{"type": "LineString", "coordinates": [[102.0, 0.0], [103.0, 1.0], [104.0, 0.0], [105.0, 1.0]]}',
			  id=nil,prop0="value0",prop1='0.0',geojson='{"type": "LineString", "coordinates": [[102.0, 0.0], [103.0, 1.0], [104.0, 0.0], [105.0, 1.0]]}' },
			{ nil, "value0", "{u'this': u'that'}",'{"type": "Polygon", "coordinates": [[[100.0, 0.0], [101.0, 0.0], [101.0, 1.0], [100.0, 1.0], [100.0, 0.0]]]}',
			  id=nil,prop0="value0",prop1="{u'this': u'that'}",geojson='{"type": "Polygon", "coordinates": [[[100.0, 0.0], [101.0, 0.0], [101.0, 1.0], [100.0, 1.0], [100.0, 0.0]]]}' }
		}
		verifyCsv( csv, src, res )
	end,

	-- ###############################  Trimming Excessive whitespaces
	TrailingWhitespace = function( self )
		Test.describe("Parsing removes WhiteSPace after value and before delimiter")
		local csv = Csv({headers=true})
		local src = [[
a,b,c
1   ,2  ,3 
]]
		local res = {
			{ "1","2","3", a="1", b="2", c="3" },
		}
		verifyCsv( csv, src, res )
	end,

	TrimPreceedingWhitespace = function( self )
		Test.describe("Parsing removes WhiteSpace after delimiter and before value")
		local csv = Csv({headers=true})
		local src = [[
a,b,c
  1  ,2,  3
]]
		local res = {
			{ "1","2","3", a="1", b="2", c="3" },
		}
		verifyCsv( csv, src, res )
	end,

	TrimSurroundingWhitespace = function( self )
		Test.describe("Parsing removes WhiteSpace after delimiter and before value")
		local csv = Csv({headers=true})
		local src = [[
a,b,c
 1  , 2,  3  
]]
		local res = {
			{ "1","2","3", a="1", b="2", c="3" },
		}
		verifyCsv( csv, src, res )
	end,

	TrimPreceedingWhitespaceQuoted = function( self )
		Test.describe("Parsing removes WhiteSPace after quoted value and before delimiter")
		local csv = Csv({headers=true})
		local src = [[
a,b,c
 " 1 "," 2",  "3   "
]]
		local res = {
			{ " 1 "," 2","3   ", a=" 1 ", b=" 2", c="3   " },
		}
		verifyCsv( csv, src, res )
	end,

	TrimTrailingWhitespaceQuoted = function( self )
		Test.describe("Parsing removes WhiteSpace after delimiter and before quoted value")
		local csv = Csv({headers=true})
		local src = [[
a,b,c
" 1 "  ," 2","3   "   
]]
		local res = {
			{ " 1 "," 2","3   ", a=" 1 ", b=" 2", c="3   " },
		}
		verifyCsv( csv, src, res )
	end,

	TrimSurroundingWhitespaceQuoted = function( self )
		Test.describe("Parsing removes WhiteSpace after and before quoted values")
		local csv = Csv({headers=true})
		local src = [[
a,b,c
 " 1 ",  " 2" ,"3   "   
]]
		local res = {
			{ " 1 "," 2","3   ", a=" 1 ", b=" 2", c="3   " },
		}
		verifyCsv( csv, src, res )
	end,

	-- #################################### Parameters to csv constructor
	SkipParameterEatsLines = function( self )
		Test.describe("Skip Parameter actually skipping lines")
		local csv = Csv({headers=false, skip=3})
		local src = [[
// these lines
// here should
// be removed
1,foo,bar
2,foo,bar
3,foo,bar
]]
		local res = {
			{ "1", "foo", "bar" },
			{ "2", "foo", "bar" },
			{ "3", "foo", "bar" },
		}
		verifyCsv( csv, src, res )
	end,

}
