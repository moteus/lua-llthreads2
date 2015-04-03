local thread_code = function(...)
  local function assert_equal(name, a, b, ...)
    if a == b then return b, ... end
    print(name .. " Fail! Expected `" .. tostring(a) .. "` got `" .. tostring(b) .. "`")
    os.exit(1)
  end

  local a,b,c,d = ...
  assert_equal("1:", 1, a )
  assert_equal("2:", 2, b )
  assert_equal("3:", 3, c )
  assert_equal("4:", 4, d )
  assert_equal("#:", 4, select("#", ...))
end

local llthreads = require"llthreads.ex"

local prelude1 = function(...) return 1, ... end

local prelude2 = function(...) return 2, ... end

local prelude = string.format([[
  local loadstring = loadstring or load
  local prelude1 = loadstring(%q)
  local prelude2 = loadstring(%q)
  return prelude1(prelude2(...))
]], string.dump(prelude1), string.dump(prelude2))

-- pass `prelude` function that change thread arguments
local thread = llthreads.new({thread_code, prelude = prelude}, 3, 4)

local a = assert(thread:start())

assert(thread:join())

assert(a == thread)

print("done!")
