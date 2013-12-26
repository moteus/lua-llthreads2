local llthreads = require"llthreads"

local sleep
local status, socket = pcall(require,"socket")
if status then
  sleep = function(secs)
    return socket.sleep(secs)
  end
end

if not sleep then
  local status, ztimer = pcall(require, "lzmq.timer")
  if status then
    sleep = function(secs)
      ztimer.sleep(secs * 1000)
    end
  end
end

if not sleep then
  sleep = function(secs)
    os.execute("sleep " .. tonumber(secs))
  end
end

local include = [[
local llthreads = require"llthreads"

local sleep
local status, socket = pcall(require,"socket")
if status then
  sleep = function(secs)
    return socket.sleep(secs)
  end
end

if not sleep then
  local status, ztimer = pcall(require, "lzmq.timer")
  if status then
    sleep = function(secs)
      ztimer.sleep(secs * 1000)
    end
  end
end

if not sleep then
  sleep = function(secs)
    os.execute("sleep " .. tonumber(secs))
  end
end
]]

local thread = llthreads.new(include .. [[
  sleep(5)
]])
thread:start()
local ok, err = thread:join(0)
print("thread:join(0): ", ok, err)
assert(ok == false)
assert(err == "timeout")

print("thread:join(): ", thread:join())
print("Done!")

