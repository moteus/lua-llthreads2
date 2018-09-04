local llthreads = require"llthreads"
local utils     = require "utils"

local include = utils.thread_init .. [[
local llthreads = require"llthreads"
]]

local thread = llthreads.new(include .. [[
  error({})
]])

thread:start()
local ok, err = thread:join()
print(ok, err)
assert(not ok)
print("Done!")

