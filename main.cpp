#include "GameInput.h"
#include "GameWindow.h"
#include "myOcr.h"
#include "opencv2/opencv.hpp"
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>

std::filesystem::path getExeDir() {
	wchar_t buf[MAX_PATH]{};
	GetModuleFileNameW(nullptr, buf, MAX_PATH);
	return std::filesystem::path(buf).parent_path();
}
std::string resolveAssetPath(const std::string& relativePath) {
	return (getExeDir() / relativePath).string();
}
class Ocr {
  public:
	Ocr() {
		// 在构造函数中完成初始化
		bool inited = myOcr::init(
		    // "PP-OCRv5_mobile_det", "./models/PP-OCRv5_mobile_det_infer",
		    // "PP-OCRv5_mobile_rec", "./models/PP-OCRv5_mobile_rec_infer",
		    "PP-OCRv5_mobile_det", "C:/Users/14640/Downloads/paddle/PP-OCRv5_mobile_det_infer",
		    "PP-OCRv5_mobile_rec", "C:/Users/14640/Downloads/paddle/PP-OCRv5_mobile_rec_infer",
		    128);

		std::cout << "inited: " << (inited ? "true" : "false") << std::endl;
		if (!inited) {
			throw std::runtime_error("Ocr initialization failed!");
		}
	}

	~Ocr() = default;

	// 对外提供的核心 run 方法，传入 cv::Mat，返回识别结果列表
	std::vector<myOcr::Result> run(cv::Mat& img) {
		std::vector<myOcr::Result> recResults;

		if (img.empty()) {
			std::cout << "Ocr::run -> 输入图像为空！" << std::endl;
			return recResults;
		}

		// 1. 运行文本检测
		std::vector<myOcr::Result> detResults = myOcr::runDet(img);
		int total = detResults.size();

		// 2. 遍历检测结果并运行文字识别
		for (int i = total - 1; i >= 0; --i) {
			myOcr::Result item = detResults[i];
			myOcr::runRec(item);
			recResults.push_back(item);
		}
		return recResults;
	}
};

// 假设你的 GameWindow 类已经定义，并且有 CaptureMat() 方法
// bool FindSmallIconPos(GameWindow& gameWin, const std::string& templatePath, cv::Point& outPos, double threshold = 0.85) {

// 	// 1. 获取游戏窗口实时画面
// 	cv::Mat screenMat = gameWin.CaptureMat();
// 	if (screenMat.empty()) {
// 		std::cout << "错误：游戏画面获取失败！" << std::endl;
// 		return false;
// 	}

// 	// 2. 从硬盘加载要找的小图（模板）
// 	// 注意：cv::imread 默认读取为 3 通道 BGR，如果你的 CaptureMat 是 4 通道 BGRA，
// 	// 最好保持通道一致（或者都转为 BGR/灰度图再匹配）
// 	cv::Mat templateMat = cv::imread(templatePath, cv::IMREAD_COLOR);
// 	if (templateMat.empty()) {
// 		std::cout << "错误：无法加载目标小图文件 -> " << templatePath << std::endl;
// 		return false;
// 	}

// 	// 【保险措施】如果 screenMat 是 4 通道的 (BGRA)，建议转成 3 通道 (BGR) 再匹配，防止通道数不匹配报错
// 	if (screenMat.channels() == 4) {
// 		cv::cvtColor(screenMat, screenMat, cv::COLOR_BGRA2BGR);
// 	}

// 	// 3. 检查小图尺寸是否大于大图，防止矩阵运算崩溃
// 	if (templateMat.cols > screenMat.cols || templateMat.rows > screenMat.rows) {
// 		std::cout << "错误：小图尺寸大于游戏画面，无法匹配！" << std::endl;
// 		return false;
// 	}

// 	// 4. 创建结果矩阵 (Result Matrix)
// 	int resultCols = screenMat.cols - templateMat.cols + 1;
// 	int resultRows = screenMat.rows - templateMat.rows + 1;
// 	cv::Mat result(resultRows, resultCols, CV_32FC1);

