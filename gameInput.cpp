#include "gameInput.h"
#include <chrono>
#include <iostream>
#include <minwindef.h>
#include <thread>

static void ForceForegroundWindow(HWND targetHwnd) {
	if (!targetHwnd || !IsWindow(targetHwnd)) return;

	// 仅在窗口处于最小化状态时才还原，避免把原本最大化的窗口变成普通尺寸
	if (IsIconic(targetHwnd)) {
		ShowWindow(targetHwnd, SW_RESTORE);
	}

	HWND foreHwnd = GetForegroundWindow();
	if (foreHwnd == targetHwnd) return;

	DWORD foreThreadId = foreHwnd ? GetWindowThreadProcessId(foreHwnd, nullptr) : 0;
	DWORD targetThreadId = GetWindowThreadProcessId(targetHwnd, nullptr);
	DWORD currentThreadId = GetCurrentThreadId();

	// 1. 临时挂载线程输入队列
	if (foreThreadId != 0 && foreThreadId != currentThreadId) {
		AttachThreadInput(currentThreadId, foreThreadId, TRUE);
	}
	if (targetThreadId != 0 && targetThreadId != currentThreadId) {
		AttachThreadInput(currentThreadId, targetThreadId, TRUE);
	}

	// 2. 关键：模拟一次扩展 Alt 键，让 Windows 内核授予当前线程前台切换权限
	INPUT altInputs[2] = {};
	altInputs[0].type = INPUT_KEYBOARD;
	altInputs[0].ki.wVk = VK_MENU;
	altInputs[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;

	altInputs[1].type = INPUT_KEYBOARD;
	altInputs[1].ki.wVk = VK_MENU;
	altInputs[1].ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP;

	SendInput(2, altInputs, sizeof(INPUT));

	// 3. 置前并恢复内部输入焦点
	SetForegroundWindow(targetHwnd);
	BringWindowToTop(targetHwnd);
	SetFocus(targetHwnd);

	// 4. 解除线程关联
	if (foreThreadId != 0 && foreThreadId != currentThreadId) {
		AttachThreadInput(currentThreadId, foreThreadId, FALSE);
	}
	if (targetThreadId != 0 && targetThreadId != currentThreadId) {
		AttachThreadInput(currentThreadId, targetThreadId, FALSE);
	}
}

// 内部辅助结构体和回调函数（用于通过 PID 查找窗口）
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

// ---- GameInput 类成员函数实现 ----

GameInput::GameInput(DWORD pid) : m_pid(pid) {
	m_mainHwnd = GetHwndByPid(pid);
	if (!m_mainHwnd || !IsWindow(m_mainHwnd)) {
		std::cout << "GameInput: 未找到 PID 为 " << pid << " 的窗口！" << std::endl;
	}
}

void GameInput::click(int physicalX, int physicalY, int qiantai, int clickCount, int intervalMs, double dpiScale) {
	// 确保窗口句柄有效（若中途关闭可尝试重新获取）
	if (!m_mainHwnd || !IsWindow(m_mainHwnd)) {
		m_mainHwnd = GetHwndByPid(m_pid);
		if (!m_mainHwnd || !IsWindow(m_mainHwnd)) {
			std::cout << "GameInput::click -> 无效的窗口句柄！" << std::endl;
			return;
		}
	}

	// 125% 高分屏物理坐标转逻辑坐标
	int clientX = (int)(physicalX / dpiScale);
	int clientY = (int)(physicalY / dpiScale);

	// 寻找该坐标下精确的子控件句柄 (HWND)
	POINT pt = {clientX, clientY};
	HWND targetHwnd = ChildWindowFromPoint(m_mainHwnd, pt);
	if (!targetHwnd || targetHwnd == m_mainHwnd) {
		targetHwnd = m_mainHwnd;
	}

	LPARAM lParam = MAKELPARAM(clientX, clientY);

	// 执行多击或双击循环
	for (int i = 0; i < clickCount; ++i) {
		if (i == 1) {
			// 第二次点击发送双击专属消息
			PostMessage(targetHwnd, WM_MOUSEMOVE, 0, lParam);
			Sleep(10);
			PostMessage(targetHwnd, WM_LBUTTONDBLCLK, MK_LBUTTON, lParam);
			PostMessage(targetHwnd, WM_LBUTTONUP, 0, lParam);
		} else {
			// 常规单击
			PostMessage(targetHwnd, WM_MOUSEMOVE, 0, lParam);
			Sleep(10);

			if (qiantai == 0) {
				PostMessage(targetHwnd, WM_LBUTTONDOWN, MK_LBUTTON, lParam);
				Sleep(30);
				PostMessage(targetHwnd, WM_LBUTTONUP, 0, lParam);
			} else {

				// 记录当前前台窗口
				// 使用了vdd虚拟显示屏，就不需要切换了
				// HWND prevForeHwnd = GetForegroundWindow();

				// 记录当前物理鼠标位置
				POINT oldPos;
				GetCursorPos(&oldPos);

				if (IsIconic(m_mainHwnd)) {
					std::cout << "minimize window\n";
					// 使用 SW_RESTORE 还原窗口，但加上 SW_SHOWNOACTIVATE 标志！
					// 这样可以让游戏恢复显示和渲染，同时绝对不会抢走你主屏幕编辑器的键盘焦点！
					ShowWindow(m_mainHwnd, SW_SHOWNOACTIVATE);
					Sleep(50);  // 留出一点时间让游戏恢复画面渲染
				}

				// 计算目标点在屏幕上的物理坐标
				POINT screenPt = {clientX, clientY};
				ClientToScreen(m_mainHwnd, &screenPt);

				// 使用了vdd虚拟显示屏，就不需要切换了
				// 3. 切换游戏窗口到前台（如果游戏本身就在前台，则跳过切窗等待）
				// bool needSwitch = (prevForeHwnd != m_mainHwnd);
				// if (needSwitch) {
				// 	ForceForegroundWindow(m_mainHwnd);
				// 	Sleep(50);  // 给 D3D/游戏引擎 2~3 帧时间完成焦点切换并解除后台输入冻结
				// }


				// 瞬移鼠标到目标落点并等待指示器刷新
				SetCursorPos(screenPt.x, screenPt.y);
				Sleep(20);

				// 发送硬件级左键点击
				INPUT inputs[2] = {};
				inputs[0].type = INPUT_MOUSE;
				inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
				inputs[1].type = INPUT_MOUSE;
				inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

				SendInput(1, &inputs[0], sizeof(INPUT));
				Sleep(40);
				SendInput(1, &inputs[1], sizeof(INPUT));
				Sleep(20);  // 留出极短缓冲，确保游戏处理完 WM_LBUTTONUP 后再切走窗口

				// 6. 归位物理鼠标
				SetCursorPos(oldPos.x, oldPos.y);
				// 使用了vdd虚拟显示屏，就不需要切换了
				// if (needSwitch && prevForeHwnd && IsWindow(prevForeHwnd)) {
				// 	ForceForegroundWindow(prevForeHwnd);
				// }
			}
		}

		if (i < clickCount - 1 && intervalMs > 0) {
			Sleep(intervalMs);
		}
	}
}

void GameInput::sendKey(WORD vkCode) {
	if (!m_mainHwnd || !IsWindow(m_mainHwnd)) {
		m_mainHwnd = GetHwndByPid(m_pid);
		if (!m_mainHwnd || !IsWindow(m_mainHwnd)) {
			std::cout << "GameInput::sendKey -> 无效的窗口句柄！" << std::endl;
			return;
		}
	}

	UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
	LPARAM lParamDown = 1 | (scanCode << 16);
	LPARAM lParamUp = 1 | (scanCode << 16) | (1 << 30) | (1 << 31);

	PostMessage(m_mainHwnd, WM_KEYDOWN, vkCode, lParamDown);
	Sleep(50);
	PostMessage(m_mainHwnd, WM_KEYUP, vkCode, lParamUp);
}

void GameInput::sendComboKey(WORD modifier, WORD vkCode) {
	if (!m_mainHwnd || !IsWindow(m_mainHwnd)) {
		m_mainHwnd = GetHwndByPid(m_pid);
		if (!m_mainHwnd || !IsWindow(m_mainHwnd)) {
			std::cout << "GameInput::sendComboKey -> 无效的窗口句柄！" << std::endl;
			return;
		}
	}

	// 1. 获取修饰键（如 Ctrl / Shift / Alt）的扫描码并按下
	UINT modScanCode = MapVirtualKey(modifier, MAPVK_VK_TO_VSC);
	LPARAM modDown = 1 | (modScanCode << 16);

	// 部分游戏需要配合扩展键标志或虚拟按键状态消息，这里用标准的 PostMessage
	PostMessage(m_mainHwnd, WM_KEYDOWN, modifier, modDown);
	std::this_thread::sleep_for(std::chrono::milliseconds(30));  // 稍作停顿确保游戏识别到 Ctrl 按下

	// 2. 发送主按键 (按下并抬起)
	UINT scanCode = MapVirtualKey(vkCode, MAPVK_VK_TO_VSC);
	LPARAM lParamDown = 1 | (scanCode << 16);
	LPARAM lParamUp = 1 | (scanCode << 16) | (1 << 30) | (1 << 31);

	PostMessage(m_mainHwnd, WM_KEYDOWN, vkCode, lParamDown);
	std::this_thread::sleep_for(std::chrono::milliseconds(50));  // 模拟键程保持
	PostMessage(m_mainHwnd, WM_KEYUP, vkCode, lParamUp);

	std::this_thread::sleep_for(std::chrono::milliseconds(30));

	// 3. 释放修饰键
	LPARAM modUp = 1 | (modScanCode << 16) | (1 << 30) | (1 << 31);
	PostMessage(m_mainHwnd, WM_KEYUP, modifier, modUp);

	std::cout << "-> 已向后台窗口发送组合键 (0x" << std::hex << modifier << " + 0x" << vkCode << ")" << std::endl;
}
