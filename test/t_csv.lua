---
-- \file    test/t_csv.lua
-- \brief   Test for the CSV module basics
local Test     = require't.Test'
local Csv      = require't.Csv'
local Buffer   = require't.Buffer'

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
			doublequoted = false,
			headers      = { "first column", "second column", "third column" },
			skip         = 7
		}
		-- if first argument is a table, following arguments shall be ignored
		local csv     = Csv( def, false, '"', '\\', true, 2 )
		for k,v in pairs(def) do
			assert( v == csv[k], ("Expeted '%s' <%s>, but was <%s>"):format(k, tostring(v), tostring(csv[k]) ) )
		end
	end,

	SkipParameterEatsLines = function( self )
		Test.describe("Skip Parameter ")
		local csv = Csv({headers=false, skip=3})
		assert( 3 == csv.skip, ('Expected "skip"> to be <%s>, but was <%s>'):format( 3, tostring(csv.skip)) )
	end,

}
