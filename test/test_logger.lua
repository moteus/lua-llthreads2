local utils = require "utils"

require "llthreads".new(utils.thread_init .. [[
require "string"

require "llthreads".set_logger(function(msg)
  if type(msg) ~= 'string' then
    print("ERROR! Invalid error message: ", msg)
    os.exit(-2)
  end
  if not msg:find("SOME ERROR", nil, true) then
    print("ERROR! Invalid error message: ", msg)
    os.exit(-1)
  end
  print("Done!")
  os.exit(0)
end)

error("SOME ERROR")
]]):start():join()

print("ERROR! Logger has not been call!")
os.exit(-1)

