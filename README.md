lua-llthreads2
==============
[![Build Status](https://travis-ci.org/moteus/lua-llthreads2.png?branch=master)](https://travis-ci.org/moteus/lua-llthreads2)

This is full dropin replacement for [llthreads](https://github.com/Neopallium/lua-llthreads) library.

##Incompatibility list with origin llthreads library
* does not support Lua 5.0
* does not support ffi interface (use Lua C API for LuaJIT)
* returns nil instead of false on error
* start method returns self instead of true on success

##Additional
* thread:join() method support zero timeout to check if thread alive (does not work on Windows with pthreads)
* thread:join() method support arbitrary timeout on Windows threads
* set_logger function allow logging errors (crash Lua VM) in current llthread's threads
* thread:start() has additional parameter which control in which thread child Lua VM will be destroyed
* allow pass cfunctions to child thread (e.g. to initialize Lua state) (experemental)

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
-- This error with traceback will be passed to logger
error("SOME ERROR")
```

### Start atached thread collectd in child thread
``` Lua 
-- This is main thread.
local thread = require "llthreads".new[[
  require "utils".sleep(5)
]]

-- We tell that we start atached thread but child Lua State shuld be close in child thread. 
-- So thread:join() can not return any Lua values.
-- If `thread` became garbage in main thread then finallizer calls thread:join() 
-- and main thread may hungup.
thread:start(false, false)

-- we can call join
thread:join()
```

### Start detached thread on which we can call join
``` Lua 
-- This is main thread.
local thread = require "llthreads".new[[
  require "utils".sleep(5)
]]

-- We tell that we start detached thread but with ability call thread:join() and 
-- gets lua return values from child thread. In fact we start atached thread but if `thread` 
-- became garbage in main thread then finallizer just detach child thread and main thread
-- may not hungup.
thread:start(true, true)

-- we can call join
thread:join()
```

### Pass to child thread host application`s library loader
If you close parent Lua state then some dynamic library may be unloaded
and cfunction in child Lua state (thread) became invalid.

``` Lua 
-- `myhost.XXX` modules is built-in modules in host application
-- host application registers cfunction as module loader
local preload = {}
preload[ 'myhost.logger' ] = package.preload[ 'myhost.logger' ]
preload[ 'myhost.config' ] = package.preload[ 'myhost.config' ]
llthreads.new([[
  -- registers preload
  local preload  = ...
  for name, fn in pairs(preload) do package.preload[name] = fn end

  local log = require 'myhost.logger'

]], preload):start(true)
```

[![Bitdeli Badge](https://d2weczhvl823v0.cloudfront.net/moteus/lua-llthreads2/trend.png)](https://bitdeli.com/free "Bitdeli Badge")

