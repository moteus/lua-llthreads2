local llthreads = require"llthreads"
local utils     = require "utils"
local sleep     = utils.sleep

local include = [[
local llthreads = require"llthreads"
]] .. utils.thread_init .. [[ 
local sleep = require "utils".sleep
]]

local thread = llthreads.new(include .. [[
  sleep(5)
]])
thread:start()
local ok, err = thread:join(0)
print("thread:join(0): ", ok, err)
assert(ok == nil)
assert(err == "timeout")

print("thread:join(): ", thread:join())
print("Done!")

