window_created = function (id)
	log("Window with id " .. id .. " created!")

	position_window(id, 0, 0, 500, 500)
end

window_destroyed = function (id)
	log("Window with id " .. id .. " destroyed!")
end

