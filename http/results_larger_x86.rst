Larger payloads (61 kBytes)
==========================

Hardware is the RaspberryPi 4 (4GB) runs ``wrk`` and the servers are running on
a NUC with an AMD A5000 APU(8GB). Please observe that in some cases the
transfer rate approaches 114000 kBytes per second which could be limited by the
Gigabit Ethernet hardware connection.  It does not invalidate the results as it
shows how the software reacts near the hardware limitation.

Legend
------

All values are pretty self explanatory and standard for HTTP load testing.
The only unusual value is ``perc averg``.  It stands for how many percent of
the request returned in lest than the average latency for the whole run.


lua-t (keep-alive)
------------------

Memory consumption:

.. code::

  [tobias@progress http]$ ps aux | grep s_t
  USER         PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
  tobias      3026 22.1  0.0   8392  6168 pts/1    S+   20:47   1:28 ../out/bin/lua s_t.lua 10000

Benchmark results:

===== ======== ========== ========= ========== ====== ======= ======= ======= ====== ====== ======= ======= ======= ======= ====== ===== ====== ====== ====== ====== ====== =====
conns requests kBytes     requests  kBytes     latncy latency latency latency latncy latncy latency latency latency latency kBytes time  error  error  error  error  error  perc 
      total    total      second    second     min    max     average stddev  30%    50%    75%     90%     99%     99.99%  req    total conns  reads  writes timeou status averg
===== ======== ========== ========= ========== ====== ======= ======= ======= ====== ====== ======= ======= ======= ======= ====== ===== ====== ====== ====== ====== ====== =====
50(c) 9436     582878.55  1850.23   114291.84  7.82   95.77   25.63   5.52    23.97  25.12  26.73   28.61   50.07   95.77   61.77  5.10  0      0      0      0      0      58   
1     14121    870194.25  706.01    43507.34   1.06   5.84    1.34    0.17    1.27   1.31   1.49    1.55    1.62    5.58    61.62  20.00 0      0      0      0      0      65   
5     37141    2288842.90 1849.55   113979.81  1.05   14.46   2.07    0.34    2.04   2.05   2.07    2.37    2.88    11.90   61.63  20.08 0      0      0      0      0      74   
10    37216    2293436.42 1860.42   114648.28  1.09   8.46    4.17    1.24    3.76   4.42   5.10    5.64    6.27    7.39    61.63  20.00 0      0      0      0      0      41   
50    37441    2308633.87 1862.74   114857.79  2.95   60.73   25.60   4.14    24.27  25.06  26.37   27.71   50.07   59.53   61.66  20.10 0      0      0      0      0      61   
100   37319    2302372.84 1856.67   114546.23  5.67   224.31  53.48   9.59    51.30  52.02  52.86   53.95   103.44  142.84  61.69  20.10 0      0      0      0      0      87   
200   31340    1936239.04 1564.94   96684.73   17.53  657.35  128.95  40.52   117.63 119.59 121.89  131.39  326.05  601.23  61.78  20.03 0      0      0      0      0      89   
500   21969    1369702.33 1094.53   68240.92   39.41  1884.11 444.91  102.92  429.93 435.64 459.73  478.71  799.23  1748.19 62.35  20.07 0      0      0      1      0      67   
800   19158    1205566.80 955.23    60110.05   97.68  1995.29 798.50  224.20  706.02 728.64 791.74  1040.09 1723.81 1987.35 62.93  20.06 0      0      0      178    0      75   
1000  18488    1169714.36 922.42    58360.42   118.98 1999.54 972.92  282.30  816.42 846.57 1069.99 1399.35 1897.32 1999.32 63.27  20.04 0      0      0      847    0      65   
===== ======== ========== ========= ========== ====== ======= ======= ======= ====== ====== ======= ======= ======= ======= ====== ===== ====== ====== ====== ====== ====== =====


nodejs
------

Memory consumption:

