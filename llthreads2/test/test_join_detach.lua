local llthreads = require"llthreads"
local utils     = require "utils"

do

local thread = llthreads.new(utils.thread_init .. [[
  local sleep = require"utils".sleep
  while true do sleep(1) end
]])

-- detached + joindable
thread:start(true, true)

local ok, err = thread:join(0)
print("thread:join(0): ", ok, err)
assert(ok == nil)
assert(err == "timeout")

end

-- enforce collect `thread` object
-- we should not hungup
for i = 1, 10 do collectgarbage("collect") end


do

local thread = llthreads.new(utils.thread_init .. [[
  local sleep = require"utils".sleep
  sleep(1)
]])

-- detached + joindable
thread:start(true, true)

local ok, err = thread:join(0)
print("thread:join(0): ", ok, err)
assert(ok == nil)
assert(err == "timeout")

for i = 1, 12 do
  utils.sleep(5)
  ok, err = thread:join(0)
  print("thread:join(0)#" .. i .. ": ", ok, err)
  if ok then break end
  assert(err == 'timeout')
end

assert(ok)

end

-- enforce collect `thread` object
-- we should not get av
for i = 1, 10 do collectgarbage("collect") end


print("Done!")

