#include "gameWindow.h"
#include <iostream>

struct WindowData {
	DWORD targetPid;
	HWND foundHwnd;
};

static BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam) {
	WindowData* data = (WindowData*)lParam;
	DWORD windowPid = 0;
	GetWindowThreadProcessId(hwnd, &windowPid);

	if (windowPid == data->targetPid) {
		if (GetWindow(hwnd, GW_OWNER) == NULL) {
			data->foundHwnd = hwnd;
			return FALSE;  // 找到即停止
		}
	}
	return TRUE;
}

static HWND GetHwndByPid(DWORD pid) {
	WindowData data;
	data.targetPid = pid;
	data.foundHwnd = NULL;
	EnumWindows(EnumWindowsCallback, (LPARAM)&data);
	return data.foundHwnd;
}

// ---- GameWindow 类成员函数实现 ----

GameWindow::GameWindow(DWORD pid) {
	m_hwnd = GetHwndByPid(pid);

	if (!m_hwnd || !IsWindow(m_hwnd)) {
		std::cout << "未找到 PID 为 " << pid << " 的窗口！" << std::endl;
	} else {
		GetClientRect(m_hwnd, &m_rect);
		m_width = m_rect.right - m_rect.left;
		m_height = m_rect.bottom - m_rect.top;

		m_hdcWindow = GetDC(m_hwnd);
		m_hdcMem = CreateCompatibleDC(m_hdcWindow);
		m_hBitmap = CreateCompatibleBitmap(m_hdcWindow, m_width, m_height);
		m_hOldBitmap = (HBITMAP)SelectObject(m_hdcMem, m_hBitmap);
	}
}

bool GameWindow::saveTo(const char* filename) {
	bool success{false};

	// 标志 1 (PW_CLIENTONLY) 表示只打印客户区，自动剔除标题栏和边框
	BOOL printed = PrintWindow(m_hwnd, m_hdcMem, 1);

	if (!printed) {
		std::cout << "PrintWindow 截取失败！" << std::endl;
		return success;
	}

	BITMAP bmp;
	GetObject(m_hBitmap, sizeof(BITMAP), &bmp);

	BITMAPINFOHEADER bi = {};
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmp.bmWidth;
	bi.biHeight = bmp.bmHeight;  // 正数表示 Bottom-up DIB
	bi.biPlanes = 1;
	bi.biBitCount = 32;  // 32位色
	bi.biCompression = BI_RGB;

	DWORD dwBmpSize = ((bmp.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmp.bmHeight;
	HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
	char* lpbitmap = (char*)GlobalLock(hDIB);

	GetDIBits(m_hdcMem, m_hBitmap, 0, (UINT)bmp.bmHeight, lpbitmap, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

	HANDLE hFile = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		DWORD dwSizeNeeded = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize;

		BITMAPFILEHEADER bmfHdr = {};
		bmfHdr.bfType = 0x4D42;  // "BM"
		bmfHdr.bfSize = dwSizeNeeded;
		bmfHdr.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		DWORD dwWritten = 0;
		WriteFile(hFile, &bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
		WriteFile(hFile, &bi, sizeof(BITMAPINFOHEADER), &dwWritten, NULL);
		WriteFile(hFile, lpbitmap, dwBmpSize, &dwWritten, NULL);
		CloseHandle(hFile);

		std::cout << "已成功保存纯客户区截图到: " << filename << " (分辨率: " << m_width << "x" << m_height << ")" << std::endl;
		success = true;
	}

	GlobalUnlock(hDIB);
	GlobalFree(hDIB);

	return success;
}

Color GameWindow::getColorAt(int logicX, int logicY) {
	BOOL printed = PrintWindow(m_hwnd, m_hdcMem, 1);

	Color color{};
	COLORREF color_ref = CLR_INVALID;

	if (printed) {
		color_ref = GetPixel(m_hdcMem, logicX, logicY);
	}

	// 注意：这里用 color_ref != CLR_INVALID 判断更严谨（因为纯黑色 RGB(0,0,0) 的 color_ref 也是 0）
	if (color_ref != CLR_INVALID) {
		color.r = GetRValue(color_ref);
		color.g = GetGValue(color_ref);
		color.b = GetBValue(color_ref);
	}

	return color;
}

// ---- 新增的公共函数：直接输出 cv::Mat 供 OpenCV 和 OCR 使用 ----
cv::Mat GameWindow::CaptureMat() {
	if (!m_hwnd || !IsWindow(m_hwnd)) {
		return cv::Mat();
	}

	// 1. 打印窗口画面到兼容 DC (PW_CLIENTONLY 自动剔除标题栏和边框)
	BOOL printed = PrintWindow(m_hwnd, m_hdcMem, 1);
	if (!printed) {
		std::cout << "PrintWindow 截取失败！" << std::endl;
		return cv::Mat();
	}

	BITMAP bmp;
	GetObject(m_hBitmap, sizeof(BITMAP), &bmp);

	// 2. 配置 DIB 信息头
	BITMAPINFOHEADER bi = {};
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmp.bmWidth;
	bi.biHeight = -bmp.bmHeight;  // 【关键技巧】设为负数表示 Top-down，免去后续 cv::flip 翻转
	bi.biPlanes = 1;
	bi.biBitCount = 32;  // 32位色 (BGRA)
	bi.biCompression = BI_RGB;

	// 3. 直接创建一个相同大小的 cv::Mat (4通道 BGRA)
	cv::Mat mat(bmp.bmHeight, bmp.bmWidth, CV_8UC4);

	// 4. 将像素数据直接读取到 cv::Mat 的内存缓冲区中
	int lines = GetDIBits(m_hdcMem, m_hBitmap, 0, (UINT)bmp.bmHeight, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
	if (lines == 0) {
		std::cout << "GetDIBits 获取像素失败！" << std::endl;
		return cv::Mat();
	}

	return mat;
}

GameWindow::~GameWindow() {
	if (m_hdcMem) {
		SelectObject(m_hdcMem, m_hOldBitmap);
	}
	if (m_hBitmap) {
		DeleteObject(m_hBitmap);
	}
	if (m_hdcMem) {
		DeleteDC(m_hdcMem);
	}
	if (m_hwnd && m_hdcWindow) {
		ReleaseDC(m_hwnd, m_hdcWindow);
	}
}