// 	// 5. 执行模板匹配
// 	// 使用 TM_CCOEFF_NORMED（归一化相关系数匹配法），其值在 -1 到 1 之间，1 表示完美匹配
// 	cv::matchTemplate(screenMat, templateMat, result, cv::TM_CCOEFF_NORMED);

// 	// 6. 寻找匹配结果中的最大值及其位置
// 	double minVal, maxVal;
// 	cv::Point minLoc, maxLoc;
// 	cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

// 	// 7. 判断最大相似度是否超过设定的阈值（例如 0.85 即 85% 相似）
// 	if (maxVal >= threshold) {
// 		outPos = maxLoc;  // maxLoc 就是小图在大图中的【左上角】坐标 (x, y)

// 		// 如果你需要小图的【中心点】坐标，可以这样计算：
// 		outPos.x += templateMat.cols / 2;
// 		outPos.y += templateMat.rows / 2;

// 		std::cout << "找到目标！相似度: " << maxVal << " 坐标: (" << outPos.x << ", " << outPos.y << ")" << std::endl;
// 		return true;
// 	}

// 	// 没找到
// 	return false;
// }

bool FindSmallIconPos(GameWindow& gameWin, const std::string& templatePath, cv::Point& outPos, double threshold = 0.85, const cv::Rect& roi = cv::Rect()) {

	// 1. 获取游戏窗口实时画面
	cv::Mat screenMat = gameWin.CaptureMat();
	if (screenMat.empty()) {
		std::cout << "错误：游戏画面获取失败！" << std::endl;
		return false;
	}

	// 2. 从硬盘加载要找的小图（模板）
	cv::Mat templateMat = cv::imread(templatePath, cv::IMREAD_COLOR);
	if (templateMat.empty()) {
		std::cout << "错误：无法加载目标小图文件 -> " << templatePath << std::endl;
		return false;
	}

	// 【保险措施】如果 screenMat 是 4 通道的 (BGRA)，转成 3 通道 (BGR)
	if (screenMat.channels() == 4) {
		cv::cvtColor(screenMat, screenMat, cv::COLOR_BGRA2BGR);
	}

	int offsetX = 0;
	int offsetY = 0;

	// 3. 如果指定了有效的 ROI 区域，则在裁剪前先做边界保护并截取子图
	if (!roi.empty() && roi.width > 0 && roi.height > 0) {
		// 求交集防止越界崩溃
		cv::Rect validRoi = roi & cv::Rect(0, 0, screenMat.cols, screenMat.rows);
		if (validRoi.width <= 0 || validRoi.height <= 0) {
			std::cout << "错误：指定的 ROI 区域超出了屏幕边界！" << std::endl;
			return false;
		}
		screenMat = screenMat(validRoi);
		offsetX = validRoi.x;  // 记录偏移量，用于后续坐标还原
		offsetY = validRoi.y;
	}

	// 4. 检查小图尺寸是否大于搜索区域
	if (templateMat.cols > screenMat.cols || templateMat.rows > screenMat.rows) {
		std::cout << "错误：小图尺寸大于搜索区域（全屏或ROI），无法匹配！" << std::endl;
		return false;
	}

	// 5. 创建结果矩阵 (Result Matrix)
	int resultCols = screenMat.cols - templateMat.cols + 1;
	int resultRows = screenMat.rows - templateMat.rows + 1;
	cv::Mat result(resultRows, resultCols, CV_32FC1);

	// 6. 执行模板匹配
	cv::matchTemplate(screenMat, templateMat, result, cv::TM_CCOEFF_NORMED);

	// 7. 寻找最大值及其位置
	double minVal, maxVal;
	cv::Point minLoc, maxLoc;
	cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);

	// 8. 判断相似度阈值
	if (maxVal >= threshold) {
		// 【关键】将局部裁剪区域的坐标，加上偏移量还原为【全屏绝对坐标】
		outPos.x = maxLoc.x + offsetX;
		outPos.y = maxLoc.y + offsetY;

		// 计算小图的【中心点】坐标
		outPos.x += templateMat.cols / 2;
		outPos.y += templateMat.rows / 2;

		std::cout << "找到目标！相似度: " << maxVal << " 坐标: (" << outPos.x << ", " << outPos.y << ")" << std::endl;
		return true;
	}

	// 没找到
	return false;
}

