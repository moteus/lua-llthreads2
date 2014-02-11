if jit then
  local llthreads = require "llthreads"
  local thread = llthreads.new([[
    if not package.preload.ffi then
      print("ffi does not register in thread")
      os.exit(-1)
    end
    local ok, err = pcall(require, "ffi")
    if not ok then
      print("can not load ffi: ", err)
      os.exit(-2)
    end
  ]]):start():join()
end
