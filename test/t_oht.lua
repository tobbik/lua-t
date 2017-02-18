#!../out/bin/lua

---
-- \file    t_oht.lua
-- \brief   Test for the Ordered Hash Table
local   Test   = require ('t').Test
local   Oht    = require ('t').OrderedHashTable

local tests = {
	setUp = function( self )
		self.keys = { print, true  , {'a'}  , 0.75      , 'a string' , function(a) return a end, {} }
		self.vals = { false, 1.2345, 'third', Oht.concat, function(x) return x+x end, {}, {1,2} }
		assert( #self.keys == #self.vals, "Keys and Values must be of same length" )
		self.o, self.len    = Oht( ), 0
		-- insertion order is preserved inside the Hash table
		for i = 1, #self.keys do
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
			  { [self.keys[1]] = self.vals[1] }
			, { [self.keys[2]] = self.vals[2] }
			, { [self.keys[3]] = self.vals[3] }
			, { [self.keys[4]] = self.vals[4] }
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

	test_ConcatStrings = function( self )
		Test.describe( "Concat concatenates strings" )
		local sep = 'willy nilly'
		local keys = { 'one'  , 'two'   , 'three', 'four'  , 'five' , 'six'   , 'seven'   }
		local vals = { 'first', 'second', 'third', 'fourth', 'fifth', 'sixth' , 'seventh' }
		local o   = Oht()
		for i=1,#keys do o[keys[i]] = vals[1] end

		assert( Oht.concat( o, sep ) == table.concat( vals, sep ),
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
		local value = "eighth"

		self.o.eight         =  value
		assert( #self.o      == o_len+1,    "Adding element must increase length" )
		assert( self.o.eight == value,      "New Element is available under given key" )
		assert( self.o[ #self.o ] == value, "New Element is available as last element" )
	end,

	test_DeleteHashElement = function( self )
		Test.describe( "Removing Hash element decreases size and move down higher keys" )
		local o_len = #self.o
		local k4    = self.keys[4]
		local k5    = self.keys[5]
		local k6    = self.keys[6]
		local four  = self.o[k4]
		local six   = self.o[k6]

		self.o[k5]          =  nil
		assert( #self.o     == o_len-1, "Removing element must decrease length" )
		assert( self.o[k4]  == four,    "Removing element musn't affect lower keys" )
		assert( self.o[4]   == four,    "Removing element musn't affect lower keys" )
		assert( self.o[k6]  == six,     "Removing element musn't affect higher keys" )
		assert( self.o[5]   == six,     "Removing element must move down higher keys" )
	end,

	test_DeleteIndexElement = function( self )
		Test.describe( "Removing Index element decreases size and move down higher keys" )
		local o_len = #self.o
		local k4    = self.keys[4]
		local k6    = self.keys[6]
		local four  = self.o[4]
		local six   = self.o[6]

		self.o[5]   = nil
		assert( #self.o     == o_len-1, "Removing element must decrease length" )
		assert( self.o[k4]  == four,    "Removing element musn't affect lower keys" )
		assert( self.o[4]   == four,    "Removing element musn't affect lower keys" )
		assert( self.o[k6]  == six,     "Removing element musn't affect lower keys" )
		assert( self.o[5]   == six,     "Removing element must move down higher keys" )
	end,

	test_InsertElement = function( self )
		Test.describe( "Insert element increases size and move up higher keys" )
		local o_len = #self.o
		local k2    = self.keys[2]
		local k3    = self.keys[3]
		local v2    = self.o[2]
		local v3    = self.o[3]
		local value = "oddth"
		local key   = 'odd'
		local idx   = 3

		Oht.insert( self.o, idx, key, value )
		assert( #self.o       == o_len+1, "Inserting element must increase length" )
		assert( self.o[key]   == value,   "Inserted  element must be available by key" )
		assert( self.o[idx]   == value,   "Inserted  element must be available by index" )
		assert( self.o[k2]    == v2,      "Inserting element musn't affect lower keys" )
		assert( self.o[idx-1] == v2,      "Inserting element musn't affect original lower keys" )
		assert( self.o[k3]    == v3,      "Inserting element musn't affect original higher keys" )
		assert( self.o[idx+1] == v3,      "Inserting element must move up higher keys" )
	end,

	test_ReplaceHashElement = function( self )
		Test.describe( "Replace Hash element remains size, indexes and keys" )
		local o_len  = #self.o
		local idx    = 3
		local key    = self.keys[idx]
		local val    = "thirdish"
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
		local key    = self.keys[idx]
		local val    = "thirdish"
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
		local oob_idx  = #self.o + 1
		local val      = "doesn't matter"
		local func     = function() self.o[ oob_idx ] = val end

		assert( not pcall( func ), "Not allowed to create elements with indexes" )
	end,

	test_GetValues = function( self )
		Test.describe( "GetValues() returns properly ordered list of values" )
		--Test.todo( "Test.equal should report arrays as equal" )
		local vals  = Oht.getValues( self.o )
		for i=1,#vals do
			assert( vals[i] == self.vals[ i ],
				tostring( vals[i]) .." should equal ".. tostring( self.vals[i] ) )
		end
		--assert( Test.equal( vals == self.vals ), "getValues() shall return self.vals" )
	end,

	test_GetKeys = function( self )
		Test.describe( "GetKeys() returns properly ordered list of keys" )
		--Test.todo( "Test.equal should report arrays as equal" )
		local keys = Oht.getKeys( self.o )
		for i=1,#keys do
			assert( keys[i] == self.keys[ i ],
				tostring( keys[i]) .." should equal ".. tostring( self.keys[i] ) )
		end
		--assert( Test.equal( keys == self.keys ), "getKeys() shall return self.keys" )
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
		Test.describe( "__eq metamethod properly comparse for equality" )
		self.o.inner = Oht( self.o )
		local o      = Oht( self.o )
		assert( self.o == o, "Original and clone must be equal" )
	end,

	test_NotEquals = function( self )
		Test.describe( "__eq metamethod properly compares for inequality" )
		self.o.inner = Oht( self.o )
		local o      = Oht( self.o )
		local k6     = self.keys[6]
		o.inner[k6]  = 'Not sixth anymore'

		assert( self.o ~= o, "Original and clone must be equal" )
	end
}

t_oht = Test( tests )
t_oht( )
print( t_oht )
