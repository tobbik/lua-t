-- example script that demonstrates use of setup() to pass
-- data to and from the threads
--
-- is there a way to fetch the connection argument to the commandline in here?

local counter = 1
local threads = {}

function setup(thread)
   thread:set("id", counter)
   table.insert(threads, thread)
   counter = counter + 1
end

function init(args)
   --for  k,v in pairs(args) do print(k,v) end
   requests  = 0
   responses = 0

   local msg = "thread %d created"
   --print(msg:format(id))
end

function request()
   requests = requests + 1
   return wrk.request()
end

function response(status, headers, body)
   responses = responses + 1
end

function done(summary, latency, requests)
   for index, thread in ipairs(threads) do
      local id        = thread:get("id")
      local requests  = thread:get("requests")
      local responses = thread:get("responses")
      local msg = "thread %d made %d requests and got %d responses"
      print(msg:format(id, requests, responses))
   end
   local p_avg = 0
   for i=1,99 do
      if latency:percentile(i) < latency.mean then
         p_avg = i
      else
         break
      end
   end
   local f = io.open( 'spectrum.txt', "a+" )
   f:seek( "end", -6 )
   l = f:read( )
   if not l:match( "%d%d   $" ) then
      f:write( "===== ======== ========== ========= ========== ====== ======= ======= ======= ====== ====== ======= ======= ======= ======= ====== ===== ====== ====== ====== ====== ====== =====\n")
      f:write( "conns requests kBytes     requests  kBytes     latncy latency latency latency latncy latncy latency latency latency latency kBytes time  error  error  error  error  error  perc \n" )
      f:write( "      total    total      second    second     min    max     average std-dev 30%    50%    75%     90%     99%     99.99%  req    total conns  reads  writes timeou status averg\n" )
      f:write( "===== ======== ========== ========= ========== ====== ======= ======= ======= ====== ====== ======= ======= ======= ======= ====== ===== ====== ====== ====== ====== ====== =====\n")
   end
   local msg = "      %-7d  %-10.2f %-9.2f %-10.2f %-6.2f %-7.2f %-6.2f  %-6.2f  %-6.2f %-6.2f %-7.2f %-7.2f %-7.2f %-7.2f %-6.2f %-5.2f %-6d %-6d %-6d %-6d %-6d %-5d\n"
   f:write( msg:format(
      summary.requests,
      summary.bytes/1024,
      summary.requests    / (summary.duration / 1000000),
      summary.bytes/1024  / (summary.duration / 1000000),
      latency.min   /1000,
      latency.max   /1000,
      latency.mean  /1000,
      latency.stdev /1000,
      latency:percentile(30.0)  / 1000,
      latency:percentile(50.0)  / 1000,
      latency:percentile(75.0)  / 1000,
      latency:percentile(90.0)  / 1000,
      latency:percentile(99.0)  / 1000,
      latency:percentile(99.99) / 1000,
      (summary.bytes  / summary.requests) / 1024,
      summary.duration / 1000000,
      summary.errors.connect,
      summary.errors.read,
      summary.errors.write,
      summary.errors.timeout,
      summary.errors.status,
      p_avg
   ) )
   f:close( )
end

wrk.method = "GET"
--wrk.headers["Connection"] = "close"
--wrk.headers["Connection"] = "keep-alive"
