-- Test if you build module with
-- LLTHREAD_REGISTER_THREAD_LIBRARY

local llthreads = require "llthreads"
local thread = llthreads.new([[
    if not package.preload.llthreads then
      print("llthreads does not register in thread")
      os.exit(-1)
    end
    local ok, err = pcall(require, "llthreads")
    if not ok then
      print("can not load llthreads: ", err)
      os.exit(-2)
    end
]]):start():join()
