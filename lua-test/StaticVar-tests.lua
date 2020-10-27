if _VERSION == "Lua 5.3" then
  stv = require("StaticVar")
else
  require("StaticVar")
end

message("-- StaticVar-tests.lua --")

stv.SetVar("xxx", "xxx-test-value")
stv.SetVar("xxx-num", 392)
stv.SetVar("xxx-int64", 123456789012345678)

stv.UseNameSpace("testNS")

stv.SetVar("xxxNS", "xxxNS-test-value")
stv.SetVar("xxxNS-num", 529)
stv.SetVar("xxxNS-dbl", 87657.39)

function main()
    sleep(50)

    stv.UseNameSpace()

    test("xxx", "xxx-test-value")
    test("xxx-num", 392)
    test("xxx-int64", 123456789012345678)

    test("xxxNS-num", nil)

    stv.UseNameSpace("testNS")

    test("xxxNS", "xxxNS-test-value")
    test("xxxNS-num", 529)
    test("xxxNS-dbl", 87657.39)

    test("xxx2", nil)
end

function test(name, val)
    -- trick to distinguish between float and integer (int64) in Lua5.3
    local g_type = type  -- save reference to global type() function
    local type = g_type  -- init to global type() function (for Lua < 5.3)
    if _VERSION == "Lua 5.3" then
      type = function(v)  -- redefine !local! variable here!
        if g_type(v) == "number" then
          return math.type(v)
        else
          return g_type(v)
        end
      end
    end

    local v = stv.GetVar(name)
    if type(v) ~= type(val) then
        message("'" .. name .. "' (type:" .. tostring(type(val)) .. ") has invalid type: " .. tostring(type(v)))
        return
    end
    if not (val == nil) then
        if v ~= val then
            message("'" .. name .. "' (type:" .. tostring(type(val)) .. ") has invalid value: " .. tostring(v))
            return
        end
    end
    if _VERSION == "Lua 5.3" then
        -- check int64
        -- problem: type returns 'number' for float and integer values
    end
    message("'" .. name .. "' ok")
end
