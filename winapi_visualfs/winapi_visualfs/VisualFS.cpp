// Автор: Николай Кочетов
// Описание: Визуализация дерева файловой системы


#include "VisualFS.h"
#include "Shlobj.h"

const wchar_t VisualFS::CLASS_NAME[] = L"VisualFS";

bool VisualFS::Create(const fs::wpath &path)  {
    currentPath = path;
    CreateWindowExW(
        //0, 
        WS_EX_TRANSPARENT,
        CLASS_NAME,
        L"VisualFS",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        500, 500,
        NULL,
        NULL,
        ::GetModuleHandleW(0),
        this);
    UpdateWindow(hWnd);
    rootFolder = new FolderWindow(hWnd, fsTree.OpenPath(path), *this);
    //rootFolder->Resize(1, 1, 495, 495);
    return hWnd != 0;
}

void VisualFS::Show(int cmdShow) {
    ShowWindow(hWnd, cmdShow);
    //rootFolder->Show(cmdShow);
}

bool VisualFS::RegisterClass() {
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = MyWndowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = ::GetModuleHandleW(0);
    wcex.hIcon = NULL;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = CLASS_NAME;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

    if (!RegisterClassExW(&wcex))
        return 1;
}

LRESULT CALLBACK VisualFS::pathTextBoxMyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    VisualFS *visualFS = (VisualFS *) ::GetWindowLongPtr(::GetParent(hwnd), GWL_USERDATA);
    switch (msg)
    {
    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_RETURN:
            visualFS->onPathTextBoxEnter();
            break;
        }
        break;
    }
    return CallWindowProc(visualFS->pathTextBoxWndProc, hwnd, msg, wParam, lParam);
}

LRESULT __stdcall VisualFS::MyWndowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

    VisualFS *visualFS = (VisualFS *) ::GetWindowLongPtr(hWnd, GWL_USERDATA);

    switch (message) {
    case WM_NCCREATE:
        ::SetWindowLongPtr(hWnd, GWL_USERDATA, LONG(LPCREATESTRUCT(lParam)->lpCreateParams));
        ((VisualFS*) ::GetWindowLongPtr(hWnd, GWL_USERDATA))->hWnd = hWnd;
        return DefWindowProcW(hWnd, message, wParam, lParam);
    case WM_CREATE:
        visualFS->onCreate();
        return DefWindowProcW(hWnd, message, wParam, lParam);
    case WM_DESTROY:
        visualFS->onDestroy();
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        visualFS->onSize();
        break;
    //case WM_ERASEBKGND:
    //    return 1;
    //    break;
    case WM_CLOSE:
        visualFS->onClose();
        break; 
    case WM_TIMER:
        visualFS->onTimer();
        break;
    case WM_LBUTTONDBLCLK:
        visualFS->onMouseLButtonDblClk();
        break;
        
    case WM_COMMAND:
            switch HIWORD(wParam) {
            case EN_KILLFOCUS:
                if ((HWND)lParam == visualFS->pathTextBoxHandle) {
                    visualFS->onPathTextBoxKillFocus();
                }
            }
            if ((HWND)lParam == visualFS->returnButtonHandle)
                visualFS->onReturnButtonClicked();
            else if ((HWND)lParam == visualFS->openFileDlgButtonHandle)
                visualFS->onOpenDlgButtonClicked();
            break;
        break;
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
        break;
    }
    return 0;
}


void VisualFS::onCreate() {
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    int width = clientRect.right - clientRect.left;
    int height = -(clientRect.top - clientRect.bottom);
    pathTextBoxHandle = ::CreateWindowW(L"EDIT", currentPath.file_string().c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, width - 3 - openFileDlgButtonWidth - returnButtonWidth
        , textBoxWndProcHeight, hWnd, NULL, ::GetModuleHandleW(NULL), NULL);
    ::SetTimer(hWnd, 0, 100, NULL);
    pathTextBoxWndProc = (WNDPROC)SetWindowLongPtr(pathTextBoxHandle, GWL_WNDPROC, (LONG_PTR)((WNDPROC)pathTextBoxMyWndProc));
    returnButtonHandle = CreateWindowW(L"BUTTON", L"<-",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, width - openFileDlgButtonWidth - returnButtonWidth, 0, returnButtonWidth,
        textBoxWndProcHeight, hWnd, NULL, ::GetModuleHandleW(NULL), NULL); 
    openFileDlgButtonHandle = CreateWindowW(L"BUTTON", L"...",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, width - openFileDlgButtonWidth, 0, openFileDlgButtonWidth, 
        textBoxWndProcHeight, hWnd, NULL, ::GetModuleHandleW(NULL), NULL);
}