/**
 * 检查血量并吃血瓶
 * @return 如果本次成功触发了吃血瓶，返回 true；否则返回 false
 */
bool checkAndHeal(GameWindow& gameWin, GameInput& gameInput, double bloodPercent, int hpCheckX, int hpCheckY, int normalColorThreshold, std::chrono::steady_clock::time_point& lastPotionTime, std::chrono::seconds potionCooldown) {
	auto now = std::chrono::steady_clock::now();
	Color hpColor = gameWin.getColorAt(hpCheckX, hpCheckY);

	// 判断是否为有效颜色
	if (hpColor.r != -1 && hpColor.g != -1 && hpColor.b != -1) {
		// 当红色分量 - 绿色分量的差值 低于阈值时触发吃药
		if (hpColor.r - hpColor.g < normalColorThreshold) {
			// 检查血瓶冷却是否结束
			if (now - lastPotionTime >= potionCooldown) {
				std::cout << "[紧急吃血] 检测到血量低于 " << bloodPercent * 100 << "%，立即按下血瓶键！" << std::endl;

				// 按下 `=` 键吃血瓶
				gameInput.sendKey(VK_OEM_PLUS);

				// 更新上次吃药时间，刷新 5 秒冷却
				lastPotionTime = std::chrono::steady_clock::now();
				return true;
			}
		}
	}
	return false;
}

/**
 * 激活能量珠
 */
void hitEnergyBall(GameInput& gi, EnergyBall& b) {
	int phyX = static_cast<int>(b.x * 1.25);
	int phyY = static_cast<int>(b.y * 1.25);
	gi.click(phyX, phyY, 0, 2);
	std::cout << "激活了 " << b.id << " 号能量珠, 坐标 " << b.x << "," << b.y << std::endl;
}

// =========================================================================
// 抽离的业务子函数群
// =========================================================================

/**
 * 处理能量珠轮询与点击
 */
void handleEnergyBalls(GameWindow& gameWin, GameInput& gameInput, std::vector<EnergyBall>& balls, const int ballValidR, const std::chrono::milliseconds& eachBallInterval, int healCastTime, double bloodPercent, int hpCheckX, int hpCheckY, int normalColorThreshold, std::chrono::steady_clock::time_point& lastPotionTime, const std::chrono::seconds& potionCooldown) {
	for (size_t i = 0; i < balls.size(); ++i) {
		auto& b = balls[i];
		auto now = std::chrono::steady_clock::now();
		std::cout << "遍历到 " << b.id << "号能量珠" << std::endl;
		if (now - b.lastEnableTime < b.duration) {
			std::cout << "能量珠 " << b.id << " 已处于激活状态" << std::endl;
			continue;
		};

		Color c = gameWin.getColorAt(b.x, b.y);
		// 有效颜色判定
		if (c.r != -1 && c.g != -1 && c.b != -1) {
			// 能量珠可被激活
			if (c.r > ballValidR) {
				hitEnergyBall(gameInput, b);
				now = std::chrono::steady_clock::now();
				b.lastEnableTime = now;

				// 修改前面的能量珠的 lastEnableTime
				for (int j = 1; j < b.id; ++j) {
					auto& preBall = balls[j - 1];
					preBall.lastEnableTime = now;
				}

				auto energyBallWaitStart = std::chrono::steady_clock::now();
				bool justAtePotion = false;

				// 等待下一个能量珠时，持续监控血量
				while (std::chrono::steady_clock::now() - energyBallWaitStart < eachBallInterval) {
					if (checkAndHeal(gameWin, gameInput, bloodPercent, hpCheckX, hpCheckY, normalColorThreshold, lastPotionTime, potionCooldown)) {
						justAtePotion = true;
					}
					std::this_thread::sleep_for(std::chrono::milliseconds(200));
				}

				// 如果激活能量珠期间吃过血瓶，额外等待
				if (justAtePotion && healCastTime > 0) {
					std::cout << "激活一个能量珠后, 吃过血瓶，额外等待 healCastTime=" << healCastTime << "ms..." << std::endl;
					std::this_thread::sleep_for(std::chrono::milliseconds(healCastTime));
				}
			}
		}
	}
}
struct BuffTarget {
	std::string name{};
	std::string imgPath{};
	cv::Point pos{};
};

