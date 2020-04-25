Buffer=require't.Buffer'

x="Alles was ich habe ist meine Kuechenschabe"
x="1234567890123456789012345678901234567890"
b=Buffer(x)
s=b:Segment()
s:read(8,12)
b:read(8,12)
s1=b:Segment(8,12)
s1:read(8,12)
s:read(12,20)

