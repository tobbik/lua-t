#!out/bin/lua
t=require't'
s=t.Socket('TCP')
s:listen(5)
ip=s:getsockname()
