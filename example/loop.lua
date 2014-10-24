xt=require'xt'

l=xt.Loop()
tm=xt.Time(2000)
t= l:addTimer(tm,false,print,1,2,3,4,5,6,7)
l:addTimer(xt.Time(4000),false,print,8,7,6,5,4,3,2,1)
l:run()
