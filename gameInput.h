#pragma once

#include <windows.h>

class GameInput {
  private:
	DWORD m_pid{0};
	HWND m_mainHwnd{nullptr};

  public:
	// 构造函数：根据进程 PID 查找并绑定目标窗口
	explicit GameInput(DWORD pid);
	~GameInput() = default;

	/**
	 * 后台模拟鼠标点击
	 * @param physicalX 物理坐标 X (如截图或工具量出的原始像素)
	 * @param physicalY 物理坐标 Y
	 * @param qiantai 后台移动到 X,Y 坐标后，使用前台的方式按下左键
	 * @param clickCount 点击次数 (1=单击, 2=双击, N=循环点击)
	 * @param intervalMs 多击之间的延迟毫秒数
	 * @param dpiScale 屏幕缩放比例
	 */
	void click(int physicalX, int physicalY, int qiantai=0, int clickCount = 1, int intervalMs = 150, double dpiScale = 1.25);

	/**
	 * 后台发送键盘按键
	 * @param vkCode 虚拟键码 (例如：VK_F1, 'A', '1' 等)
	 */
	void sendKey(WORD vkCode);

	// 新增：发送组合键 (例如 modifier = VK_CONTROL, vkCode = '3')
	void sendComboKey(WORD modifier, WORD vkCode);

	// 获取当前绑定的主窗口句柄
	HWND getHwnd() const { return m_mainHwnd; }
};
