#include "GDIPlusManager.hpp"
#include <Windows.h>
#include <gdiplus.h>
#include <mutex>
#include <iostream>

namespace GDIPlusManager {
	ULONG_PTR gdiplusToken;
	std::mutex glock;

	bool Init() {
		std::lock_guard<std::mutex> lock(glock);
		gdiplusToken = NULL;

		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != Gdiplus::Ok) {
			std::cout << "Failed to init GDI+" << std::endl;
			return false;
		}

		return true;
	}

	bool Deinit() {
		std::lock_guard<std::mutex> lock(glock);

		Gdiplus::GdiplusShutdown(gdiplusToken);

		return true;
	}
}