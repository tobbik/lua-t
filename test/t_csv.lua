---
-- \file    test/t_csv.lua
-- \brief   Test for the CSV/TSV parser
local Test     = require't.Test'
local Csv      = require't.Csv'

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
	ContructorSetQuotechar = function( self )
		Test.describe("Constructor setting quotechar should set quotechar field")
		local csv     = Csv( nil, "'" )
		assert( "'"  == csv.quotchar,   ("Expected qoutation character <%s>, but was <%s>"):format("'", csv.quotchar ) )
	end,
	ContructorSetEscapechar = function( self )
		Test.describe("Constructor setting escapechar should set escapechar field")
		local csv     = Csv( nil, nil, "|" )
		assert( "|" == csv.escapechar, ("Expected escape character <%s>, but was <%s>"):format("|", csv.escapechar ) )
	end,
	ContructorSetDoublequoted = function( self )
		Test.describe("Constructor setting doublequote should set doublequote boolean field")
		local csv     = Csv( nil, nil, nil, true )
		assert( csv.doublequoted, ("Expeted doubleQuoted <%s>, but was <%s>"):format(tostring(true), tostring(csv.doublequoted) ) )
		local csv     = Csv( nil, nil, nil, false )
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
}

