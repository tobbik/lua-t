#!../out/bin/lua

---
-- \file    t_oht.lua
-- \brief   Test for the Ordered Hash Table
local   Test   = require ('t').Test
local   Oht    = require ('t').OrderedHashTable

local tests = {
	setUp = function( self )
		self.keys = { 'one'  , 'two'   , 'three', 'four'  , 'five' , 'six'   , 'seven'   }
		self.vals = { 'first', 'second', 'third', 'fourth', 'fifth', 'sixth' , 'seventh' }
		assert( #self.keys == #self.vals, "Keys and Values must be of same length" )
		self.o    = Oht( )
		self.len  = 0
		-- insertion order is preserved inside the Hash table
		for i = 1, #self.keys do
			self.vals[i] = "This is an element at the " .. self.vals[i] .. " position."
			self.o[ self.keys[i] ] = self.vals[i]
			self.len  = self.len+1   -- count inserts
		end
		assert( #self.keys == self.len, "Number of inserts must equal length of Keys" )
	end,

	--tearDown = function( self )  -- not necessary for this suite
	--end,

	test_LengthMustEqualInserts = function( self )
		Test.describe( "Length of OrderedHashTable must be equal number of inserts" )
		assert( #self.o == self.len, "Length must equal number of inserts" )
	end,

	test_TableStyleConstructor = function( self )
		Test.describe( "Length of OrderedHashTable must be equal number of inserts" )
		o   = Oht(
			  { [self.keys[1]]= self.vals[1] }
			, { [self.keys[2]]= self.vals[2] }
			, { [self.keys[3]]= self.vals[3] }
			, { [self.keys[4]]= self.vals[4] }
		)
		assert( #o == 4, "Length must equal number of constructor arguments" )

		o[ self.keys[5] ] = self.vals[5]
		o[ self.keys[6] ] = self.vals[6]
		o[ self.keys[7] ] = self.vals[7]
		assert( #o == 4 + 3, "Length of OrderedHashTable must equal number of arguments" ..
		                     " in construtor plus number of inserts" )
	end,

	test_CopyConstructor = function( self )
		Test.describe( "OrderedHashTable constructed by copy must match original" )
		o   = Oht( self.o )
		assert( #self.o == self.len, "Original and clone length must be equal" )
		for i=1, self.len do
			assert( self.o[i] == o[i] , "Indexed value in clone must match original value" )
		end
	end,

	test_EmptyConstructor = function( self )
		Test.describe( "OrderedHashTable constructed from empty must match original" )
		o   = Oht( )
		for k,v,_ in pairs( self.o ) do  -- use pairs() iterator to fill copy
			o[k] = v
		end

		assert( #self.o == self.len, "Original and clone length must be equal" )
		for i=1, self.len do
			assert( self.o[i] == o[i] , "Indexed value in clone must match original value" )
		end
	end,

	test_PairsIterator = function( self )
		Test.describe( "Function pair() must iterate in proper order" )
		local ri = 0
		for k,v,i in pairs( self.o ) do
			ri = ri+1
			assert( i == ri, "Iterator index '"..i.."' must match running index '"..ri.."'" )
			assert( k == self.keys[i], "Iterator hash must match running index hash" )
			assert( v == self.vals[i], "Iterator value must match running index value" )
		end
		assert( #self.o == ri,
			 "Number of iterations in pairs() ("..ri..
			 ") must be equal to length of OrderedHashTable("..#self.o..")" )
	end,

	test_IpairsIterator = function( self )
		Test.describe( "Function ipair() must iterate in proper order" )
		local ri = 0
		for i,v,k in ipairs( self.o ) do
			ri = ri+1
			assert( i == ri, "Iterator index '"..i.."' must match running index '"..ri.."'" )
			assert( k == self.keys[i], "Iterator hash must match running index hash" )
			assert( v == self.vals[i], "Iterator value must match running index value" )
		end
		assert( #self.o == ri,
			 "Number of iterations in ipairs() ("..ri..
			 ") must be equal to length of OrderedHashTable("..#self.o..")" )
	end,

	test_Concat = function( self )
		Test.describe( "Concat concatenates values" )
		local sep   = 'willy nilly'

		assert( Oht.concat( self.o, separator ) == table.concat( self.vals, separator ),
			 "Concatenated values must equal normal table concat results" )
	end,

	test_AccessIndexedElements = function( self )
		Test.describe( "All elements must be available by their indexes" )
		for i=1, self.len do
			assert( self.o[i] == self.vals[i], "Indexed value must match original value" )
			assert( self.o[self.keys[i]] == self.o[i], "Hash access must match index access" )
			assert( Oht.getIndex( self.o, self.keys[i] ) == i, "Key index must match index" )
			assert( Oht.getKey( self.o, i ) == self.keys[i], "Key index must match index" )
		end
	end,

	test_AddElement = function( self )
		Test.describe( "Adding element increases size" )
		local o_len = #self.o
		local value   = string.gsub( self.vals[1], "first", "eighth" )

		self.o.eight  = value
		assert( #self.o      == o_len+1,    "Adding element must increase length" )
		assert( self.o.eight == value,      "New Element is available under given key" )
		assert( self.o[ #self.o ] == value, "New Element is available as last element" )
	end,

	test_DeleteHashElement = function( self )
		Test.describe( "Removing Hash element decreases size and move down higher keys" )
		local o_len = #self.o
		local four  = self.o.four
		local six   = self.o.six

		self.o.five   = nil
		assert( #self.o     == o_len-1, "Removing element must decrease length" )
		assert( self.o.four == four,    "Removing element musn't affect lower keys" )
		assert( self.o[4]   == four,    "Removing element musn't affect lower keys" )
		assert( self.o.six  == six,     "Removing element musn't affect higher keys" )
		assert( self.o[5]   == six,     "Removing element must move down higher keys" )
	end,

	test_DeleteIndexElement = function( self )
		Test.describe( "Removing Index element decreases size and move down higher keys" )
		local o_len = #self.o
		local four  = self.o[4]
		local six   = self.o[6]

		self.o[5]   = nil
		assert( #self.o     == o_len-1, "Removing element must decrease length" )
		assert( self.o.four == four,    "Removing element musn't affect lower keys" )
		assert( self.o[4]   == four,    "Removing element musn't affect lower keys" )
		assert( self.o.six  == six,     "Removing element musn't affect higher keys" )
		assert( self.o[5]   == six,     "Removing element must move down higher keys" )
	end,

	test_InsertElement = function( self )
		Test.describe( "Insert element increases size and move up higher keys" )
		local o_len  = #self.o
		local two    = self.o[2]
		local three  = self.o[3]
		local value  = string.gsub( self.vals[1], "first", "oddth" )
		local key    = 'odd'

		Oht.insert( self.o, 3, key, value )
		assert( #self.o      == o_len+1, "Inserting element must increase length" )
		assert( self.o[key]  == value,   "Inserted  element must be available by key" )
		assert( self.o[3]    == value,   "Inserted  element must be available by index" )
		assert( self.o.two   == two,     "Inserting element musn't affect lower keys" )
		assert( self.o[2]    == two,     "Inserting element musn't affect original lower keys" )
		assert( self.o.three == three,   "Inserting element musn't affect original higher keys" )
		assert( self.o[4]    == three,   "Inserting element must move up higher keys" )
	end,

	test_ReplaceHashElement = function( self )
		Test.describe( "Replace Hash element remains size, indexes and keys" )
		local o_len  = #self.o
		local idx    = 3
		local key    = 'three'
		local val    = string.gsub( self.vals[1], "first", "thirdish" )
		local keys   = { table.unpack( self.keys ) }
		local vals   = { table.unpack( self.vals ) }
		keys[ idx ]  = key
		vals[ idx ]  = val

		self.o[ key ] = val
		for i=1,self.len do
			assert( self.o[i]       == vals[i], "Indexed Values match after Replace" )
			assert( self.o[keys[i]] == vals[i], "Hashed  Values match after Replace" )
			assert( Oht.getIndex(self.o, keys[i] ) == i, "Indexes for keys match after Replace" )
		end
	end,

	test_ReplaceIndexElement = function( self )
		Test.describe( "Replace Index element remains size, indexes and keys" )
		local o_len  = #self.o
		local idx    = 3
		local key    = 'three'
		local val    = string.gsub( self.vals[1], "first", "thirdish" )
		local keys   = { table.unpack( self.keys ) }
		local vals   = { table.unpack( self.vals ) }
		keys[ idx ]  = key
		vals[ idx ]  = val

		self.o[ idx ] = val
		for i=1,self.len do
			assert( self.o[i]       == vals[i], "Indexed Values match after Replace" )
			assert( self.o[keys[i]] == vals[i], "Hashed  Values match after Replace" )
			assert( Oht.getIndex(self.o, keys[i] ) == i, "Indexes for keys match after Replace" )
		end
	end,

	test_CantCreateIndexOutOfBoundElement = function( self )
		Test.describe( "Can't create an element with an index higher than length" )
		local o_len    = #self.o
		local oob_idx  = o_len + 1
		local val      = "doesn't matter"
		local func     = function() self.o[ oob_idx ] = val end

		assert( not pcall( func ), "Not allowed to create elements with indexes" )
	end,

	test_GetValues = function( self )
		Test.describe( "GetValues() returns properly ordered list of values" )
		local sep   = '_|||_'
		local vals  = Oht.getValues( self.o )
		local c_oV  = table.concat( self.vals, sep )
		local c_rV  = table.concat( vals, sep )
		assert( c_oV == c_rV,
			 "List of values must be equal:\n_" ..c_oV .."_\n_" ..c_rV.."_" )
	end,

	test_GetKeys = function( self )
		Test.describe( "GetKeys() returns properly ordered list of keys" )
		local sep  = '_|||_'
		local keys = Oht.getKeys( self.o )
		local c_oK = table.concat( self.keys, sep )
		local c_rK = table.concat( keys, sep )
		assert( c_oK == c_rK,
			 "List of keys must be equal:\n_" ..c_oK .."_\n_" ..c_rK.."_")
	end,

	test_GetTable = function( self )
		Test.describe( "GetTable() returns a complete table of keyvalue pairs" )
		local t = Oht.getTable( self.o )
		local i = 0

		for k,v in pairs( t ) do
			assert( v == self.o[ k ], "Key/Value from simple table must matched OrderedHashTable" )
			i = i+1
		end
		assert( #self.o == i,
			 "Number of elements in simple table ("..i..
			 ") must be equal to number of elements in OrderedHashTable("..#self.o..")" )
	end,

	test_Equals = function( self )
		Test.describe( " __eq metamethod properly comparse for equality" )
		self.o.clone = Oht( self.o )
		local o      = Oht( self.o )
		assert( self.o == o, "Original and clone must be equal" )
	end,

	test_NotEquals = function( self )
		Test.describe( " __eq metamethod properly comparse for inequality" )
		self.o.clone = Oht( self.o )
		local o      = Oht( self.o )
		o.clone.six  = 'Not sixth anymore'

		assert( self.o ~= o, "Original and clone must be equal" )
	end
}

t_oht = Test( tests )
t_oht( )
print( t_oht )
