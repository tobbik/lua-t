HTTP Performance Tests and Examples
+++++++++++++++++++++++++++++++++++

Overview
--------

The goal for the HTTP Server functionality in lua-t was to provide the basic
functionality and get a reasonable performance without putting a serious
strain on the system.  Keeping the memory consumption low was the formost
concern over both speed and extra functionality.  When comparing to existing
platforms it comes closest to NodeJS by being single-threaded, asynchronous
and based on a scripting language.  NodeJS utilizes the amazing capabilities
of the V8 VM and therefore shall be faster but also use more memory.  It was
never the goal to match NodeJS in both speed and functionality.  A go
implementation is also included to show the differences.


Approach for larger payloads
----------------------------

The server will compose a string of a certain lenght and the write it out to
the socket 


Approach for smaller payloads
-----------------------------

Seeing a lot of benchmarks online which merely echo back a payload or just
respond with a status code I felt that the server side should do at least a
tiny amount of processing to come closer to realistic scenarios.  So instead
of doing only trivial things the HTTP server should:

 - parse URL for path and decide which logic to use
 - parse query string for values
 - remove unwanted characters
 - rot47 the 'password' portion and compare it to a previously stored
   password
 - formulate an answer based on what was found
 - handle 404 for non found path
 - handle 400 for Bad Request( unknown user )
 - handle 401 for bad password
 - handle 200 success for "authentication"

It attempts to rely on *implemented functionality* instead of factoring it
out to native crypt style libraries.  This keeps it more comparable.


ROT47, really???
----------------

Firstly, this is sample code for the sake of benchmarking it.  NEVER, I
repeat **NEVER**, use this in production.  The original inspiration for the
code was lifted of `Easy profiling for Node.js Applications
<https://nodejs.org/en/docs/guides/simple-profiling/>`_.  Now that code uses
``crypto.pbkdf2Sync()`` which is a native implementation that parallalizes
the hashing process with up to ``n`` available CPU cores.  While it probably
would be possible to fudge something like this in a (lua) library, it is not
realistic.  By implementing ROT47 natively in each language it becomes, in
my opinion, more comparable.


So what is it actually comparing?
---------------------------------

When it comes to HTTP benchmarks it really tests much more than the language
or the platform.  It tests the entire eco system.  Here is the explaination
why this benchamrk is flawed:

 1. NodeJS *can* parallelize some stuff but the end-user has only marginal
    control over it.  lua-t can not.  So keeping the workload basically
    single threaded makes it fairer and more comparable.
 2. Go makes using multiple CPUs through goroutines almost too easy.
    Really, it would feel awkward to not use them in this scenario.  That
    means the Go example will come out ahead of everything else by a margin
    or even a factor that is determined by the number of CPU cores available
    at the system where the benchmark is executed.  This advantage is well
    deserved because Go makes it that easy!
 3. Writing any code in Lua (in the context of this example) that would take
    advantage of multiple CPU cores would have to be explicit and it would
    make the code clunky.


Implementations
---------------

The Http servers have been implemented in 3 languages:

 1. JavaScript (for NodeJS)
 2. Go(for Go std library)
 3. Lua5.4 (for lua-t library)

After the servers get primed( creating users(s) via HTTP requests), they get
tortured with ``wrk``.  There will be a number (100000ish) of requests run
to prime any system resources (buffers etc) and, where applicable, JIT
compilers in the VM.  All servers get benchmarked at several concurrency 
levels, all using either ``Connection: keep-alive`` OR ``Connection: close``
mode.


Setup and Procedure
-------------------

The tests will be run over a fast network connection since the benchmarking
tool would gobble up resources that effect the server.  Using this logic,
benchmarking the server over a sufficiently fast network would yield better
results than running the benchamrk tool agains localhost where both the
server and the benchmarking tool would compete for resources such as CPU
cycles and network connections from the kernel.  To provide meaningful
resluts the tests are performed on two very small machines, a RaspberryPi
4(the first with GigaBit ethernet) and a NUC that is equipped with an
ancient AMD A5000 APU.  These have a hard time to saturate a Gigabit
ethernet connection.

Each test series will be exectued by starting to primed with a run of 5
seconds @50 concurrency to warm up the network stack and where applicable
the garbage collector and the JIT compiler.  Then the ``wrk`` command gets
issued with increasing numbers of concurrnt connections up to 1000.  After
each run of ``wrk`` there is a cooldown period allowing for the OS to
collect stale connectors etc.

There were sporadic ``ps aux`` snapshots taken on the system running the
server to get a feel for memory consumption. These were always timed for the
worst case scenario, meaning it was always taken when the benchamrk with the
highests concurrent connections was running.

Each type of test is conducted in both keep-alive and single connection
mode. The single connection test more or less shows how the server interacts
with the systems network stack because a lot of time is spent accepting and
closing sockets.  This is essentially network abuse but nevertheless a valid
possible scenario.  Note that the values regarding the concurrentcy levels
are all over the place and it is rather hard to interpret.  Also these
measurements are snapshots.  The next time the exact benchmark is repeated
the values will vary more than for the ``keep-alive`` scenario.  This is
true for all three servers.  The ``wrk`` command here gets called with the
``--script report.lua`` argument.  This file sets
``wrk.headers['Connection'] = "close"`` to achieve the desired behaviour.


Conclusion
----------

It's bad karma to conclude your own benchmarks on your own code ... so here
are some thoughts.  I'm more than pleased to see these numbers on lua-t
performance and I never had expected to see this happening.  While it's
awesome that the sheer numbers suggest that lua-t *could* go head to head
with nodeJS I wouldn't go as far as saying so.  Here is why: When more code
needs to be run nodeJS JIT will recoup a lot of performance.  Also nodeJS
has, to some extend, the ability to use multiple processors.  It's a mature
eco-system with a wealth of 3rd party libs.  I tried to make the benchmark a
little more complex than just receiving and responding but my guess is that
in real life situations nodeJS will beat lua-t in most aspects and that's
fine by me.  The true outcome of this benchmark is memory consumption.
lua-t keeps it extremely low and, more importantly, it keeps it constantly
low.  It makes it much more predictable on lower end hardware and that's
exactly where I see it being used.

As far as the Go code goes, yes it's blazingly fast, however, the binary is
about eight times the size of the Lua interpreter and ALL of lua-t libs
combined. The memory consumption however is significantly less than nodeJS.
There is no question, Go is made for the bigger iron, when speed is the most
important feature I wouldn't hesitate to use it.  It scales over multiple
CPU-cores without any efforts and that makes it awesome!
