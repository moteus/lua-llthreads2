local llthreads = require"llthreads"
local utils     = require"utils"

local thread = llthreads.new(utils.thread_init .. [[
  require "llthreads"
  local fn = ...

  if type(fn) ~= 'function' then
    print("ERROR! No function : ", fn, type(fn))
    os.exit(-2)
  end

  fn("print('Done!'); require'os'.exit(0)"):start():join()
]], llthreads.new)

print(thread:start():join())
os.exit(-1)