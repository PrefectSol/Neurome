#ifndef __PROCESS_HANDLER_H
#define __PROCESS_HANDLER_H

#include <string>
#include <cstdint>
#include <thread>
#include <mutex>
#include <chrono>

#include <Windows.h>
#include <tlhelp32.h>

#include <opencv2/opencv.hpp>
#include <torch/torch.h>

#define FRAME_CHECKER
#define INTERPOLATION cv::INTER_NEAREST

class ProcessHandler
{
public:
	explicit ProcessHandler();

	~ProcessHandler();

	bool getProcess(std::string clientName);

	bool getProcess(std::string clientName, bool *loop, uint32_t loopDelay = 0);

	bool restart();

	/// <summary>
	/// Блокировка входящего/исходящего сетевого трафика процесса
	/// </summary>
	/// <returns>true - трафик заблокирован, false - невозможно заблокировать трафик</returns>
	bool blockTraffic();

	std::string getProcessPath() const;

	bool getCaptureWindow(cv::Mat *capture, int32_t width, int32_t height) const;

	bool getCaptureWindow(torch::Tensor *capture, int32_t width, int32_t height) const;

private:
	HWND m_window;

	HDC m_hdcWindow;

	HDC m_hdcMemory;

	HBITMAP m_hBitmap;

	RECT m_windowRect;

	int32_t m_sourceWidth;

	int32_t m_sourceHeight;

	DWORD m_processId;

	DWORD getProcessIdByPartialName(const std::wstring &partialName) const;

	HWND getWindowByProcessId(DWORD processId) const;

	HWND getWindowByProcessIdTimed(DWORD processId, uint32_t timeout, uint32_t delay) const;

	bool initializeCapture();

	void cleanupCapture();
};
#endif // !__PROCESS_HANDLER_H