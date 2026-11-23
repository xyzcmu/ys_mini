#ifndef GAME_WINDOW_H
#define GAME_WINDOW_H
#include <chrono>
#include <opencv2/opencv.hpp>
#include <windows.h>

struct Color {
	int r{-1};
	int g{-1};
	int b{-1};
};

struct EnergyBall {
	int id{};
	int x{};
	int y{};
	std::chrono::steady_clock::time_point lastEnableTime{};
	std::chrono::minutes duration{};
};

class GameWindow {
  private:
	HWND m_hwnd{nullptr};

	HDC m_hdcWindow{nullptr};
	HDC m_hdcMem{nullptr};
	HBITMAP m_hBitmap{nullptr};
	HBITMAP m_hOldBitmap{nullptr};

	// 窗口 client 区域尺寸
	RECT m_rect{};
	int m_width{};
	int m_height{};

  public:
	// 构造函数：根据 PID 初始化窗口并缓存资源
	GameWindow(DWORD pid);

	// 析构函数：释放所有 GDI 资源
	~GameWindow();

	/** 截取当前纯客户区画面并保存为 BMP 文件
	 * 未经过屏幕缩放的画面
	 */
	bool saveTo(const char* filename);

	/**
	 * 获取客户区指定坐标 (x, y) 的颜色
	 * @param logicX 未经过屏幕缩放后的逻辑坐标x
	 * @param logicY 未经过屏幕缩放后的逻辑坐标y
	 */
	Color getColorAt(int logicX, int logicY);

	// 【新增】获取游戏画面的 OpenCV 矩阵格式 (BGRA)
	cv::Mat CaptureMat();

	// 获取窗口句柄的辅助方法（可选）
	HWND getHwnd() const { return m_hwnd; }
	int getWidth() const { return m_width; }
	int getHeight() const { return m_height; }
};

#endif
