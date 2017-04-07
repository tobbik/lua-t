B=require't.Buffer'
S=B.Segment

b=B('1234567890')
print( 'Read All:', b:read() )
print( 'Read 4..:', b:read(4) )
print( 'Read 5o3:', b:read(3,5) )

bs0=S(b,1,0)
bs1=S(b,1,1)
bs34=S(b,3,4)
bs50=S(b,5,0)
bsE=S(b,#b,0)

for i=1,#b+1 do
	bs = S(b,i)
	print( i, bs, bs:read(), b:read(i) )
end
