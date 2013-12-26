lua-llthreads2
==============
[![Build Status](https://travis-ci.org/moteus/lua-llthreads2.png?branch=master)](https://travis-ci.org/moteus/lua-llthreads2)

This is full dropin replacement for [llthreads](https://github.com/Neopallium/lua-llthreads) library.

##Incompatibility list with origin llthreads library
* does not support Lua 5.0
* does not support ffi interface (use Lua C API for LuaJIT)
* returns nil instead of false on error
* start method returns self instead of true on success
* does not open all standart libraries (set LLTHREAD_REGISTER_STD_LIBRARY to on this feature)
* register loaders for llthreads library itself

##Additional
* thread:join() method support zero timeout to check if thread alive
* thread:join() method support arbitrary timeout on Windows platform
* set_logger function allow logging errors (crash Lua VM) in current llthread's threads

[![Bitdeli Badge](https://d2weczhvl823v0.cloudfront.net/moteus/lua-llthreads2/trend.png)](https://bitdeli.com/free "Bitdeli Badge")


##Usage

### Use custom logger
In this example I use [lua-log](https://github.com/moteus/lua-log) library.
``` Lua
-- This is child thread.
local llthreads = require "llthreads"
-- Send logs using ZMQ
local LOG = require"log".new(
  require "log.writer.net.zmq".new("tcp://127.0.0.1:5555")
)
llthread.set_logger(function(msg) LOG.error(msg) end)

...

-- This error with traceback will be passed to logger
error("SOME ERROR")
```
