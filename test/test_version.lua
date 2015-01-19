local llthreads = require"llthreads"

local a,b,c,d = llthreads.version()

local str = ("%d.%d.%d"):format(a,b,c)
if d then str = str .. "-" .. d end

assert(str == llthreads._VERSION,
  tostring(str) .. " is not eaqual to " .. tostring(llthreads._VERSION)
)

print("Done!")