.. code::

  [tobias@progress http]$ ps aux | grep s_node
  USER         PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
  tobias      3052 40.3  1.1 627844 88396 pts/1    Rl+  20:54   2:18 node s_node.js 10000

Benchmark results:

===== ======== ========== ========= ========== ====== ======= ======= ======= ====== ====== ======= ======= ======= ======= ====== ===== ====== ====== ====== ====== ====== =====
conns requests kBytes     requests  kBytes     latncy latency latency latency latncy latncy latency latency latency latency kBytes time  error  error  error  error  error  perc 
      total    total      second    second     min    max     average stddev  30%    50%    75%     90%     99%     99.99%  req    total conns  reads  writes timeou status averg
===== ======== ========== ========= ========== ====== ======= ======= ======= ====== ====== ======= ======= ======= ======= ====== ===== ====== ====== ====== ====== ====== =====
      6463     398319.90  1267.36   78108.53   5.92   208.06  37.74   18.33   29.90  32.80  38.45   51.42   133.47  208.06  61.63  5.10  0      0      0      0      0      71   
      11748    723963.27  587.27    36189.94   1.20   13.19   1.64    0.39    1.62   1.66   1.70    1.74    2.73    11.73   61.62  20.00 0      0      0      0      0      41   
      34408    2120360.81 1719.40   105956.68  1.61   25.28   4.56    1.73    3.28   3.34   6.59    6.70    8.35    21.37   61.62  20.01 0      0      0      0      0      66   
      36540    2253117.70 1818.11   112107.72  8.03   102.65  26.24   4.67    24.83  25.63  26.82   28.21   50.06   99.95   61.66  20.10 0      0      0      0      0      63   
      36110    2225277.42 1804.87   111224.74  12.75  340.47  55.30   13.53   51.36  54.09  56.60   70.23   107.69  234.32  61.62  20.01 0      0      0      0      0      70   
      27105    1670378.55 1354.02   83443.06   28.67  543.50  147.32  36.50   126.64 132.30 155.53  208.09  248.65  531.82  61.63  20.02 0      0      0      0      0      71   
      14384    902185.13  716.20    44920.78   3.48   1786.49 449.23  120.99  402.63 441.85 470.56  562.20  932.91  1596.01 62.72  20.08 0      500    0      500    0      63   
      18623    1147624.19 929.34    57269.89   69.13  1174.64 641.17  112.69  577.58 619.64 695.54  778.23  1001.21 1172.47 61.62  20.04 0      0      0      0      0      57   
      12439    796585.81  619.84    39694.08   1.89   1997.04 900.90  278.41  788.80 897.11 1018.14 1212.98 1740.28 1996.66 64.04  20.07 0      961    0      1148   0      51   
===== ======== ========== ========= ========== ====== ======= ======= ======= ====== ====== ======= ======= ======= ======= ====== ===== ====== ====== ====== ====== ====== =====


go
--

Memory consumption:

.. code::

  [tobias@progress http]$ ps aux | grep s_go
  USER         PID %CPU %MEM     VSZ   RSS TTY     STAT START   TIME COMMAND
  tobias      3154 54.4  0.5 1449048 42444 pts/1   Sl+  21:00   3:06 ./s_go 10000

Benchmark results:

===== ======== ========== ========= ========== ====== ======= ======= ======= ====== ====== ======= ======= ======= ======= ====== ===== ====== ====== ====== ====== ====== =====
conns requests kBytes     requests  kBytes     latncy latency latency latency latncy latncy latency latency latency latency kBytes time  error  error  error  error  error  perc 
      total    total      second    second     min    max     average stddev  30%    50%    75%     90%     99%     99.99%  req    total conns  reads  writes timeou status averg
