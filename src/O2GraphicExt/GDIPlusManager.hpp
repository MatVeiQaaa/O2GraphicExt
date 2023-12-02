#pragma once

namespace GDIPlusManager {
	/*
		Init GDI+ to able use any GDI+ classes or any image manipulation
	*/
	bool Init();

	/*
		Clear any GDI+ resources to prevent switch screen lag
	*/
	bool Deinit();
}