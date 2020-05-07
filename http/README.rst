HTTP Performance Tests and Examples
+++++++++++++++++++++++++++++++++++

Overview
--------

When writing the t_htp* and T.Http.* code it was never the goal to attempt
nodeJS speed or even go close to it's performance.  NodeJS uses a
sophisticated JIT compiling JavaScript VM that is worth a couple of
Megabytes, while the Lua VM is much simpler in this regard but is also much
smaller.  However, good HTTP performance is mainly dependent on good i/o
handling and thus it was possible to achieve good numbers.  The goal for
lua-t HTTP implementation was to provide usable functionality out of the box
conviniently available without writing a lot of custom code.  Performance
was from the beginning a secondary concern.


Approach
--------

Seeing a lot of benchmarks online which merely respond back as fast as
possible this was thought up to resemble a bit more of a realistic workload.
So instead of doing only trivial things the HTTP server should:

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

It attempts to rely on *written functionality* instead of factoring it out
to native crypt style libraries.  This keeps it somehwat fair and
comparable.

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
my opinion, more comparable.  Also, the used ROT47 implementations uses less
number crunching rather than hashtable lookups which should make things more
meaningful in terms of language comarisons.  Number crunching in HTTP
requests seems like a bad idea in general.


So what is it actually comparing?
---------------------------------

When it comes to HTTP benchmarks it really tests much more than the language
or the platform.  It tests the entire eco system.  Here is the explaination
why this benchamrk is flawed:

 1. NodeJS *can* parallelize some stuff but the enduser has hardly any
    control over it.  lua-t can not.  So keeping the workload basically
    single threaded makes it fairer and more comparable.
 2. Go makes using multiple CPUs through goroutines almost too easy.
    Really, it would feel awkward to not use them in this scenario.  That
    means the Go example will come out ahead of everything else by a margin
    or even factor that is determined by the number of CPU cores available
    at the system where the benchmark is executed.  This advantage is well
    deserved because Go makes it that easy!
 3. Writing any code in Lua (in the context of this example) that would take
    advantage of multiple CPU cores would have to be explicit and it would
    make the code clunky.
 4. Luvit!  The results don't make any sense to me and I think I have an
    error in installation or configuration.  That code should perform
    similar to nodeJS at least but it seems to be hampered by something.
    Don't use this to compare luvit to nodeJS until that was shown to the
    luvit folks and have a look at it.


Implementations
---------------

The Http servers have been implemented in 3 languages:

1. JavaScript (for NodeJS)
2. Go(for Go std library)
3. Lua5.3 (for lua-t library)
4. Luvit (basically Lua on a different VM)

After the servers get primed( creating users(s) via HTTP requests), they get
tortured with three different programs:

 - ab (no keepAlive for NodeJS, it interprets HTTP/1.0 and not the header)
 - wrt
 - siege

There will be a number (100000ish) of requests run to prime any system
resources (buffers etc) and, where applicable, JIT compilers in the VM.  All
tests get attempted in low(4), medium(40) and higher(400) concurrency, all
using either ``keepAlive`` and ``non-keepAlive`` mode.  Also tests will be
conducted on different computers.

 - Zotac mini computer with AMD A5000 (4x1.5GHz turbo, 8GB Ram)
 - Dell XPS15; Intel i7-4702MQ ( 2.2-3.6GHz, 16GB Ram)

The tests will be run over a fast network connection since the benchmarking
tool would gobble up resources that effect the server.  Using this logic,
benchmarking the server over a sufficiently fast network would yield better
results than running the benchamrk tool agains localhost.  The results will
only be true if this condition is met.

Compared will be the results of the benchmark program as well as memory
consumption during the test.


Conclusion
----------

It's bad karma to conclude your own benchmarks on your own code ... so here
are some thoughts.  I'm more than pleased to see these numbers on lua-t
performance and I never had expected to see this happening.  While it's
awesome that the sheer numbers suggest that lua-t can beat nodeJS I wouldn't
go as far as saying so.  Here is why: When more code needs to be run nodeJS
JIT will recoup quite some performance.  Also nodeJS has, to some extend,
the ability to use multiple processors.  It's a mature ecosystem with a
wealth of 3rd party libs.  I tried to make the benchmark a little more
complex than just receiving and responding but my guess is that in real life
situations nodeJS will beat lua-t in most aspects and that's fine by me.
The true outcome of this benchmark is memory consumption.  lua-t keeps it
extremely low and, more importantly, it keeps it constantly low.  It makes
it much more predictable on lower end hardware and that's exactly where I
see it being used.

As far as the Go code goes, yes it's blazingly fast, however, the binary is
about eight times the size of the Lua interpreter and ALL of lua-t libs
combined, the memory consumption is worse than nodeJS.  There is no
question, Go is made for the bigger iron, when speed is the most important
feature I wouldn't hesitate to use it.  It scales over multiple CPU-cores
without any efforts and that makes it awesome!
