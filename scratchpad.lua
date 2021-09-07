Csv,pp = require"t.Csv", require't.Table'.pprint
csv = Csv( ",", true )
src = 'a,b,c,d\r\n' ..
      '1,2,3,4\r\n' ..
      'Z,Y,"cunt ""FROM"" hell",X\r\n'

for row,r in csv:rows( io.lines('foo.csv') ) do
  print( "ROW:", row, #row )
  pp(row)
  for i,v in pairs( row ) do
    print( ("\tROW: %s,  COLUMN: %s,  VALUE: %s"):format(r, i, v))
  end
end
