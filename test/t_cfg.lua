-- 
-- ---
-- \file    t_cfg.lua
-- \brief   Various configuration variables for the Test suite
-- \details can set various configuration variables
--
return {
	  privPort        = 25    -- priviledged port < 1000
	, nonPrivPort     = 2345  -- non priviledged port > 1000; nothing else should be bound to it
	, nonPrivPortAlt  = 2346  -- alternative non priviledged port > 1000; nothing else should be bound to it
}
