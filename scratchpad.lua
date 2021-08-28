Csv     = require't.Csv'
--csv     = Csv( 'sample.tsv', '\t' )
csv     = Csv( 'x.tsv', '\t' )

print( csv )
print( csv.delimiter )
print( csv.quotchar )
print( csv.escapechar )
print( csv.doublequoted )
print( csv.state )
print( csv.handle )

for k,v in pairs(getmetatable(csv)) do print(k,v) end
for row,r in csv:rows( ) do
  print( "ROW:", row, #row )
  for i,v in pairs( row ) do
    print( r, i, v)
  end
end