void attack(GameInput& gi) {
	gi.sendKey(VK_TAB);
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	gi.sendKey('1');
}

/**
 * 循环释放 Buff 技能组
 */
void handleBuffs(GameInput& gameInput, GameWindow& gw, Ocr& ocr, std::vector<WORD>& buffLists, int buffCastTime, int healCastTime, double bloodPercent, int hpCheckX, int hpCheckY, int normalColorThreshold, std::chrono::steady_clock::time_point& lastPotionTime, const std::chrono::seconds& potionCooldown) {

	for (WORD k : buffLists) {
		// 特别处理buff: 8 双器速攻
		if (k == '8') {
			std::cout << "handle buff 8 shuangqishugong\n";
			cv::Mat img;
			cv::Rect rect{31, 609, 365, 32};  // 聊天框文字识别区域，倒数两行
			cv::Point p;
			cv::Rect topRightRect{803, 2, 551, 66};
			// 界面右上角找buff图片
			bool found = FindSmallIconPos(gw, resolveAssetPath("samplePic/shuangqishugong.bmp"), p, 0.85, topRightRect);
			if (found) continue;  // buff卡bug了，这时候不能更新buff, 等下一轮
			std::cout << "no shuangqishugong buff in ui\n";
			while (true) {
				gameInput.sendKey('7');  // 换斧头
				std::this_thread::sleep_for(std::chrono::milliseconds(350));
				img = gw.CaptureMat()(rect);
				cv::cvtColor(img, img, cv::COLOR_BGRA2BGR);
				std::vector<myOcr::Result> results = ocr.run(img);
				bool switchedShuangqi{false};
				for (myOcr::Result& r : results) {
					if (r.text.find("装备了双器") != std::string::npos) {
						std::cout << "switched shuangqifutou\n";
						switchedShuangqi = true;
						break;
					}
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				if (switchedShuangqi) break;
			}
		}

		gameInput.sendKey(k);
		std::cout << "-> 已发送 Buff 键: " << (char)k << std::endl;

		// 等待技能释放硬直时间
		if (k != '8') {
			std::this_thread::sleep_for(std::chrono::milliseconds(buffCastTime));
		}

		if (k == '8') {
			cv::Mat img;
			cv::Rect rect{31, 609, 365, 32};  // 聊天框文字识别区域，倒数两行
			cv::Point p;
			// 释放双器速攻后
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			while (true) {
				gameInput.sendKey('9');  // 换弓
				std::this_thread::sleep_for(std::chrono::milliseconds(350));
				std::cout << "switched back to gong\n";
				img = gw.CaptureMat()(rect);
				cv::cvtColor(img, img, cv::COLOR_BGRA2BGR);
				std::vector<myOcr::Result> results = ocr.run(img);
				bool switchedGong{false};
				for (myOcr::Result& r : results) {
					if (r.text.find("装备了木") != std::string::npos) {
						std::cout << "shibie dao gong\n";
						switchedGong = true;
						break;
					}
				}
				if (switchedGong) break;
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}
		attack(gameInput);  // 加完一个buff，释放一次攻击。
		checkAndHeal(gw, gameInput, bloodPercent, hpCheckX, hpCheckY, normalColorThreshold, lastPotionTime, potionCooldown);
	}
}

void findBuffTargetsPos(GameWindow& win, std::vector<BuffTarget>& btlist) {
	for (BuffTarget& bt : btlist) {

		bool found = FindSmallIconPos(win, bt.imgPath, bt.pos, 0.85);

		if (!found) {
			std::cout << "未找到 " << bt.name << " 的坐标---" << std::endl;
		}
	}
}

void cropPicAndSave(GameWindow& gw, cv::Rect& roi, const char* path) {
	cv::Mat fullMat = gw.CaptureMat();
	if (fullMat.empty()) {
		std::cout << "获取画面失败，无法裁剪！" << std::endl;
	}

	// 定义你要裁剪的区域：cv::Rect(x, y, width, height)
	// x, y 是裁剪框左上角在窗口中的像素坐标
	// width, height 是裁剪框的宽和高
	// 例如：截取游戏左上角小地图区域 (假设坐标 x=20, y=20, 宽=150, 高=150)
	// cv::Rect roi(38, 230, 55, 16);

	// 【安全检查】防止坐标超出当前窗口的实际分辨率导致程序崩溃
	if (roi.x < 0 || roi.y < 0 ||
	    roi.x + roi.width > fullMat.cols ||
	    roi.y + roi.height > fullMat.rows) {
		std::cout << "错误：裁剪区域超出了窗口边界！" << std::endl;
	}

	// 3. 直接通过括号运算符裁出子矩阵（注意：这只是内存共享，不会发生实际的大规模数据复制，非常高效）
	cv::Mat croppedMat = fullMat(roi);

	// 4. 将裁剪出来的图像保存到硬盘
	// cv::imwrite 支持常见的图片格式，如 .png, .jpg, .bmp 等
	bool success = cv::imwrite(path, croppedMat);
	if (success) {
		std::cout << std::format("局部截图保存成功:{}\n", path);
	} else {
		std::cout << "局部截图保存到硬盘失败！" << std::endl;
	}
}

struct Point {
	int x;
	int y;
};

void convertTOPhysic(Point& p) {
	p.x = static_cast<int>(p.x * 1.25);
	p.y = static_cast<int>(p.y * 1.25);
}

void addWangbaBuff(GameInput& gi, Point& wangbazhuangtai, Point& confirm, std::vector<Point>& wangbaBuffLists) {

	for (Point& b : wangbaBuffLists) {
		gi.click(wangbazhuangtai.x, wangbazhuangtai.y);
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		gi.click(b.x, b.y);
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		gi.click(confirm.x, confirm.y);
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
}

bool isDead(GameWindow& gw, const int normalColorThreshold) {
	Color deadColor = gw.getColorAt(157, 34);
	if (deadColor.r != -1 && deadColor.g != -1 && deadColor.b != -1) {
		if (deadColor.r - deadColor.g < normalColorThreshold && deadColor.r != 0) {
			return true;
		}
	}
	return false;
}

struct BuffItem {
    const char* name;
    WORD key;
    std::chrono::seconds cooldown;
    std::chrono::milliseconds castTime;
    std::chrono::steady_clock::time_point lastTime;
};

struct BuffScheduler {
    std::vector<BuffItem> buffs;
    std::chrono::steady_clock::time_point lastAnyCastTime;
    std::chrono::seconds staggerInterval; // 两个 buff 之间的最小错开间隔（10s）
};
void updateBuffs(GameInput& gameInput, BuffScheduler& scheduler) {
    auto now = std::chrono::steady_clock::now();

    // 距离上一个 buff 释放还没满 10s，先不放下一个 buff，回去正常攻击
    if (now - scheduler.lastAnyCastTime < scheduler.staggerInterval) {
        return;
    }

    for (auto& b : scheduler.buffs) {
        if (now - b.lastTime >= b.cooldown) {
            gameInput.sendKey(b.key);

            auto castDoneTime = std::chrono::steady_clock::now();
            b.lastTime = castDoneTime;
            scheduler.lastAnyCastTime = castDoneTime;

            std::cout << "释放 " << b.name
                      << "，进入 " << b.castTime.count() << "ms 施法保护...\n";

            // 只独占等待这一个 buff 的 2s 施法硬直
            std::this_thread::sleep_for(b.castTime);

            // 每次只放一个 buff，立刻退出循环，把时间交还给主循环攻击，
            // 等 10s staggerInterval 过了再放下一个冷却好的 buff
            return;
        }
    }
}

// =========================================================================
// 主函数
// =========================================================================
int main() {
	// 设置控制台编码以支持中文输出
	SetConsoleOutputCP(65001);

	DWORD gamePid =13048;  // 目标游戏 PID
	// DWORD gamePid = 17992;  // 目标游戏 PID

	// 初始化窗口取色与键鼠操作对象
	GameWindow gameWin(gamePid);
	GameInput gameInput(gamePid);

	// gameWin.saveTo("full.bmp");
	// return 0;

	// cv::Rect rect {591,43,13,13};
	// cropPicAndSave(gameWin, rect, "./samplePic/shuangqishugong_small.bmp");
	// return 0;

	// test ocr
	Ocr ocr{};
	// cv::Mat img = cv::imread("./ts.png", cv::IMREAD_COLOR_BGR);
	// cv::Mat img = gameWin.CaptureMat();
	// cv::cvtColor(img, img, cv::COLOR_BGRA2BGR);
	// cv::Rect roi {33,402,362,32};
	// 游戏聊天区域最低一行的左上角的坐标：(31，625)
	// 聊天区域有3种文字行数，由少到多：[7，7+8，7+8+8]
	// 每行的宽度是[364]，高度是[16]
	// cv::Rect roi {31,625,364,16};
	// img = img(roi);
	// std::vector<myOcr::Result> res = ocr.run(img);
	// for (myOcr::Result& r : res) {
	// 	std::cout << std::format("srore:{}, text:{}\n", r.score, r.text);
	// }
	// return 0;

	// cv::Rect rec{38, 230, 30, 16};
	// cropPicAndSave(gameWin, rec, "./targetPicture/mini.bmp");
	// return 0;

	// 网吧状态相关变量
	const auto wangbaBuffInterval = std::chrono::seconds(3605);
	auto lastWangbaBuffTime = std::chrono::steady_clock::now() - wangbaBuffInterval;
	// logic
	// Point zhutiwangba{244, 718};
	// convertTOPhysic(zhutiwangba);
	Point wangbazhuangtai{490, 690};
	convertTOPhysic(wangbazhuangtai);
	Point confirm{450, 512};
	convertTOPhysic(confirm);
	// buffs
	Point jifengqu{258, 368};
	convertTOPhysic(jifengqu);
	Point zhanhunqu{258, 214};
	convertTOPhysic(zhanhunqu);
	Point jinjiqu{258, 330};
	convertTOPhysic(jinjiqu);
	Point tianjiqu{258, 292};
	convertTOPhysic(tianjiqu);
	std::vector<Point> wangbaBuffLists{jifengqu,zhanhunqu};

	// 定义血条监控坐标
	// 调用 gameWin.asveTo("gameWindowShot.bmp")
	// 然后用系统自带的画图工具打开, 取色，取坐标点
	// 血条坐标范围 [156,33]-[378,33], 278-156=222
	// x值每+22,+10%百分比
	double bloodPercent = 0.55;
	int hpCheckX = (int)(156 + bloodPercent * 222);
	int hpCheckY = 34;

	// 血条监控坐标点的颜色阈值
	int normalColorThreshold = 20;

	// 能量珠坐标列表
	std::vector<EnergyBall> balls = {
	    {1, 71, 22, std::chrono::steady_clock::now() - std::chrono::minutes(8), std::chrono::minutes(8)},
	    {2, 63, 75, std::chrono::steady_clock::now() - std::chrono::minutes(5), std::chrono::minutes(5)},
	    {3, 67, 112, std::chrono::steady_clock::now() - std::chrono::minutes(3), std::chrono::minutes(3)}};

	// 时间与周期参数配置
	// 能量珠检测周期
	const auto energyInterval = std::chrono::minutes(3);
	// 激活一个能量珠之后的时间间隔
	const auto eachBallInterval = std::chrono::milliseconds(1500);
	// 可被激活的能量珠的颜色 r分量
	const int ballValidR = 150;
	auto lastEnergyTime = std::chrono::steady_clock::now() - energyInterval;

	int healCastTime = 0;
	const auto potionCooldown = std::chrono::seconds(5);

	// 5min buff
	int buffCastTime = 2860;  // 3500
	std::vector<WORD> buffLists = {'4', '5', '8'};
	std::string shuangqishugongPath = "./samplePic/shuangqishugong.bmp";
	const auto buffInterval = std::chrono::seconds(302);  // 302
	auto lastBuffTime = std::chrono::steady_clock::now() - buffInterval;
	bool isFourReleased = false;
	bool isFiveReleased = false;
	

	// 3min buff
	int buffCastTime2 = 1000;  // 1000
	std::vector<WORD> buffLists2 = {'3'};
	const auto buffInterval2 = std::chrono::seconds(180);  // 183
	auto lastBuffTime2 = std::chrono::steady_clock::now() - buffInterval2;
	// 20min gongshuka
	int buffCastTime3 = 800;
	std::vector<WORD> buffLists3 = {'0'};
	const auto buffInterval3 = std::chrono::seconds(1200);  // 1204
	auto lastBuffTime3 = std::chrono::steady_clock::now() - buffInterval3;

	auto lastPotionTime = std::chrono::steady_clock::now();

	std::cout << "=== 游戏挂机脚本已启动 ===" << std::endl;

	while (true) {
		if (isDead(gameWin, normalColorThreshold)) {
			auto now = std::chrono::system_clock::now();
			auto localSec = std::chrono::floor<std::chrono::seconds>(
			    std::chrono::zoned_time{std::chrono::current_zone(), now}.get_local_time());
			// 输出：2026-11-20 17:26:08
			std::cout << std::format("{:%Y-%m-%d %H:%M:%S}: 迷你 is dead...\n", localSec);
			// 文件名不能有 ':'，用 '-' 或 '_' 替代，后缀写 .png
			std::string fileName = std::format("迷你_dead_{:%Y-%m-%d_%H-%M-%S}.png", localSec);
			gameWin.saveTo(fileName.c_str());
			break;
		}

		auto now = std::chrono::steady_clock::now();

		// 1. 能量珠轮询任务
		if (now - lastEnergyTime >= energyInterval) {
			handleEnergyBalls(gameWin, gameInput, balls, ballValidR, eachBallInterval, healCastTime,
			                  bloodPercent, hpCheckX, hpCheckY, normalColorThreshold, lastPotionTime, potionCooldown);
			lastEnergyTime = std::chrono::steady_clock::now();
		}

		// wangba buff
		if (now - lastWangbaBuffTime >= wangbaBuffInterval) {
			// addWangbaBuff(gameInput, wangbazhuangtai, confirm, wangbaBuffLists);
			// lastWangbaBuffTime = std::chrono::steady_clock::now();
		}

		// 2. 5min Buff 任务
		if (now - lastBuffTime >= buffInterval) {
			handleBuffs(gameInput, gameWin, ocr, buffLists, buffCastTime, healCastTime,
			            bloodPercent, hpCheckX, hpCheckY, normalColorThreshold, lastPotionTime, potionCooldown);
			lastBuffTime = std::chrono::steady_clock::now();
		}
		// 3min buff 任务
		if (now - lastBuffTime2 >= buffInterval2) {
			handleBuffs(gameInput, gameWin, ocr, buffLists2, buffCastTime2, healCastTime,
			            bloodPercent, hpCheckX, hpCheckY, normalColorThreshold, lastPotionTime, potionCooldown);
			lastBuffTime2 = std::chrono::steady_clock::now();
		}

		// 20min gongsuka 任务
		if (now - lastBuffTime3 >= buffInterval3) {
			handleBuffs(gameInput, gameWin, ocr, buffLists3, buffCastTime3, healCastTime,
			            bloodPercent, hpCheckX, hpCheckY, normalColorThreshold, lastPotionTime, potionCooldown);
			lastBuffTime3 = std::chrono::steady_clock::now();
		}

		// 3. 日常高频实时血量监控
		checkAndHeal(gameWin, gameInput, bloodPercent, hpCheckX, hpCheckY, normalColorThreshold, lastPotionTime, potionCooldown);

		attack(gameInput);

		// 5. 常规循环节拍控制（降低 CPU 占用）
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	return 0;
}
