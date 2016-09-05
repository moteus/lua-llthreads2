-- only since Lua 5.3
if math.type then

  local thread_code = function(...)
    local function assert_equal(name, a, b, ...)
      if a == b then return b, ... end
      print(name .. " Fail! Expected `" .. tostring(a) .. "` got `" .. tostring(b) .. "`")
      os.exit(1)
    end

    local a,b = ...
    assert_equal("1:", 'integer', math.type(a))
    assert_equal("2:", 'float', math.type(b))
  end

  local llthreads = require"llthreads.ex"

  local thread = llthreads.new(thread_code, 10, 20.0 )

  assert(thread:start())

  assert(thread:join())
end

print("done!")