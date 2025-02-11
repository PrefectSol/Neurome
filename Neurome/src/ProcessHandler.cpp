#include "ProcessHandler.h"

ProcessHandler::ProcessHandler()
    : m_window(NULL), m_hdcWindow(NULL), m_hdcMemory(NULL),
    m_hBitmap(NULL), m_windowRect(), m_sourceWidth(0), m_sourceHeight(0),
    m_processId(0), ProcessPolicy(nullptr), ProcessRules(nullptr), NewProcessRule(nullptr) {
    
    RuleName = SysAllocString((BSTR)L"NetBarrier");

    CoInitialize(NULL);

    RuleResult = CoCreateInstance(__uuidof(NetFwPolicy2),
        NULL,
        CLSCTX_ALL,
        __uuidof(INetFwPolicy2),
        (void**)&ProcessPolicy);

    NewRuleResult = CoCreateInstance(__uuidof(NetFwRule),
        NULL,
        CLSCTX_ALL,
        __uuidof(INetFwRule2),
        (void**)&NewProcessRule);

}

ProcessHandler::~ProcessHandler() 
{
    CleanUpNetBarrier();
    cleanupCapture();
}

int ProcessHandler::requestAdmin()
{
    if (isAdmin())
    {
        return 0;
    }

    std::wstring path(MAX_PATH, L'\0');
    GetModuleFileName(NULL, &path[0], MAX_PATH);

    const DWORD currentPID = GetCurrentProcessId();

    SHELLEXECUTEINFO sei = { sizeof(SHELLEXECUTEINFO) };
    sei.lpVerb = L"runas";
    sei.lpFile = path.c_str();
    sei.nShow = SW_SHOWNORMAL;

    if (ShellExecuteEx(&sei))
    {
        CloseHandle(sei.hProcess);

        return 1;
    }
    
    return -1;
}

bool ProcessHandler::isAdmin()
{
    struct RAIICleanup 
    {
        std::unique_ptr<void, decltype(&FreeSid)> adminSid;
        std::unique_ptr<void, decltype(&free)> tokenInfo;

        HANDLE token;

        RAIICleanup() : adminSid(nullptr, FreeSid), 
                        tokenInfo(nullptr, free),
                        token(nullptr) {}

        ~RAIICleanup() 
        {
            if (token)
            {
                CloseHandle(token);
            }
        }
    } cleanup;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &cleanup.token)) 
    {
        return false;
    }

    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    PSID adminSid;
    if (!AllocateAndInitializeSid(&ntAuthority, 2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0, 0, 0, 0, 0, 0,
                                  &adminSid)) 
    {
        return false;
    }

    cleanup.adminSid.reset(adminSid);

    DWORD tokenSize = sizeof(TOKEN_ELEVATION);
    GetTokenInformation(cleanup.token, TokenElevation, nullptr, 0, &tokenSize);

    const auto tokenInfo = static_cast<PTOKEN_ELEVATION>(malloc(tokenSize));
    if (!tokenInfo)
    {
        return false;
    }
    
    cleanup.tokenInfo.reset(tokenInfo);

    TOKEN_ELEVATION elevation{};
    if (!GetTokenInformation(cleanup.token, TokenElevation,
                             &elevation, tokenSize, &tokenSize)) 
    {
        return false;
    }

    return elevation.TokenIsElevated != 0;
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
    BSTR BSTRPath = SysAllocString(_com_util::ConvertStringToBSTR(getProcessPath().c_str()));

    if (!BSTRPath)
        return FALSE;

    if (SUCCEEDED(RuleResult)) {
        RuleResult = ProcessPolicy->get_Rules(&ProcessRules);
    }
    else {
        return FALSE;
    }

    NewProcessRule->put_Name(RuleName);
    NewProcessRule->put_Action(NET_FW_ACTION_BLOCK);
    NewProcessRule->put_Direction(NET_FW_RULE_DIR_OUT);
    NewProcessRule->put_Profiles(NET_FW_PROFILE2_ALL);
    NewProcessRule->put_Protocol(NET_FW_IP_PROTOCOL_ANY);
    NewProcessRule->put_Enabled(VARIANT_TRUE);
    NewProcessRule->put_ApplicationName(BSTRPath);

    RuleResult = ProcessRules->Add(NewProcessRule);

    SysFreeString(BSTRPath);

    return TRUE;
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

bool ProcessHandler::getCaptureWindow(torch::Tensor *capture, int32_t width, int32_t height) const
{
    if (!m_window || !capture || width <= 0 || height <= 0)
    {
        return false;
    }

    if (!BitBlt(m_hdcMemory, 0, 0, m_sourceWidth, m_sourceHeight, m_hdcWindow, 0, 0, SRCCOPY))
    {
        return false;
    }

    const BITMAPINFOHEADER bi{ sizeof(BITMAPINFOHEADER),
                                m_sourceWidth, -m_sourceHeight,
                                1, 32, BI_RGB,
                                0, 0, 0, 0, 0 };

    std::vector<uint8_t> buffer(m_sourceWidth * m_sourceHeight * 4);
    if (!GetDIBits(m_hdcMemory, m_hBitmap, 0, m_sourceHeight, buffer.data(), (BITMAPINFO *)&bi, DIB_RGB_COLORS))
    {
        return false;
    }

    const torch::Tensor original = torch::from_blob(buffer.data(), { m_sourceHeight, m_sourceWidth, 4 }, torch::kUInt8).to(torch::kCUDA);;
    const torch::Tensor grayscale = original.index({ torch::indexing::Slice(), torch::indexing::Slice(), torch::indexing::Slice(0, 3) }).to(torch::kFloat32).mean(-1).unsqueeze(-1);
    const torch::Tensor resized = torch::upsample_bilinear2d(grayscale.permute({ 2, 0, 1 }).unsqueeze(0), { height, width }, false);

    *capture = resized.div(255.0f);
    
    return true;
}

bool ProcessHandler::getWindowSizes(uint32_t *width, uint32_t *height) const
{
    if (!m_window || !width || !height)
    {
        return false;
    }

    RECT rect;
    if (GetClientRect(m_window, &rect))
    {
        *width = rect.right - rect.left;
        *height = rect.bottom - rect.top;

        return true;
    }

    return false;
}

bool ProcessHandler::getWindowOffsets(uint32_t *top, uint32_t *left, uint32_t *bottom, uint32_t *right) const
{
    if (!m_window)
    {
        return false;
    }

    RECT rect;
    if (!GetWindowRect(m_window, &rect))
    {
        return false;
    }

    if (top)
    {
        *top = rect.top;
    }

    if (left)
    {
        *left = rect.left;
    }

    if (bottom)
    {
        *bottom = rect.bottom;
    }

    if (right)
    {
        *right = rect.right;
    }

    return true;
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

void ProcessHandler::CleanUpNetBarrier(){

    RuleResult = ProcessRules->Remove(RuleName);

    if (!ProcessRules)
    {
        ProcessRules->Release();
        ProcessRules = nullptr;
    }


    if (!ProcessPolicy)
    {
        ProcessPolicy->Release();
        ProcessPolicy = nullptr;
    }

    if (!ProcessRules)
    {
        NewProcessRule->Release();
        NewProcessRule = nullptr;
    }

    SysFreeString(RuleName);

    CoUninitialize();
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