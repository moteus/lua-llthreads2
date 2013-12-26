local lua_version_t
local function lua_version()
  if not lua_version_t then 
    local version = rawget(_G,"_VERSION")
    local maj,min = version:match("^Lua (%d+)%.(%d+)$")
    if maj then                         lua_version_t = {tonumber(maj),tonumber(min)}
    elseif not math.mod then            lua_version_t = {5,2}
    elseif table.pack and not pack then lua_version_t = {5,2}
    else                                lua_version_t = {5,2} end
  end
  return lua_version_t[1], lua_version_t[2]
end

local LUA_MAJOR, LUA_MINOR = lua_version()
local IS_LUA_51 = (LUA_MAJOR == 5) and (LUA_MINOR == 1)
local IS_LUA_52 = (LUA_MAJOR == 5) and (LUA_MINOR == 2)

local LUA_INIT = "LUA_INIT"
local LUA_INIT_VER
if not IS_LUA_51 then
  LUA_INIT_VER = LUA_INIT .. "_" .. LUA_MAJOR .. "_" .. LUA_MINOR
end

LUA_INIT = LUA_INIT_VER and os.getenv( LUA_INIT_VER ) or os.getenv( LUA_INIT ) or ""

LUA_INIT = [[do 
  local lua_init = ]] .. ("%q"):format(LUA_INIT) .. [[
  if lua_init and #lua_init > 0 then
    if lua_init:sub(1,1) == '@' then
        dofile(lua_init:sub(2))
    else
        assert((loadstring or load)(lua_init))()
    end
  end
end;]]

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

return {
  thread_init = LUA_INIT,
  sleep       = sleep,
}
