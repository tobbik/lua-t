-- 
-- ---
-- \file    t_cfg.lua
-- \brief   Various configuration variables for the Test suite
-- \details can set various configuration variables
--
local nonPrivPorts, nonPrivPortsAlt = {},{}
return setmetatable( { }, {
  __index = function( self, key )
    if           'privPort' == key then return 25
    elseif    'nonPrivPort' == key then
      local p = math.random(1500,2500)
      --print("NP  PORT",p)
      if nonPrivPorts[ p ] then
         print(("%d detected as duplicate nonPrivPort"):format(p))
      else
         nonPrivPorts[p] = true
      end
      --return p
      return 1500
      --return math.random(1500,2500)
    elseif 'nonPrivPortAlt' == key then
      local p = math.random(3000,4000)
      --print("NPA PORT",p)
      if nonPrivPortsAlt[ p ] then
         print(("%d detected as duplicate nonPrivPortAlt"):format(p))
      else
         nonPrivPortsAlt[p] = true
      end
      return 1501
      --return p
    end
  end
} )
