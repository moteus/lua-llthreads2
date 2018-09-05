local llthreads = require"llthreads"
local utils     = require "utils"

local include = utils.thread_init .. [[
local llthreads = require"llthreads"
]]

do
    local thread = llthreads.new(include .. [[
      error({})
    ]])
    
    thread:start()
    local ok, err = thread:join()
    print(ok, err)
    assert(not ok)
end
do
    local thread = llthreads.new(include .. [[
      llthreads.set_logger(function(msg) print("XXX", msg) end)
      error({})
    ]])
    
    thread:start()
    local ok, err = thread:join()
    print(ok, err)
    assert(not ok)
end
do
    local thread = llthreads.new(include .. [[
      llthreads.set_logger(function(msg) end)
      error({})
    ]])
    
    thread:start()
    local ok, err = thread:join()
    print(ok, err)
    assert(not ok)
end
print("Done!")