===== ======== ========== ========= ========== ====== ======= ======= ======= ====== ====== ======= ======= ======= ======= ====== ===== ====== ====== ====== ====== ====== =====
50(c) 9300     574854.63  1823.61   112721.44  7.51   36.97   25.99   2.22    25.09  26.04  27.37   28.76   30.77   36.97   61.81  5.10  0      0      0      0      0      48   
1     13780    849686.56  685.59    42274.21   1.06   14.83   1.38    0.28    1.31   1.34   1.39    1.48    2.43    13.38   61.66  20.10 0      0      0      0      0      73   
5     32359    1995283.33 1610.15   99283.36   0.97   11.17   2.36    0.81    1.88   2.32   2.79    3.25    5.05    7.84    61.66  20.10 0      0      0      0      0      51   
10    35632    2197223.67 1781.25   109839.34  1.03   12.52   4.33    1.28    3.79   4.56   5.26    5.69    7.07    11.27   61.66  20.00 0      0      0      0      0      43   
50    36685    2263380.53 1825.15   112607.36  9.29   47.79   26.11   2.21    25.02  26.22  27.66   28.84   30.69   45.37   61.70  20.10 0      0      0      0      0      47   
100   36339    2243734.42 1808.00   111634.34  10.19  349.37  55.51   16.22   52.00  53.31  55.24   57.16   108.94  345.18  61.74  20.10 0      0      0      0      0      77   
200   29748    1839556.94 1486.95   91950.25   24.99  668.41  135.52  37.57   126.38 128.13 130.23  133.06  339.05  616.07  61.84  20.01 0      0      0      0      0      91   
500   21223    1324255.79 1057.35   65975.93   96.02  1836.69 466.38  100.14  435.37 441.67 469.55  516.31  819.81  1720.52 62.40  20.07 0      0      0      1      0      71   
800   18776    1183500.91 934.63    58911.96   71.74  1998.76 816.51  220.86  723.23 744.98 839.95  1040.28 1741.58 1998.36 63.03  20.09 0      0      0      178    0      74   
1000  18119    1149112.51 902.88    57261.20   157.13 1999.97 982.43  285.97  824.64 855.00 1080.52 1421.60 1912.03 1999.66 63.42  20.07 0      0      0      887    0      65   
===== ======== ========== ========= ========== ====== ======= ======= ======= ====== ====== ======= ======= ======= ======= ====== ===== ====== ====== ====== ====== ====== =====


Results for single connections
==============================


lua-t
-----

Memory consumption:

.. code::

  [tobias@progress http]$ ps aux | grep s_t
  USER         PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
  tobias      3184 27.1  0.0   6304  4176 pts/1    R+   21:09   1:28 ../out/bin/lua s_t.lua 10000

Benchmark results:

===== ======== ========== ========= ========== ====== ======= ======= ======= ====== ====== ======= ======= ======= ======= ====== ===== ====== ====== ====== ====== ====== =====
conns requests kBytes     requests  kBytes     latncy latency latency latency latncy latncy latency latency latency latency kBytes time  error  error  error  error  error  perc 
      total    total      second    second     min    max     average stddev  30%    50%    75%     90%     99%     99.99%  req    total conns  reads  writes timeou status averg
