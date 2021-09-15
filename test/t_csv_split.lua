---
-- \file    test/t_csv_split.lua
-- \brief   Test for the CSV module split() function
local Test     = require't.Test'
local Csv      = require't.Csv'

local verifySplit = function( iter, words, w_suffix )
	local cnt = 0
	w_suffix = w_suffix or ""
	for word, i in iter do
		cnt = cnt+1
		assert( i == cnt, ("Expected <%d> number, but got <%d>"):format( cnt, i))
		if '' ~= w_suffix and i<#words then
			assert( word == words[i]..w_suffix, ("Expected token <%s>, but got <%s>"):format(words[i]..w_suffix, word))
		else
			assert( word == words[i], ("Expected token <%s>, but got <%s>"):format(words[i], word))
		end
	end
	assert( cnt == #words, ("Expected <%d> tokens, but got <%s>"):format( #words, cnt))
end


return {
	SplitByChar = function( self )
		Test.describe("Csv.split(txt, delim) should split a string via a single character")
		local words, dlm = {"foo", "", "bar", "snafu", "another", "", "word", "and", "some", "more", "non-sense" }, ","
		local str, cnt   = table.concat( words, dlm ), 0
		verifySplit( Csv.split( table.concat(words, dlm), dlm ), words )
	end,

	SplitByCharTail = function( self )
		Test.describe("Split a string ending in delimiter via a single character should return empty string")
		local words, dlm = {"foo", "", "bar", "snafu", "another", "", "word", "and", "some", "more", "non-sense", "", "" }, "|"
		verifySplit( Csv.split( table.concat(words, dlm), dlm ), words )
	end,

	SplitByCharSequence = function( self )
		Test.describe("Csv.split(txt, delim) should split a string via a character sequence")
		local words, dlm = {"foo", "", "bar", "snafu", "another", "", " word", "and", "some", "more", "non-sense" }, " ,^y "
		verifySplit( Csv.split( table.concat(words, dlm), dlm ), words )
	end,

	SplitByCharSequenceTail = function( self )
		Test.describe("Split a string ending in delimiter via character sequence should return empty string")
		local words, dlm = {"foo", "", "bar", "snafu", "another", "", " word", "and", "some", "more", "non-sense", "", "" }, " ,^y "
		verifySplit( Csv.split( table.concat(words, dlm), dlm ), words )
	end,

	SplitByCharSequencePartial = function( self )
		Test.describe("Csv.split(txt, delim) should split a string composed of partial delimiters")
		local words, dlm = { }, "_01234567890-"
		for n=1,#dlm do table.insert( words, dlm:sub(1, #dlm-n) ) end
		verifySplit( Csv.split( table.concat(words, dlm), dlm ), words )
	end,

	SplitCRLFByOnlyLF = function( self )
		Test.describe("Csv.split() CRLF separated lines split by LF only shall retain CR at end of line")
		local words    = {"a,b,c", "1,2,3", "x,y,z", "" }
		verifySplit( Csv.split( table.concat(words, "\r\n"), "\n" ), words, "\r" )
	end,

	SplitCRLFByFullCRLF = function( self )
		Test.describe("Csv.split() CRLF separated lines split by CRLF only shall have no CR or LF at end of line")
		local words, dlm    = {"a,b,c", "1,2,3", "" }, "\r\n"
		verifySplit( Csv.split( table.concat(words, dlm), dlm ), words )
	end,

	SplitByCharKeepDelimiter = function( self )
		Test.describe("Csv.split(txt, delim, true) - split string by single character, keep delimiter")
		local words, dlm = {"foo", "", "bar", "snafu", "another", "", "word", "and", "some", "more", "non-sense" }, ","
		verifySplit( Csv.split( table.concat(words, dlm), dlm, true ), words, dlm )
	end,

	SplitBySequenceKeepDelimiter = function( self )
		Test.describe("Csv.split(txt, delim, true) - split string by single character, keep delimiter")
		local words, dlm = {"foo", "", "bar", "snafu", "another", "", "word", "and", "some", "more", "non-sense" }, "{-=+|"
		verifySplit( Csv.split( table.concat(words, dlm), dlm, true ), words, dlm )
	end,

	SplitBySequenceKeepDelimiterLastEmpty = function( self )
		Test.describe("Csv.split(txt, delim, true) - split string by single character, keep delimiter, last field empty")
		local words, dlm = {"foo", "", "bar", "snafu", "another", "", "word", "and", "some", "more", "non-sense", "" }, "{-=+|"
		verifySplit( Csv.split( table.concat(words, dlm), dlm, true ), words, dlm )
	end,




}
