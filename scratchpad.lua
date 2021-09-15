Csv,pp = require"t.Csv", require't.Table'.pprint
src="foo,bar,,foobar,snafu,and,some,other,random,words"

for word,n in Csv.split( src, ',', true ) do
  print( ("\tword: _%s_"):format(word))
end

src="foo|bar||||foobar||snafu||and||some||other||random||words||"
for word,n in Csv.split( src, '||', true ) do
  print( ("\tword: _%s_"):format(word))
end

