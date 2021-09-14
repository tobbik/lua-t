
--for line in io.lines('d.csv') do
--  print( (" %d  LINE: _%s_"):format(#line,line))
--end

Csv,pp = require"t.Csv", require't.Table'.pprint
csv = Csv( ",", true )
csv = Csv( {separator=",", headers=true} )
src = 'a,b,c,d\r\n' ..
      '1,2,3,4\r\n' ..
      'Z,Y,"cunt ""FROM"" hell",X\r\n'

src = '  f1  , g2 H5 ,i3    ,   j4  \n' ..
      '"   p1     "    ,"q2 r5 ",  "  S3    ","T4"\n' ..
      '"   x1     "    ,"y2 z5 ",  "  A3    ","   b4 "   ' ..
      '"g1", "h2 k5 ",  "i3 ""j4"" l  ","m5"   '

src = [[
"g1", "h2 
 k5 ",  "i3 ""j4"" l  "]]

src = [[
a,b,c,d
,2,3,4
]]

src = [[
"a", " b ", "",,x
1,2,3,4,5
]]


src = [[
f1  , g2 H5 ,i3    ,   j4  
   "   p1     "    ,"q2 r5 ",  "  S3    ","T4"
"   x1     "    ,"y2 z5 ",  "  A3    ","   b4 "
"g1", "h2 k5 ",  "i3 ""j4"" l  ","m5"   
"g1", "h2 
 k5 ",  "i3 ""j4"" l  ","m5"   ]]


src = [[
a,b,c,d,e,f,g,h
 1  , 2,  "3 " , 4  ,,6, "", 8
]]

src = [[
a,b,c
1,"",""
,3,]]

src = [[
a,b
1,"ha 
""ha"" 
ha"
3,4
]]


src = "a,b,c\r\n" ..
      "1,2,3\r\n"
--for row,r in csv:rows( io.lines('d.csv') ) do
for row,r in csv:rows( Csv.split(src,'\n') ) do
  print( "ROW:", row, #row )
  pp(row)
  --for i,v in pairs( row ) do
  for i=1,#row do
    print( ("\tROW: %s,  COLUMN: %s,  VALUE: _%s_"):format(r, i, row[i]))
  end
end


