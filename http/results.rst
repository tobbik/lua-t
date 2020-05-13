keep-alive mode (RPi64):
=======================

lua-t:
------
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   150.28ms    4.62ms 173.25ms   97.94%
    Req/Sec     1.63k   477.17     2.47k    68.07%
  Latency Distribution
     50%  150.44ms
     75%  150.96ms
     90%  151.34ms
     99%  157.29ms
  193671 requests in 30.10s, 24.20MB read
Requests/sec:   6434.47
Transfer/sec:    823.16KB
thread 1 made 48693 requests and got 48448 responses
thread 2 made 48710 requests and got 48465 responses
thread 3 made 48738 requests and got 48493 responses
thread 4 made 48510 requests and got 48265 responses

node:
-----

  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   146.30ms   69.89ms 955.48ms   95.12%
    Req/Sec     1.79k   347.82     2.31k    80.29%
  Latency Distribution
     50%  130.97ms
     75%  133.10ms
     90%  142.90ms
     99%  526.00ms
  208306 requests in 30.05s, 25.92MB read
Requests/sec:   6931.25
Transfer/sec:      0.86MB
thread 1 made 52334 requests and got 52088 responses
thread 2 made 52326 requests and got 52082 responses
thread 3 made 52302 requests and got 52058 responses
thread 4 made 52322 requests and got 52078 responses


close(single) mode (RPi64):
==========================

lua-t:
------

  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   304.19ms   18.31ms 319.07ms   98.97%
    Req/Sec   813.94    466.02     2.11k    66.01%
  Latency Distribution
     50%  306.21ms
     75%  306.89ms
     90%  307.51ms
     99%  309.67ms
  95467 requests in 30.09s, 11.47MB read
Requests/sec:   3172.99
Transfer/sec:    390.43KB
thread 1 made 24093 requests and got 23848 responses
thread 2 made 24147 requests and got 23903 responses
thread 3 made 24015 requests and got 23771 responses
thread 4 made 24190 requests and got 23945 responses


node:
-----

  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   484.93ms  105.88ms 983.78ms   92.15%
    Req/Sec   704.12    518.20     1.99k    64.99%
  Latency Distribution
     50%  460.50ms
     75%  480.94ms
     90%  530.60ms
     99%  953.00ms
  59780 requests in 30.07s, 7.12MB read
Requests/sec:   1987.93
Transfer/sec:    242.31KB
thread 1 made 15191 requests and got 14945 responses
thread 2 made 15190 requests and got 14945 responses
thread 3 made 15190 requests and got 14945 responses
thread 4 made 15190 requests and got 14945 responses

