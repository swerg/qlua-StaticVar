require("StaticVar")

message("-- StaticVar-tests.lua --")

stv.SetVar("xxx", "xxx-test-value")
stv.SetVar("xxx-num", 392)

stv.UseNameSpace("testNS")

stv.SetVar("xxxNS", "xxxNS-test-value")
stv.SetVar("xxxNS-num", 529)

function main()
    sleep(50)

    stv.UseNameSpace()

    test("xxx", "xxx-test-value")
    test("xxx-num", 392)

    test("xxxNS-num", nil)

    stv.UseNameSpace("testNS")

    test("xxxNS", "xxxNS-test-value")
    test("xxxNS-num", 529)

    test("xxx2", nil)
end

function test(name, val)
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
    message("'" .. name .. "' ok")
end