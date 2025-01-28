#include "ProcessHandler.h"

ProcessHandler::ProcessHandler() 
    : m_window(NULL), m_hdcWindow(NULL), m_hdcMemory(NULL),
    m_hBitmap(NULL), m_windowRect(), m_sourceWidth(0), m_sourceHeight(0),
    m_processId(0) {}

ProcessHandler::~ProcessHandler() 
{
    cleanupCapture();
}

bool ProcessHandler::getProcess(std::string clientName)
{
    if (clientName.empty())
    {
        return false;
    }

    cleanupCapture();

    m_processId = getProcessIdByPartialName(std::wstring(clientName.begin(), clientName.end()));
    if (!m_processId)
    {
        return false;
    }

    m_window = getWindowByProcessId(m_processId);
    if (!m_window)
    {
        return false;
    }

    return initializeCapture();
}

bool ProcessHandler::getProcess(std::string clientName, bool *loop, uint32_t loopDelay)
{
    if (clientName.empty() || !loop)
    {
        return false;
    }

    cleanupCapture();

    const std::wstring partialName(clientName.begin(), clientName.end());
    while (*loop)
    {
        const DWORD pid = getProcessIdByPartialName(partialName);
        if (pid)
        {
            m_processId = pid;
            break;
        }

        Sleep(loopDelay);
    }

    m_window = getWindowByProcessId(m_processId);
    if (!m_window)
    {
        return false;
    }

    return initializeCapture();
}

bool ProcessHandler::restart()
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE, FALSE, m_processId);
    if (hProcess == NULL) 
    {
        return false;
    }

    TCHAR szProcessPath[MAX_PATH];
    DWORD pathSize = MAX_PATH;
    if (!QueryFullProcessImageName(hProcess, 0, szProcessPath, &pathSize)) 
    {
        CloseHandle(hProcess);
        return false;
    }

    if (!TerminateProcess(hProcess, 0)) 
    {
        CloseHandle(hProcess);
        return false;
    }

    WaitForSingleObject(hProcess, 7000);
    CloseHandle(hProcess);

    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (!CreateProcess(szProcessPath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) 
    {
        return false;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    cleanupCapture();

    m_window = NULL;
    m_processId = 0;

    return true;
}

bool ProcessHandler::blockTraffic()
{
    // ...

    return false;
}

std::string ProcessHandler::getProcessPath() const
{
    if (!m_processId)
    {
        return std::string();
    }

    std::wstring path;

    HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, m_processId);
    if (processHandle) 
    {
        wchar_t buffer[MAX_PATH];
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(processHandle, 0, buffer, &size)) 
        {
            path = buffer;
        }

        CloseHandle(processHandle);
    }

    return std::string(path.begin(), path.end());
}

bool ProcessHandler::getCaptureWindow(cv::Mat *capture, int32_t width, int32_t height) const
{
    if (!m_window || !capture || width <= 0 || height <= 0) 
    {
        return false;
    }

    if (!BitBlt(m_hdcMemory, 0, 0, m_sourceWidth, m_sourceHeight, m_hdcWindow, 0, 0, SRCCOPY))
    {
        return false;
    }

    const BITMAPINFOHEADER bi { sizeof(BITMAPINFOHEADER),
                                m_sourceWidth, -m_sourceHeight,
                                1, 32, BI_RGB,
                                0, 0, 0, 0, 0};

    cv::Mat originalFrame(m_sourceHeight, m_sourceWidth, CV_8UC4);
    if (!GetDIBits(m_hdcMemory, m_hBitmap, 0, m_sourceHeight, originalFrame.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS)) 
    {
        return false;
    }

    cv::resize(originalFrame, *capture, cv::Size(width, height), 0.0, 0.0, INTERPOLATION);

    cv::cvtColor(*capture, *capture, cv::COLOR_BGRA2RGB);
    capture->convertTo(*capture, CV_32FC3, 1.0 / 255.0);

#ifdef FRAME_CHECKER
    uint32_t nonZeroCount = 0;

    float *ptr = (float *)capture->data;
    const float *endPtr = ptr + capture->rows * capture->cols * capture->channels();

    for (; ptr != endPtr; ++ptr)
    {
        if (*ptr > 0.0f) 
        {
            ++nonZeroCount;
        }
    }

    return nonZeroCount;
#else // FRAME_CHECKER
    return true;
#endif // FRAME_CHECKER
}

DWORD ProcessHandler::getProcessIdByPartialName(const std::wstring &partialName) const
{
    DWORD processId = 0;

    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) 
    {
        PROCESSENTRY32W processEntry { sizeof(PROCESSENTRY32W) };
        if (Process32FirstW(snapshot, &processEntry)) 
        {
            do 
            {
                if (std::wstring(processEntry.szExeFile).find(partialName) != std::wstring::npos) 
                {
                    processId = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &processEntry));
        }

        CloseHandle(snapshot);
    }

    return processId;
}

HWND ProcessHandler::getWindowByProcessId(DWORD processId) const
{
    struct EnumData 
    {
        DWORD processId;
        HWND window;
    }
    data { processId, NULL };

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL 
        {
            struct EnumData &data = *(EnumData*)lParam;
        
            DWORD windowProcessId;
            GetWindowThreadProcessId(hwnd, &windowProcessId);
        
            if (windowProcessId == data.processId) 
            {
                data.window = hwnd;
                return FALSE;
            }

            return TRUE;
        }, (LPARAM)&data);

    return data.window;
}

bool ProcessHandler::initializeCapture() 
{
    if (!m_window) 
    {
        return false;
    }

    m_hdcWindow = GetDC(m_window);
    if (!m_hdcWindow) 
    {
        return false;
    }

    m_hdcMemory = CreateCompatibleDC(m_hdcWindow);
    if (!m_hdcMemory) 
    {
        ReleaseDC(m_window, m_hdcWindow);
        m_hdcWindow = NULL;

        return false;
    }

    GetWindowRect(m_window, &m_windowRect);
    m_sourceWidth = m_windowRect.right - m_windowRect.left;
    m_sourceHeight = m_windowRect.bottom - m_windowRect.top;

    m_hBitmap = CreateCompatibleBitmap(m_hdcWindow, m_sourceWidth, m_sourceHeight);
    if (!m_hBitmap) 
    {
        DeleteDC(m_hdcMemory);
        ReleaseDC(m_window, m_hdcWindow);

        m_hdcWindow = NULL;
        m_hdcMemory = NULL;

        return false;
    }

    SelectObject(m_hdcMemory, m_hBitmap);

    return true;
}

void ProcessHandler::cleanupCapture() 
{
    if (m_hBitmap) 
    {
        DeleteObject(m_hBitmap);
        m_hBitmap = NULL;
    }

    if (m_hdcMemory) 
    {
        DeleteDC(m_hdcMemory);
        m_hdcMemory = NULL;
    }

    if (m_hdcWindow) 
    {
        ReleaseDC(m_window, m_hdcWindow);
        m_hdcWindow = NULL;
    }
}