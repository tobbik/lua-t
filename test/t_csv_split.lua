---
-- \file    test/t_csv_split.lua
-- \brief   Test for the CSV module split() function
local Test     = require't.Test'
local Csv      = require't.Csv'

return {

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
		for word, i in Csv.split( str, dlm ) do
			cnt = cnt+1
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

}