void VisualFS::onTimer() {
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    int width = clientRect.right - clientRect.left;
    int height = -(clientRect.top - clientRect.bottom);
    rootFolder->Resize(1, textBoxWndProcHeight + 1, width - 2, height - 2 - 16);
    //::InvalidateRect(hWnd, NULL, true);
}

void VisualFS::onSize() {
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    int width = clientRect.right - clientRect.left;
    int height = -(clientRect.top - clientRect.bottom);
    ::SetWindowPos(pathTextBoxHandle, 0, 0, 0, width - returnButtonWidth - openFileDlgButtonWidth, textBoxWndProcHeight, SWP_NOZORDER);
    ::SetWindowPos(returnButtonHandle, 0, width - openFileDlgButtonWidth - returnButtonWidth, 0
        , returnButtonWidth, textBoxWndProcHeight, SWP_NOZORDER);
    ::SetWindowPos(openFileDlgButtonHandle, 0, width - openFileDlgButtonWidth, 0
        , openFileDlgButtonWidth, textBoxWndProcHeight, SWP_NOZORDER);
}

void VisualFS::onPathTextBoxEnter() {
    size_t textSize = ::SendMessageW(pathTextBoxHandle, WM_GETTEXTLENGTH, 0, 0);
    wchar_t *text = new wchar_t[textSize + 1];
    ::SendMessageW(pathTextBoxHandle, WM_GETTEXT, textSize + 1, (LPARAM)text);
    std::wstring path(text);
    delete[] text;
    if (fs::exists(fs::wpath(path))) {
        delete rootFolder;
        rootFolder = new FolderWindow(hWnd, fsTree.OpenPath(fs::wpath(path)), *this);
        currentPath = path;
    } else {
        ::SendMessageW(pathTextBoxHandle, WM_SETTEXT, NULL, (LPARAM)path.c_str());
    }
}

void VisualFS::onPathTextBoxKillFocus() {
}

void VisualFS::SetFolderWindow(fs::wpath &newPath, FolderWindow *newFolder){
    delete rootFolder;
    rootFolder = new FolderWindow(hWnd, fsTree.OpenChildPath(newPath), *this);
    currentPath = newPath;
    ::SendMessageW(pathTextBoxHandle, WM_SETTEXT, NULL, (LPARAM)currentPath.file_string().c_str());
}

void VisualFS::onClose() {
    ::DestroyWindow(hWnd);
}

void VisualFS::onDestroy() {
    ::KillTimer(hWnd, 0);
}

void VisualFS::onReturnButtonClicked() {
    delete rootFolder;
    rootFolder = new FolderWindow(hWnd, fsTree.OpenRootPath(), *this);
    currentPath = rootFolder->GetPath();
    ::SendMessageW(pathTextBoxHandle, WM_SETTEXT, NULL, (LPARAM)currentPath.file_string().c_str());
}

void VisualFS::onOpenDlgButtonClicked() {

    wchar_t szDir[MAX_PATH];
    BROWSEINFOW bInfo;
    bInfo.hwndOwner = hWnd;
    bInfo.pidlRoot = NULL;
    bInfo.pszDisplayName = szDir; // Address of a buffer to receive the display name of the folder selected by the user
    bInfo.lpszTitle = L"Please, select a folder"; // Title of the dialog
    bInfo.ulFlags = 0;
    bInfo.lpfn = NULL;
    bInfo.lParam = 0;
    bInfo.iImage = -1;

    LPITEMIDLIST lpItem = SHBrowseForFolderW(&bInfo);
    if (lpItem != NULL)
    {
        SHGetPathFromIDListW(lpItem, szDir);
        fs::wpath path(szDir);
        this->SetFolderWindow(path);
    }
}