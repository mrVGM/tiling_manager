active_windows = {}

function enum_windows()
    return coroutine.create(
        function()
            for k,v in pairs(active_windows) do
                if v ~= nil and v.minimized == false then
                    coroutine.yield(k)
                end
            end
        end
    )
end

function get_most_recent(before)
    local co = enum_windows()
    local _, cur = coroutine.resume(co)
    local res = nil

    while coroutine.status(co) ~= 'dead' do
        if before == nil or cur < before then
            if res == nil or cur > res then res = cur end
        end
        _, cur = coroutine.resume(co)
    end

    return res
end

function enum_priority()
    return coroutine.create(
        function ()
            local cur = get_most_recent(nil)
            while cur ~= nil do
                coroutine.yield(cur)
                cur = get_most_recent(cur)
            end
        end
    )
end

function get_windows_count()
    local co = enum_windows()
    local count = 0

    coroutine.resume(co)
    while coroutine.status(co) ~= 'dead' do
        count = count + 1
        coroutine.resume(co)
    end

    return count
end

reposition = function ()
	local count = get_windows_count()

	if count == 0 then
		return
	end

	local width = math.floor(1920 / count)
	local height = 1000

	local i = 0
    local co = enum_priority()
    local _, k = coroutine.resume(co)

    while k ~= nil do
        position_window(k, i * width, 0, width, height)
        i = i + 1
        _, k = coroutine.resume(co);
    end
end

 function window_created(id)
	log("Window with id " .. id .. " created!")
	active_windows[id] = {
        id = id,
        minimized = false
    }

	reposition()
end

 function window_destroyed(id)
	log("Window with id " .. id .. " destroyed!")

	active_windows[id] = nil

	reposition()
end

function window_minimized(id)
    active_windows[id].minimized = true
    reposition()
end

function window_restored(id)
    active_windows[id].minimized = false
    reposition()
end