===== ======== ========== ========= ========== ====== ======= ======= ======= ====== ====== ======= ======= ======= ======= ====== ===== ====== ====== ====== ====== ====== =====
50(c) 6695     413288.19  1313.19   81064.48   4.23   75.92   27.93   4.25    26.97  27.97  29.36   30.90   43.22   75.92   61.73  5.10  0      0      0      0      0      49   
1     7053     434601.21  352.64    21729.40   1.69   4.23    2.29    0.18    2.19   2.27   2.42    2.53    2.72    4.23    61.62  20.00 0      0      0      0      0      54   
5     21755    1340569.65 1082.34   66695.37   1.73   7.67    2.96    0.58    2.58   2.83   3.32    3.86    4.40    6.84    61.62  20.10 0      0      0      0      0      59   
10    24911    1535144.30 1240.55   76449.37   1.98   14.27   5.22    1.51    4.38   4.66   5.24    7.99    8.87    12.95   61.63  20.08 0      0      0      0      0      74   
50    26123    1610214.15 1301.27   80209.86   5.39   67.19   28.27   3.69    27.29  28.16  29.18   30.77   42.83   57.41   61.64  20.08 0      0      0      0      0      53   
100   25278    1560429.69 1257.67   77636.75   6.12   302.21  62.20   10.38   59.27  62.05  65.39   69.83   84.05   292.00  61.73  20.10 0      0      0      0      0      51   
200   25296    1562739.38 1258.52   77749.05   26.89  692.92  140.49  46.40   123.51 131.21 140.15  189.98  345.66  610.94  61.78  20.10 0      0      0      0      0      75   
500   23624    1469867.97 1178.28   73311.95   34.91  1981.14 372.52  180.05  258.83 296.78 431.95  604.98  1050.06 1952.95 62.22  20.05 0      0      0      2      0      67   
800   19719    1236783.59 983.68    61696.71   56.00  1999.87 662.84  305.67  432.30 549.10 809.65  1100.69 1695.01 1996.53 62.72  20.05 0      0      0      161    0      60   
1000  18485    1165458.64 920.83    58057.61   148.29 1999.16 850.14  364.41  540.56 761.92 1061.11 1406.05 1876.96 1999.08 63.05  20.07 0      0      0      604    0      59   
===== ======== ========== ========= ========== ====== ======= ======= ======= ====== ====== ======= ======= ======= ======= ====== ===== ====== ====== ====== ====== ====== =====


node-js
-------

Memory consumption:

.. code::

  [tobias@progress http]$ ps aux | grep s_node
  USER         PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
  tobias      3198 37.8  1.3 638052 99248 pts/1    Rl+  21:15   2:07 node s_node.js 10000

Benchmark results:

===== ======== ========== ========= ========== ====== ======= ======= ======= ====== ====== ======= ======= ======= ======= ====== ===== ====== ====== ====== ====== ====== =====
conns requests kBytes     requests  kBytes     latncy latency latency latency latncy latncy latency latency latency latency kBytes time  error  error  error  error  error  perc 
      total    total      second    second     min    max     average stddev  30%    50%    75%     90%     99%     99.99%  req    total conns  reads  writes timeou status averg
===== ======== ========== ========= ========== ====== ======= ======= ======= ====== ====== ======= ======= ======= ======= ====== ===== ====== ====== ====== ====== ====== =====
      4892     301679.81  959.57    59174.74   11.90  287.23  51.33   31.71   38.63  42.33  54.20   69.59   220.47  287.23  61.67  5.10  0      0      0      0      0      69   
      6156     379342.98  306.26    18872.43   1.96   15.49   2.71    0.57    2.59   2.68   2.77    2.85    5.45    15.49   61.62  20.10 0      0      0      0      0      56   
      18421    1135103.16 920.59    56726.97   1.96   16.65   3.65    1.40    2.77   3.10   4.50    5.53    8.14    16.48   61.62  20.01 0      0      0      0      0      66   
      24114    1485999.91 1199.72   73931.38   1.91   28.11   5.50    2.00    4.56   4.86   5.41    9.78    11.96   19.77   61.62  20.10 0      0      0      0      0      76   
      25838    1592820.97 1285.55   79249.69   4.70   79.12   29.05   4.49    27.87  28.57  29.65   31.96   42.04   78.94   61.65  20.10 0      0      0      0      0      63   
      24828    1531296.91 1240.98   76538.72   18.57  358.61  62.55   18.32   59.54  60.96  62.38   66.79   132.80  357.62  61.68  20.01 0      0      0      0      0      77   
      16239    1000633.22 797.82    49161.21   2.42   835.15  198.70  95.95   156.68 163.32 205.47  310.39  586.06  828.26  61.62  20.35 0      0      0      0      0      73   
      8198     505153.71  386.55    23818.59   3.98   1752.95 662.70  384.37  396.64 638.32 963.52  1178.31 1561.97 1752.95 61.62  21.21 0      0      0      0      0      52   
      17040    1049990.16 818.25    50419.94   5.46   1996.69 737.90  284.13  663.44 682.73 732.98  1138.03 1715.19 1995.40 61.62  20.82 0      0      0      33     0      75   
      9063     558454.27  430.05    26499.21   5.47   1999.73 983.21  550.98  599.48 965.08 1454.89 1750.85 1972.41 1999.73 61.62  21.07 0      0      0      967    0      51   
