active_windows = {}


reposition = function ()
	local count = 0
	for k,v in pairs(active_windows) do 
		if v ~= nil then
			count = count + 1
		end
	end

	log("count " .. count)

	if count == 0 then
		return
	end

	local width = math.floor(1920 / count)
	local height = 1000

	local i = 0
	for k,v in pairs(active_windows) do
		position_window(k, i * width, 0, width, height)
		i = i + 1
	end
end

window_created = function (id)
	log("Window with id " .. id .. " created!")
	active_windows[id] = id;

	reposition()
end

window_destroyed = function (id)
	log("Window with id " .. id .. " destroyed!")

	active_windows[id] = nil

	reposition()
end

