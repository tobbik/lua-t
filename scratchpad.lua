Csv,Loop    = require't.Csv', require't.Loop'
--csv     = Csv( 'sample.tsv', '\t' )
csv     = Csv( '\t',nil,nil,false )

print( csv )
print( csv.delimiter )
print( csv.quotchar )
print( csv.escapechar )
print( csv.doublequoted )
print( csv.state )

for k,v in pairs(getmetatable(csv)) do print(k,v) end
for row,r in csv:rows( io.lines('x.tsv') ) do
  print( "ROW:", row, #row )
  for i,v in pairs( row ) do
    print( r, i, v)
  end
end

--[[
s="foo  bar snafu another  word and some more non-sense  "

print("Splitting s at < >:", s)
for w in Csv.split( s, " " ) do
	print( #w, w )
	Loop.sleep(200)
end
print("Splitting s at <  >:", s)
s="foo  bar  snafu  another   word  and  some  more  non-sense   "
for w in Csv.split( s, "  " ) do
	print( #w, w )
	Loop.sleep(200)
end

words, dlm  = {"foo", "", "bar", "snafu", "another", "", " word", "and", "some", "more", "non-sense", "" }, ","
words, dlm  = {"aaaa", "baaa", "", "caaa","" }, "-+-+- "
words, dlm = {"foo", "", "bar", "snafu", "another", "", "word", "and", "some", "more", "non-sense" }, ",,"
str  = table.concat( words, dlm )
print("STRING: <"..str..">")
for word,i in Csv.split( str, dlm ) do
	print( ("%d <%s>"):format( i, word ) )
end

words, dlm = {"foo", "", "bar", "snafu", "another", "", "word", "and", "some", "more", "non-sense", "", "" }, " ,^&*( "
words, dlm = {"foo", "", "bar", "snafu", "another", "", "word", "and", "some", "more", "non-sense", "", "" }, "_*****_"
str   = table.concat( words, dlm ):sub(1,-2)
print(("STRING: <%s>"):format(str))
for word, i in Csv.split( str, dlm ) do
	print( ("%d <%s>"):format( i, word ) )
end

words, dlm = { }, "_01234567890-"
for n=1,#dlm do table.insert( words, dlm:sub(1, #dlm-n) ) end
for i,v in pairs(words) do print(i,v) end

--]]

