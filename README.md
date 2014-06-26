lua-llthreads2
==============
[![Build Status](https://travis-ci.org/moteus/lua-llthreads2.png?branch=master)](https://travis-ci.org/moteus/lua-llthreads2)
[![Build Status](https://buildhive.cloudbees.com/job/moteus/job/lua-llthreads2/badge/icon)](https://buildhive.cloudbees.com/job/moteus/job/lua-llthreads2/)
[![Build Status](https://moteus.ci.cloudbees.com/job/lua-llthreads2/badge/icon)](https://moteus.ci.cloudbees.com/job/lua-llthreads2/)

This is full dropin replacement for [llthreads](https://github.com/Neopallium/lua-llthreads) library.

##Incompatibility list with origin llthreads library
* does not support Lua 5.0
* does not support ffi interface (use Lua C API for LuaJIT)
* returns nil instead of false on error
* start method returns self instead of true on success

##Additional
* thread:join() method support zero timeout to check if thread alive (does not work on Windows with pthreads)
* thread:join() method support arbitrary timeout on Windows threads
* thread:alive() method return whether the thread is alive (does not work on Windows with pthreads)
* set_logger function allow logging errors (crash Lua VM) in current llthread's threads
* thread:start() has additional parameter which control in which thread child Lua VM will be destroyed
* allow pass cfunctions to child thread (e.g. to initialize Lua state)

##Thread start arguments
| default | `detached` | `joinable` | join returns | child state closes by | gc calls | detach on |
|:-------:|:----------:|:----------:|:------------:|:---------------------:|:--------:|:---------:|
|         |    false   |    false   | `true`       |         child         |  join    | `<NEVER>` |
|    *    |    false   |    true    | Lua values   |         parent        |  join    | `<NEVER>` |
|    *    |    true    |    false   | raise error  |         child         | `<NONE>` |   start   |
|         |    true    |    true    | `true`       |         child         | detach   |    gc     |


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

### Start attached thread collectd in child thread
``` Lua 
-- This is main thread.
local thread = require "llthreads".new[[
  require "utils".sleep(5)
]]

-- We tell that we start attached thread but child Lua State shuld be close in child thread. 
-- If `thread` became garbage in main thread then finallizer calls thread:join() 
-- and main thread may hungup. But because of child state closes in child thread all objects
-- in this state can be destroyed and we can prevent deadlock.
thread:start(false, false)

-- We can call join.
-- Because of Lua state destroys in child thread we can not get 
-- returned Lua vaules so we just returns `true`.
thread:join()
```

### Start detached joinable thread
``` Lua 
-- This is main thread.
local thread = require "llthreads".new[[
  require "utils".sleep(5)
]]

-- We tell that we start detached joinable thread. In fact we start attached 
-- thread but if `thread` became garbage in main thread then finallizer just 
-- detach child thread and main thread may not hungup.
thread:start(true, true)

-- We can call join.
-- Because of Lua state destroys in child thread we can not get 
-- returned Lua vaules so we just returns `true`.
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

### Wait while thread is alive
``` Lua 
local thread = require "llthreads".new[[
  require "utils".sleep(5)
  return 1
]]
thread:start()

-- we can not use `thread:join(0)` because we can not call it twice
-- so all returned vaules will be lost
while thread:alive() do 
  -- do some work
end

local ok, ret = thread:join() -- true, 1
```

### Use `ex` module
``` Lua 
local Threads = require "llthreads.ex"

local ok, v = Threads.new(function()
  return 1
end):start():join()
assert(v == 1)

local thread = Threads.new({
  -- this thread code gets changed arguments
  function(a, b)
    assert(1 == a)
    assert(2 == b)
    print("Done")
  end;
  
  -- prelude can change thread arguments
  prelude = function(a, b)
    assert("1" == a)
    assert(nil == b)
    return tonumber(a), 2
  end;
}, "1")

thread:start():join()
```

[![Bitdeli Badge](https://d2weczhvl823v0.cloudfront.net/moteus/lua-llthreads2/trend.png)](https://bitdeli.com/free "Bitdeli Badge")

