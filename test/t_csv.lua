---
-- \file    test/t_csv.lua
-- \brief   Test for the CSV/TSV parser
local Test     = require't.Test'
local Csv      = require't.Csv'
local Buffer   = require't.Buffer'
local pp       = require't.Table'.pprint

return {

	DefaultContructor = function( self )
		Test.describe("Empty Constructor should fill out with default values")
		local csv     = Csv( )
		assert( ","  == csv.delimiter,  ("Expected delimiter <%s>, but was <%s>"):format(",", csv.delimiter ) )
		assert( '"'  == csv.quotchar,   ("Expected qoutation character <%s>, but was <%s>"):format('"', csv.quotchar ) )
		assert( "\\" == csv.escapechar, ("Expected escape character <%s>, but was <%s>"):format("\\", csv.escapechar ) )
		assert( csv.doublequoted, ("Expeted doubleQuoted <%s>, but was <%s>"):format(tostring(true), tostring(csv.doublequoted) ) )
	end,
	ContructorSetDelimiter = function( self )
		Test.describe("Constructor setting delimiter should fill delimiter field")
		local csv     = Csv( '\t' )
		assert( "\t" == csv.delimiter,  ("Expected delimiter <%s>, but was <%s>"):format("<TAB>", csv.delimiter ) )
	end,
	ContructorSetHeaders = function( self )
		Test.describe("Constructor setting header should set header behaviour")
		local csv     = Csv( nil, false )
		assert( false  == csv.headers,   ("Expected headers <%s>, but was <%s>"):format(tostring(false), tostring(csv.headers) ) )
	end,
	ContructorSetQuotechar = function( self )
		Test.describe("Constructor setting quotechar should set quotechar field")
		local csv     = Csv( nil, nil, "'" )
		assert( "'"  == csv.quotchar,   ("Expected qoutation character <%s>, but was <%s>"):format("'", csv.quotchar ) )
	end,
	ContructorSetEscapechar = function( self )
		Test.describe("Constructor setting escapechar should set escapechar field")
		local csv     = Csv( nil, nil, nil, "|" )
		assert( "|" == csv.escapechar, ("Expected escape character <%s>, but was <%s>"):format("|", csv.escapechar ) )
	end,
	ContructorSetDoublequoted = function( self )
		Test.describe("Constructor setting doublequote should set doublequote boolean field")
		local csv     = Csv( nil, nil, nil, nil, true )
		assert( csv.doublequoted, ("Expeted doubleQuoted <%s>, but was <%s>"):format(tostring(true), tostring(csv.doublequoted) ) )
		local csv     = Csv( nil, nil, nil, nil, false )
		assert( not csv.doublequoted, ("Expeted doubleQuoted <%s>, but was <%s>"):format(tostring(false), tostring(csv.doublequoted) ) )
	end,
	ContructorSetByTable = function( self )
		Test.describe("Empty Constructor should fill out with default values")
		local def = {
			delimiter    = "\t",
			quotchar     = "'",
			escapechar   = "|",
			doublequoted = false
		}
		-- if first argument is a table, following arguments shall be ignored
		local csv     = Csv( def, '"', '\\', true )
		for k,v in pairs(def) do
			assert( v == csv[k], ("Expeted '%s' <%s>, but was <%s>"):format(k, tostring(v), tostring(csv[k]) ) )
		end
	end,
	-- ####################################################################################
	-- Test for Csv.split() function
	SplitByChar = function( self )
		Test.describe("Csv.split(txt, delim) should split a string via a single character")
		local words, dlm = {"foo", "", "bar", "snafu", "another", "", "word", "and", "some", "more", "non-sense" }, ","
		local str, cnt   = table.concat( words, dlm ), 0
		for word, i in Csv.split( str, dlm ) do
			cnt = cnt+1
			assert( i    == cnt, ("Expected <%d> number, but got <%d>"):format( cnt, i))
			assert( word == words[i], ("Expected token <%s>, but got <%s>"):format(words[i], word))
		end
		assert( cnt == #words, ("Expected <%d> tokens, but got <%s>"):format( #words, cnt))
	end,

	SplitByCharTail = function( self )
		Test.describe("Csv.split(txt, delim) should split a string via a single character ending in delimiter")
		local words, dlm = {"foo", "", "bar", "snafu", "another", "", "word", "and", "some", "more", "non-sense", "", "" }, "|"
		local str, cnt   = table.concat( words, dlm ), 0
		for word, i in Csv.split( str, dlm ) do
			cnt = cnt+1
			assert( i    == cnt, ("Expected <%d> number, but got <%d>"):format( cnt, i))
			assert( word == words[i], ("Expected token <%s>, but got <%s>"):format(words[i], word))
		end
		assert( cnt == #words, ("Expected <%d> tokens, but got <%s>"):format( #words, cnt))
	end,

	SplitByCharSequence = function( self )
		Test.describe("Csv.split(txt, delim) should split a string via a character sequence")
		local words, dlm = {"foo", "", "bar", "snafu", "another", "", " word", "and", "some", "more", "non-sense" }, " ,^y "
		local str, cnt   = table.concat( words, dlm ), 0
		for word, i in Csv.split( str, dlm ) do
			cnt = cnt+1
			assert( i    == cnt, ("Expected <%d> number, but got <%d>"):format( cnt, i))
			assert( word == words[i], ("Expected token <%s>, but got <%s>"):format(words[i], word))
		end
		assert( cnt == #words, ("Expected <%d> tokens, but got <%s>"):format( #words, cnt))
	end,

	SplitByCharSequenceTail = function( self )
		Test.describe("Csv.split(txt, delim) should split a string via a character sequence")
		local words, dlm = {"foo", "", "bar", "snafu", "another", "", " word", "and", "some", "more", "non-sense", "", "" }, " ,^y "
		local str, cnt   = table.concat( words, dlm ), 0
		for word, i in Csv.split( str, dlm ) do
			cnt = cnt+1
			assert( i    == cnt, ("Expected <%d> number, but got <%d>"):format( cnt, i))
			assert( word == words[i], ("Expected token <%s>, but got <%s>"):format(words[i], word))
		end
		assert( cnt == #words, ("Expected <%d> tokens, but got <%s>"):format( #words, cnt))
	end,

	SplitByCharSequencePartial = function( self )
		Test.describe("Csv.split(txt, delim) should split a string composed of partial delimiters")
		local words, dlm = { }, "_01234567890-"
		for n=1,#dlm do table.insert( words, dlm:sub(1, #dlm-n) ) end
		local str, cnt   = table.concat( words, dlm ), 0
		--print(("STRING: <%s>"):format(str))
		for word, i in Csv.split( str, dlm ) do
			cnt = cnt+1
			--print( ("%d <%s>    %s"):format( i, word, words[i] ) )
			assert( i    == cnt, ("Expected <%d> number, but got <%d>"):format( cnt, i))
			assert( word == words[i], ("Expected token <%s>, but got <%s>"):format(words[i], word))
		end
		assert( cnt == #words, ("Expected <%d> tokens, but got <%s>"):format( #words, cnt))
	end,

	SplitCRLFByOnlyLF = function( self )
		Test.describe("Csv.split() CRLF separated Lines by LF only shall retain CR at end of line")
		local words    = {"a,b,c", "1,2,3", "" }
		local str, cnt = table.concat( words, "\r\n" ), 0
		for word, i in Csv.split( str, "\n" ) do
			cnt = cnt+1
			assert( i    == cnt, ("Expected <%d> number, but got <%d>"):format( cnt, i))
			if i<#words then
				assert( word == words[i].."\r", ("Expected token <%s>, but got <%s>"):format(words[i].."\r", word))
			end
		end
		assert( cnt == #words, ("Expected <%d> tokens, but got <%s>"):format( #words, cnt))
	end,

	SplitCRLFByFullCRLF = function( self )
		Test.describe("Csv.split() CRLF separated Lines by LF only shall retain CR at end of line")
		local words    = {"a,b,c", "1,2,3", "" }
		local str, cnt = table.concat( words, "\r\n" ), 0
		for word, i in Csv.split( str, "\r\n" ) do
			cnt = cnt+1
			assert( i    == cnt, ("Expected <%d> number, but got <%d>"):format( cnt, i))
			assert( word == words[i], ("Expected token <%s>, but got <%s>"):format(words[i], word))
		end
		assert( cnt == #words, ("Expected <%d> tokens, but got <%s>"):format( #words, cnt))
	end,


	-- ####################################################################################
	-- Test for Csv parsing
	CsvParsingSimple = function( self )
		Test.describe("Simple straight forward CSV parsing")
		local csv = Csv({headers=true})
		local src = [[
a,b,c
1,2,3]]
		local res = { "1","2","3", a="1", b="2", c="3" }
		for row,n in csv:rows( Csv.split( src, "\n" ) ) do
			--pp(row)
			for i,v in pairs(res) do
				--print( ("For key <%s> expected <%s> but got <%s>"):format(i, res[i], row[i]) )
				assert( res[i] == row[i], ("For key <%s> expected <%s> but got <%s>"):format(i, res[i], row[i]) )
			end
		end
	end,

	CsvParsingSimpleCrlf = function( self )
		Test.describe("Simple straight forward CSV parsing with CRLF LineBreaks")
		Test.todo("since io.lines() removes \n and \r lua-t.Csv can't recover that information")
		local csv = Csv({headers=true})
		local src =
			"a,b,\r\n" ..
			"1,2,3\r\n"
		local res = { "1","2","3", a="1", b="2", c="3" }
		for row,n in csv:rows( Csv.split( src, "\n" ) ) do
			--pp(row)
			for i,v in pairs(res) do
				--print( ("For key <%s> expected <%s> but got <%s>"):format(i, res[i], row[i]) )
				assert( res[i] == row[i], ("For key <%s> expected <%s> but got <%s>"):format(i, res[i], row[i]) )
			end
		end
	end,

	CsvParsingEmptyStrings = function( self )
		Test.describe("Parsing empty strings enclosed in quotes")
		Test.todo("Empty enclosed strings are not implemented yet")
		local csv = Csv({headers=true})
		local src = [[
a,b,c
1,"",""
2,3,4]]
		local res = {
			{ "1","","", a="1", b="", c="" },
			{ "2","3","4", a="2", b="3", c="4" },
		}

		for row,n in csv:rows( Csv.split( src, "\n" ) ) do
			pp(row)
			for i,v in pairs(res) do
				--print( ("For key <%s> expected <%s> but got <%s>"):format(i, res[i], row[i]) )
				assert( res[n][i] == row[i], ("For key <%s> expected <%s> but got <%s>"):format(i, res[n][i], row[i]) )
			end
		end
	end,

	CsvParsingEmptyStringsCrlf = function( self )
		Test.describe("Parsing empty strings enclosed in quotes and CRLF line breaks")
		Test.todo("Empty enclosed strings are not implemented yet")
		local csv = Csv({headers=true})
		local src = 
			"a,b,c\r\n" ..
			'1,"",""\r\n' ..
			"2,3,4\r\n"
		local res = {
			{ "1","","", a="1", b="", c="" },
			{ "2","3","4", a="2", b="3", c="4" },
		}

		for row,n in csv:rows( Csv.split( src, "\n" ) ) do
			pp(row)
			for i,v in pairs(res) do
				--print( ("For key <%s> expected <%s> but got <%s>"):format(i, res[i], row[i]) )
				assert( res[n][i] == row[i], ("For key <%s> expected <%s> but got <%s>"):format(i, res[n][i], row[i]) )
			end
		end
	end,

	CsvParsingCommaInQuotes = function( self )
		Test.describe("Parsing for Commas (Separators) within Quotes")
		local csv = Csv({headers=true})
		local src = [[
first,last,address,city,zip
John,Doe,120 any st.,"Anytown, WW",08123
]]
		local res = {
			"John", "Doe","120 any st.","Anytown, WW","08123",
			first   = "John",
			last    = "Doe",
			address = "120 any st.",
			city    = "Anytown, WW",
			zip     = "08123"
		}
		for row,n in csv:rows( Csv.split( src, "\n" ) ) do
			--pp(row)
			for i,v in pairs(res) do
				--print( ("For key <%s> expected <%s> but got <%s>"):format(i, res[i], row[i]) )
				assert( res[i] == row[i], ("For key <%s> expected <%s> but got <%s>"):format(i, res[i], row[i]) )
			end
		end
	end,

	CsvParsingEscapedQuotes = function( self )
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
		for row,n in csv:rows( Csv.split( src, "\n" ) ) do
			--pp(row)
			for i,v in pairs(res) do
				--print( ("For key <%s> expected <%s> but got <%s>"):format(i, res[i], row[i]) )
				assert( res[n][i] == row[i], ("For key <%s> expected <%s> but got <%s>"):format(i, res[n][i], row[i]) )
			end
		end
	end,

	CsvParsingUTF8 = function( self )
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
		for row,n in csv:rows( Csv.split( src, "\n" ) ) do
			--pp(row)
			for i,v in pairs(res) do
				--print( ("For key <%s> expected <%s> but got <%s>"):format(i, res[i], row[i]) )
				assert( res[n][i] == row[i], ("For key <%s> expected <%s> but got <%s>"):format(i, res[n][i], row[i]) )
			end
		end
	end,


	CsvParsingQuotesAndNewlines = function( self )
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
		for row,n in csv:rows( Csv.split( src, "\n" ) ) do
			--pp(row)
			for i,v in pairs(res) do
				--print( ("For key <%s> expected <%s> but got <%s>"):format(i, res[i], row[i]) )
				assert( res[n][i] == row[i], ("For key <%s> expected <%s> but got <%s>"):format(i, res[n][i], row[i]) )
			end
		end
	end,

	CsvParsingJson = function( self )
		Test.describe("Parsing doubleQuotes within an JSON value")
		Test.todo("Can not yet determine between empty string and <nil>")
		local csv = Csv({headers=true})
		local src = [=[
id,prop0,prop1,geojson
,value0,,"{""type"": ""Point"", ""coordinates"": [102.0, 0.5]}"
,value0,0.0,"{""type"": ""LineString"", ""coordinates"": [[102.0, 0.0], [103.0, 1.0], [104.0, 0.0], [105.0, 1.0]]}"
,value0,{u'this': u'that'},"{""type"": ""Polygon"", ""coordinates"": [[[100.0, 0.0], [101.0, 0.0], [101.0, 1.0], [100.0, 1.0], [100.0, 0.0]]]}"
]=]
		local res = {
			{ nil, "value0", nil,'{"type": "Point", "coordinates": [102.0, 0.5]}',
			  id=nil,prop0="value0",prop1=nil,geojson='{"type": "Point", "coordinates": [102.0, 0.5]}' },
			{ nil, "value0", '0.0','{"type": "LineString", "coordinates": [[102.0, 0.0], [103.0, 1.0], [104.0, 0.0], [105.0, 1.0]]}',
			  id=nil,prop0="value0",prop1='0.0',geojson='{"type": "LineString", "coordinates": [[102.0, 0.0], [103.0, 1.0], [104.0, 0.0], [105.0, 1.0]]}' },
			{ nil, "value0", "{u'this': u'that'}",'{"type": "Polygon", "coordinates": [[[100.0, 0.0], [101.0, 0.0], [101.0, 1.0], [100.0, 1.0], [100.0, 0.0]]]}',
			  id=nil,prop0="value0",prop1="{u'this': u'that'}",geojson='{"type": "Polygon", "coordinates": [[[100.0, 0.0], [101.0, 0.0], [101.0, 1.0], [100.0, 1.0], [100.0, 0.0]]]}' }
		}
		for row,n in csv:rows( Csv.split( src, "\n" ) ) do
			--pp(row)
			for i,v in pairs(res) do
				--print( ("For key <%s> expected <%s> but got <%s>"):format(i, res[i], row[i]) )
				assert( res[n][i] == row[i], ("For key <%s> expected <%s> but got <%s>"):format(i, res[n][i], row[i]) )
			end
		end
	end,

	CsvParsingNewlines = function( self )
		Test.describe("Parsing escaped quotes and enclosed newlines with CRLF linebreaks")
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
		for row,n in csv:rows( Csv.split( src, "\n" ) ) do
			--pp(row)
			for i,v in pairs(res) do
				--print( ("For key <%s> expected <%s> but got <%s>"):format(i, res[i], row[i]) )
				assert( res[n][i] == row[i], ("For key <%s> expected <%s> but got <%s>"):format(i, res[n][i], row[i]) )
			end
		end
	end,

	CsvParsingQuotesAndNewlinesCrlf = function( self )
		Test.describe("Parsing escaped quotes and enclosed newlines with CRLF linebreaks")
		Test.todo("since io.lines() removes \n and \r lua-t.Csv can't recover that information")
		local csv = Csv({headers=true})
		-- writing \r into [[ ]] string markers does not put it into the string
		local src =
			'a,b,c\r\n'..
			'1,2,3\r\n' ..
			'"Once upon \r\n' ..
			'a time",5,6\r\n' ..
			'7,8,9\r\n'
		local res = {
			{ "1","2","3", a="1", b="2", c="3" },
			{ "Once upon \r\na time","5","6", a="Once upon \r\na time", b="5", c="6" },
			{ "7","8","9", a="7", b="8", c="9" }
		}
		for row,n in csv:rows( Csv.split( src, "\n" ) ) do
			--pp(row)
			for i,v in pairs(res) do
				--print( ("For key <%s> expected <%s> but got <%s>"):format(i, res[i], row[i]) )
				assert( res[n][i] == row[i], ("For key <%s> expected <%s> but got <%s>"):format(i, res[n][i], row[i]) )
			end
		end
	end,
}
