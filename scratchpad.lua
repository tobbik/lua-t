Csv = require"t.Csv"
csv = Csv( {
  delimiter   = ",",
  quotechar   = '"',
  escapechar  = "\\",
  doublequote = true,
  headers     = true
} )

csv = Csv( nil, nil )

print( csv )
print( "Delimiter :"    , csv.delimiter )
print( "QuoteChar :"    , csv.quotchar )
print( "EscapeChar :"   , csv.escapechar )
print( "DoubleQuoted :" , csv.doublequoted )
print( "State :"        , csv.state )
print( "Headers :"      , csv.headers )

csv = Csv( nil, false )
print( "Headers :"      , csv.headers )

csv = Csv( nil, true )
print( "Headers :"      , csv.headers )
--[=[
src = [[
a,b,c
1,2,3]]

for row,r in csv:rows( io.lines('foo.csv') ) do
  print( "ROW:", row, #row )
  for i,v in pairs( row ) do
    print( ("\tROW: %s,  COLUMN: %s,  VALUE: %s"):format(r, i, v))
  end
end

print("############################")
csv = Csv( {
  delimiter   = ",",
  quotechar   = '"',
  escapechar  = "\\",
  doublequote = true,
  headers     = true
} )
for row,r in csv:rows( Csv.split(src,'\n') ) do
  print( "ROW:", row, #row )
  for i,v in pairs( row ) do
    print( ("\tROW: %s,  COLUMN: %s,  VALUE: %s"):format(r, i, v))
  end
end
--]=]
