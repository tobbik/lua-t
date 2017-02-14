#!../out/bin/lua

---
-- \file    t_oht.lua
-- \brief   Test for the Set functionality
local   Test  = require ('t').Test
   Set   = require ('t').Set

local   tests = {
	setUp = function( self )
		self.intA = 12345
		self.strA = 'this is the first little string'
		self.fltA = 6.7890
		self.bolA = false
		self.tblA = { three=3, four='four', five=5, six='six' }
		self.arrA = { 3,'four',5,'six' }
		self.fncA = function(x) return string.rep('functionA', x) end

		self.intB = 54321
		self.strB = 'this is another little string'
		self.fltB = 0.9876
		self.bolB = true
		self.tblB = { seven=7, eight='eigth', ninth=9, tenth='ten' }
		self.arrB = { 7,'eight',9,'tenth' }
		self.fncB = function(x) return string.rep('functionB', x) end

		self.aryA = {
			self.intA,
			self.strA,
			self.fltA,
			self.bolA,
			self.tblA,
			self.arrA,
			self.fncA
		}
		self.aryB = {
			self.intB,
			self.strB,
			self.fltB,
			self.bolB,
			self.tblB,
			self.arrB,
			self.fncB
		}

		self.setA =  Set( self.aryA )
		self.setB =  Set( self.aryB )
		self.len  = #self.aryA
	end,

	--tearDown = function( self )  -- not necessary for this suite
	--end,

	-- -----------------------------------------------------------------------
	-- Constructor Tests
	-- -----------------------------------------------------------------------

	test_ConstructEmptySet = function( self )
		Test.describe( "Construct the Empty Set" )
		local eSet = Set()
		assert( #eSet == 0, "Length must be zero" )
		for i,v in pairs( eSet ) do
			assert( false, "No iteration over empty Set" )
		end
	end,

	test_ConstructEmptySetFromEmptyTable = function( self )
		Test.describe( "Construct Empty Set From Empty Array" )
		local eSet = Set( {} )
		assert( #eSet == 0, "Length must be zero" )
		for i,v in pairs( eSet ) do
			assert( false, "No iteration over empty Set" )
		end
		assert( eSet == Set(), "Set from empty table must be equal Emmpty set" )
	end,

	test_ConstructorFromArray = function( self )
		Test.describe( "Construct Set from Array" )
		assert( #self.setA == #self.aryA, "Length must be equal number of elements in array" )
		for i,v in ipairs( self.aryA ) do
			assert( self.setA[ v ], "Element '"..tostring(v).."' must exist in set" )
		end
	end,

	test_ConstructorFromTable = function( self )
		Test.describe( "Construct Set from (Hash) Table" )
		local tbl = { a1='a', b1=4, c1=function(a) return 2*a end, d1={1,2,3}}
		local set = Set( tbl )
		assert( #set == 4, "Length must be 4" )
		for k,v in pairs( tbl ) do
			assert( set[ v ], "Element '"..tostring(v).."' must exist in set" )
		end
	end,

	test_ConstructorFromMixTable = function( self )
		Test.describe( "Construct Set from (Hash) Table" )
		local tbl = { a1='a', b1=4, c1=function(a) return 2*a end, d1={1,2,3}, 6, 'astring', false }
		local set = Set( tbl )
		assert( #set == 7, "Length must be 7" )
		for k,v in pairs( tbl ) do
			assert( set[ v ], "Element '"..tostring(v).."' must exist in set" )
		end
	end,

	test_ConstructorByCloning = function( self )
		Test.describe( "Construct Set from existing set" )
		local lSet = Set( self.setB )
		assert( #self.setB == #lSet, "Length must be equal number elements in original set" )
		for i,v in ipairs( self.aryB ) do
			assert( self.setB[ v ], "Element '"..tostring(v).."' must exist in set" )
		end
	end,

	test_ConstructorRoundTrip = function( self )
		Test.describe( "Turn Set into table and Construct an equal Set from table" )
		local tbl = Set.getTable( self.setA )
		assert( #self.setA == #tbl, "Table length must be equal number elements in original set" )
		for i,v in ipairs( tbl ) do
			assert( self.setA[ v ], "Element '"..tostring(v).."' must exist in set" )
		end
		local cSet = Set( tbl )
		assert( cSet == self.setA, "New set must be equal to original set" )
	end,

	test_ConstructorRemovesDuplicates = function( self )
		Test.describe( "Constructing Set removes duplicates " )
		Test.todo( "Fix Constructor to remove duplicates" )
		for i=1,self.len do
			self.aryB[ self.len+i ] = self.aryB[ i ]
		end
		--print()
		--for i,v in ipairs(self.aryB) do
		--	print(i,v)
		--end
		--print( self.len, #self.aryB )
		assert( #self.aryB == 2*self.len, "New values must be double size of original array" )
		local lSet = Set( self.aryB )
		--print( self.len, #lSet )
		assert( #lSet == self.len, "New Set must be size of original array" )
	end,

	-- -----------------------------------------------------------------------
	-- Manipulate Elements of Set Add/Remove etc
	-- -----------------------------------------------------------------------

	test_AddElement = function( self )
		Test.describe( "Test adding elements to set" )

		self.setA[ 'x' ]       = true
		self.setA[ self.setB ] = true
		self.setA[ print ]     = true

		assert( #self.setA == self.len+3, "Length of SetA shall be " .. self.len+3 )
	end,

	test_AddExistingElements = function( self )
		Test.describe( "Adding existing set elements don't get added" )
		local lSet =  Set( self.setA )
		for i=1,self.len do
			lSet[ self.aryA[ i ] ] = true
		end
		assert( #self.setA == self.len, "Adding existing values shall not alter the Set" )
	end,

	test_RemoveElements = function( self )
		Test.describe( "Test removing elements from set" )

		self.setB[ self.bolB ] = nil
		self.setB[ self.intB ] = nil
		self.setB[ self.arrB ] = nil
		self.setB[ self.fncB ] = nil

		self.setA[ self.strA ] = nil
		self.setA[ self.fltA ] = nil
		self.setA[ self.tblA ] = nil

		assert( #self.setB == self.len-4, "Length of SetB shall be " .. self.len-4 )
		assert( #self.setA == self.len-3, "Length of SetA shall be " .. self.len-3 )

		assert( self.setA[ self.intA ], "Element '".. tostring( self.intA ) .."' shall exist" )
		assert( self.setA[ self.arrA ], "Element '".. tostring( self.arrA ) .."' shall exist" )
		assert( self.setA[ self.fncA ], "Element '".. tostring( self.fncA ) .."' shall exist" )
		assert( self.setA[ self.bolA ], "Element '".. tostring( self.bolA ) .."' shall exist" )

		assert( self.setB[ self.fltB ], "Element '".. tostring( self.fltB ) .."' shall exist" )
		assert( self.setB[ self.tblB ], "Element '".. tostring( self.tblB ) .."' shall exist" )
		assert( self.setB[ self.strB ], "Element '".. tostring( self.strB ) .."' shall exist" )
	end,

	test_RemoveNonExistingElements = function( self )
		Test.describe( "Removing non existing elements from set shall have no effect" )
		local aSet = Set( self.setA )
		local bSet = Set( self.setB )

		self.setB[ self.bolA ] = nil
		self.setB[ self.intA ] = nil
		self.setB[ self.arrA ] = nil
		self.setB[ self.fncA ] = nil

		self.setA[ self.strB ] = nil
		self.setA[ self.fltB ] = nil
		self.setA[ self.tblB ] = nil

		assert( self.setB == bSet, "Sets should still be identical ")
		assert( self.setA == aSet, "Sets should still be identical ")
	end,

	-- -----------------------------------------------------------------------
	-- Comparative operators
	-- -----------------------------------------------------------------------

	test_AreEqual = function( self )
		Test.describe( "Two sets are equal" )
		local aSet1 = Set( self.aryA )
		local aSet2 = Set( self.aryA )

		assert( aSet1 == aSet2, "Sets should be equal" )
		for i,v in ipairs( self.aryA ) do
			assert( aSet1[ v ], "Element '"..tostring(v).."' must exist in set" )
			assert( aSet2[ v ], "Element '"..tostring(v).."' must exist in set" )
		end
	end,

	test_AreNotEqual = function( self )
		Test.describe( "One element makes sets not equal" )
		local aSet = Set( self.setA )

		assert( aSet == self.setA, "Sets should be equal" )
		aSet.newElement1a1 = true
		assert( aSet ~= self.setA, "Sets should not be equal" )
	end,

	test_AreDisjoint = function( self )
		Test.describe( "Two sets with entirely different elements are disjoint" )
		local aSet = self.setA
		local bSet = self.setB

		assert( self.setA % self.setB, "Set should be disjoint" )
	end,

	test_AreNotDisjoint = function( self )
		Test.describe( "One common elements makes sets not disjoint" )
		local aSet = self.setA
		local bSet = self.setB

		aSet[ self.fncB ] = true   -- add one overlapping element

		assert( not (self.setA % self.setB), "Set should not be disjoint" )
	end,

	test_IsSubset = function( self )
		Test.describe( "Test set for being subset" )
		local aSet = Set( self.setA ) -- clone Set

		assert( #self.setA == #aSet, "Length of both sets must be equal" )

		assert( aSet == self.setA, "SetB should be equal setA" )
		assert( aSet <= self.setA, "SetB should be a Subset of setA" )
		assert( not (aSet < self.setA), "SetB is a Subset of setA, but not a true subset because it's equal" )
	end,

	test_IsNotSubset = function( self )
		Test.describe( "Test set for not being subset" )

		assert( not (self.setB <  self.setA), "SetB is not a Subset of setA" )
		assert( not (self.setB <= self.setA), "SetB is not a true Subset of setA" )
	end,

	test_IsTrueSubset = function( self )
		Test.describe( "Test set for being a true subset" )
		local aSet = Set( self.setA ) -- clone Set

		assert( #self.setA == #aSet, "Length of both sets must be equal" )

		self.setA[ self.fltA ] = nil
		self.setA[ self.fncA ] = nil

		assert( self.setA ~= aSet, "self.setA should not be equal aSet" )
		assert( self.setA <= aSet, "self.setA should be a Subset of aSet" )
		assert( self.setA <  aSet, "self.setA should be a true Subset of aSet" )
	end,

	test_IntersectDisjointShallCreateEmptySet = function( self )
		Test.describe( "The Intersection of 2 disjoint sets shall have no Elements" )

		local iSet = self.setA & self.setB

		assert( 0 == #iSet, "Length of Intersection shall be 0" )
	end,

	-- -----------------------------------------------------------------------
	-- Operators with results
	-- -----------------------------------------------------------------------

	test_Intersection = function( self )
		Test.describe( "The Intersection of 2 sets shall have only common Elements" )
		local aSet = Set( self.setA )

		-- remove 2 from aSet
		aSet[ self.bolA ] = nil
		aSet[ self.fncA ] = nil

		-- remove 2 from bSet
		self.setA[ self.strA ] = nil
		self.setA[ self.fltA ] = nil

		local iSet = self.setA & aSet

		assert( #self.setB-4 == #iSet, "Length of Intersection shall be ".. #self.setB-4 )
	end,

	test_IntersectionEqual = function( self )
		Test.describe( "The Intersection of 2 identical sets is the same set" )
		local aSet = Set( self.setA )
		local iSet = self.setA & aSet

		assert( self.setA == iSet, "Set and Intersection shall be equal" )
	end,

	test_IntersectionDisjunct = function( self )
		Test.describe( "The Intersection of 2 disjunct sets is the empty set" )
		local iSet = self.setA & self.setB

		assert( Set( ) == iSet, "Set and Intersection shall be equal" )
	end,

	test_Union = function( self )
		Test.describe( "The Union of 2 sets shall remove duplicate elements" )
		local aSet1 = Set( self.setA )
		local aSet2 = Set( self.setA )
		aSet1.newElement1a1 = true
		aSet1.newElement1a2 = true
		aSet2.newElement2a1 = true
		--print( Set.toString( aSet1 ) )
		--print( Set.toString( aSet2 ) )
		local uSet = aSet1 | aSet2

		assert( #self.setA+3 == #uSet, "Length of Union shall be ".. #self.setA+3 )
		for i,v in ipairs( self.aryA ) do
			assert( uSet[ v ], "Element '"..tostring(v).."' must exist in set" )
		end
		assert( uSet.newElement1a1, "Element 'newElement1a1' must exist in set" )
		assert( uSet.newElement1a2, "Element 'newElement1a2' must exist in set" )
		assert( uSet.newElement2a1, "Element 'newElement2a1' must exist in set" )
	end,

	test_UnionEqual = function( self )
		Test.describe( "The Union of 2 equal sets shall be the same set" )
		local uSet = self.setA | self.setA

		assert( self.setA == uSet, "Union shall be equal both operands" )
	end,

	test_UnionDisjunct = function( self )
		Test.describe( "The Union of 2 disjunct sets shall have the sum of their elements" )
		local uSet = self.setA | self.setB

		assert( #self.setA*2 == #uSet, "Length of Union shall be ".. #self.setB*2 )
		for i,v in ipairs( self.aryA ) do
			assert( uSet[ v ], "Element '"..tostring(v).."' must exist in union set" )
		end
		for i,v in ipairs( self.aryB ) do
			assert( uSet[ v ], "Element '"..tostring(v).."' must exist in union set" )
		end
	end,

	test_Complement = function( self )
		Test.describe( "The Complement of 2 sets shall remove elements in SetB From SetA" )
		local aSet = Set( self.setA )

		aSet[ self.bolA ] = nil
		aSet[ self.intA ] = nil
		aSet[ self.arrA ] = nil
		aSet[ self.fncA ] = nil

		local cSet = self.setA - aSet

		assert( #cSet == self.len - #aSet, "Length of SetB shall be " .. self.len -# aSet )

		assert( not cSet[ self.strA ], "Element '".. tostring( self.strA ) .."' shall not exist" )
		assert( not cSet[ self.fltA ], "Element '".. tostring( self.fltA ) .."' shall not exist" )
		assert( not cSet[ self.tblA ], "Element '".. tostring( self.tblA ) .."' shall not exist" )

		assert( cSet[ self.bolA ], "Element '".. tostring( self.bolA ) .."' shall exist" )
		assert( cSet[ self.intA ], "Element '".. tostring( self.intA ) .."' shall exist" )
		assert( cSet[ self.arrA ], "Element '".. tostring( self.arrA ) .."' shall exist" )
		assert( cSet[ self.fncA ], "Element '".. tostring( self.fncA ) .."' shall exist" )
	end,

	test_ComplementEqual = function( self )
		Test.describe( "The Complement of 2 equal sets shall be empty" )
		local aSet = Set( self.setA )
		local cSet = self.setA - aSet
		--print( #cSet, Set.toString( cSet ) )

		assert( Set( ) == cSet, "Complement shall be empty set" )
	end,

	test_ComplementDisjoint = function( self )
		Test.describe( "The Complement of 2 disjoint sets shall be equal 1st Set" )
		local cSet = self.setA - self.setB

		assert( self.setA == cSet, "Complement shall be equal SetB" )
	end,

	test_SymmetricDifference = function( self )
		Test.describe( "The Symmetric Difference of 2 equal sets shall be elements not in commmon" )
		local bSet1 = Set( self.setB )
		local bSet2 = Set( self.setB )
		bSet1.newElement1a1 = true
		bSet1.newElement1a2 = true
		bSet2.newElement2a1 = true
		bSet2.newElement2a2 = true

		local dSet = bSet1 ~ bSet2

		assert( 4 == #dSet, "Length of difference shall be 4" )
	end,

	test_SymmetricDifferenceEqual = function( self )
		Test.describe( "The Symmetric Difference of 2 equal sets shall be empty" )
		local aSet = Set( self.setA )
		local dSet = self.setA ~ aSet

		assert( Set(  ) == dSet, "Difference shall be empty set" )
	end,

	test_SymmetricDifferenceDisjunct = function( self )
		Test.describe( "The Symmetric Difference of 2 disjunt sets shall be the union of the two" )
		local dSet = self.setA ~ self.setB
		local uSet = self.setA ~ self.setB
		--print( #dSet, Set.toString( dSet ) )
		--print( #uSet, Set.toString( uSet ) )

		assert( uSet == dSet, "Difference shall be union of sets" )
	end,
}


t = Test( tests )
t( )
--print( t )
