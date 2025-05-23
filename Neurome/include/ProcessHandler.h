#ifndef __PROCESS_HANDLER_H
#define __PROCESS_HANDLER_H

#include <string>
#include <cstdint>
#include <thread>
#include <mutex>

#include <Windows.h>
#include <tlhelp32.h>
#include <netfw.h>
#include <comutil.h>
#include <comdef.h>
#include <opencv2/opencv.hpp>
#include <torch/torch.h>

#pragma comment(lib, "comsuppw.lib")

#define FRAME_CHECKER
#define INTERPOLATION cv::INTER_NEAREST

class ProcessHandler 
{
public:
	explicit ProcessHandler();

	~ProcessHandler();

	static int requestAdmin();

	static bool isAdmin();

	bool getProcess(std::string clientName);

	bool getProcess(std::string clientName, bool *loop, uint32_t loopDelay = 0);

	void release();

	bool restart();

	bool blockTraffic();

	std::string getProcessPath() const;

	bool getCaptureWindow(cv::Mat *capture, int32_t width, int32_t height) const;

	bool getCaptureWindow(torch::Tensor *capture, int32_t width, int32_t height) const;

	bool getWindowSizes(uint32_t *width, uint32_t *height) const;

	bool getWindowOffsets(uint32_t *top, uint32_t *left, uint32_t *bottom, uint32_t *right) const;

private:
	INetFwPolicy2* ProcessPolicy;

	INetFwRules* ProcessRules;

	INetFwRule2* NewProcessRule;

	HRESULT NewRuleResult;

	HRESULT RuleResult;

	BSTR RuleName;

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

	void initializeRules();

	bool initializeCapture();

	void CleanUpNetBarrier();

	void cleanupCapture();
};
#endif // !__PROCESS_HANDLER_H