===== ======== ========== ========= ========== ====== ======= ======= ======= ====== ====== ======= ======= ======= ======= ====== ===== ====== ====== ====== ====== ====== =====


go
--

Memory consumption:

.. code::

  [tobias@progress http]$ ps aux | grep s_go
  USER         PID %CPU %MEM     VSZ   RSS TTY     STAT START   TIME COMMAND
  tobias      3233 43.4  0.4 1596512 37096 pts/1   Sl+  21:21   2:24 ./s_go 10000

Benchmark results:

===== ======== ========== ========= ========== ====== ======= ======= ======= ====== ====== ======= ======= ======= ======= ====== ===== ====== ====== ====== ====== ====== =====
conns requests kBytes     requests  kBytes     latncy latency latency latency latncy latncy latency latency latency latency kBytes time  error  error  error  error  error  perc 
      total    total      second    second     min    max     average stddev  30%    50%    75%     90%     99%     99.99%  req    total conns  reads  writes timeou status averg
===== ======== ========== ========= ========== ====== ======= ======= ======= ====== ====== ======= ======= ======= ======= ====== ===== ====== ====== ====== ====== ====== =====
50(c) 6649     411517.70  1303.84   80696.79   3.48   61.78   31.00   5.95    29.98  32.16  34.20   36.26   41.60   61.78   61.89  5.10  0      0      0      0      0      37   
1     7184     443087.24  359.15    22151.07   1.67   4.26    2.24    0.23    2.15   2.22   2.31    2.41    3.36    4.26    61.68  20.00 0      0      0      0      0      55   
5     22458    1385186.56 1117.32   68914.93   1.70   8.20    2.85    0.49    2.57   2.78   3.10    3.49    4.33    7.29    61.68  20.10 0      0      0      0      0      56   
10    26645    1643401.94 1325.69   81765.22   1.94   12.40   4.75    0.98    4.28   4.61   5.12    5.93    8.04    11.79   61.68  20.10 0      0      0      0      0      58   
50    26113    1610988.59 1299.22   80152.81   4.31   76.59   31.54   4.85    30.45  32.39  34.23   36.11   40.43   62.44   61.69  20.10 0      0      0      0      0      39   
100   25908    1600968.40 1294.37   79984.58   19.83  287.23  65.67   8.60    63.07  66.58  69.94   73.34   87.09   209.95  61.79  20.02 0      0      0      0      0      43   
200   24777    1536293.54 1238.12   76769.69   35.51  621.68  143.95  40.05   130.85 138.78 148.02  176.24  304.39  586.07  62.00  20.01 0      0      0      0      0      65   
500   22068    1373968.64 1098.35   68384.06   53.70  1992.75 380.94  187.89  271.27 280.24 434.50  627.64  1094.14 1939.89 62.26  20.09 0      0      0      7      0      68   
800   19086    1198598.43 951.99    59784.63   91.68  1996.79 673.50  303.01  433.84 589.88 813.38  1109.92 1668.84 1996.43 62.80  20.05 0      0      0      185    0      60   
1000  17785    1122480.86 886.26    55935.11   147.74 1999.90 888.22  357.94  595.38 805.81 1101.70 1421.56 1893.05 1999.62 63.11  20.07 0      0      0      659    0      58   
===== ======== ========== ========= ========== ====== ======= ======= ======= ====== ====== ======= ======= ======= ======= ====== ===== ====== ====== ====== ====== ====== =====

