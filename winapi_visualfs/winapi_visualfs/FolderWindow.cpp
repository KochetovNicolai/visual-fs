#include "FolderWindow.h"
#include <algorithm>
const wchar_t FolderWindow::CLASS_NAME[] = L"FolderWindow";

FolderWindow::FolderWindow(HWND parentHandle, const std::shared_ptr<FileSystemTree::PathInfo> &pathInfo, VisualFS &visualFS) :
                           parentHandle(parentHandle), isSleeping(false), pathInfo(pathInfo), visualFS(visualFS) {
    CreateWindowExW(
        0,
        //WS_EX_TRANSPARENT,
        CLASS_NAME,
        L"FolderWindow",
        WS_CHILD | WS_CLIPCHILDREN | WS_BORDER | WS_VISIBLE,
        x, y,
        height, width,
        parentHandle,
        NULL,
        ::GetModuleHandleW(0),
        this);
    UpdateWindow(hWnd);
}

FolderWindow::~FolderWindow() {
    killChildren();
    ::DestroyWindow(hWnd);
}

void FolderWindow::updateSize(std::vector<SizeInfo> &sizeInfo, size_t begin, size_t end, const SizeInfo &rect) {
    if (end - begin == 1) {
        FolderWindow *ptr = sizeInfo[begin].folderWindow;
        sizeInfo[begin] = rect;
        sizeInfo[begin].folderWindow = ptr;
        return;
    }
    //std::sort(sizeInfo.rbegin() + begin, sizeInfo.rbegin() + end);
    std::sort(sizeInfo.rbegin() + (sizeInfo.size() - end), sizeInfo.rbegin() + (sizeInfo.size() - begin));
    size_t totalWeight = 0;
    for (size_t i = begin; i < end; ++i)
        totalWeight += weightApproximation(sizeInfo[i].size);
    size_t sum = weightApproximation(sizeInfo[begin].size);
    size_t lastPtr = begin + 1;
    for (size_t i = begin + 1; i != end; ++i) {
        size_t add = weightApproximation(sizeInfo[i].size);
        if (2 * (add + sum) < totalWeight) {
            std::swap(sizeInfo[i], sizeInfo[lastPtr]);
            sum += add;
            ++lastPtr;
        }
    }
    if (rect.width > rect.height) {
        SizeInfo newRect = rect;
        newRect.width = rect.width * sum / (totalWeight + 1);
        updateSize(sizeInfo, begin, lastPtr, newRect);
        newRect.x += newRect.width;
        newRect.width = rect.width - newRect.width;
        updateSize(sizeInfo, lastPtr, end, newRect);
    } else {
        SizeInfo newRect = rect;
        newRect.height = rect.height * sum / (totalWeight + 1);
        updateSize(sizeInfo, begin, lastPtr, newRect);
        newRect.y += newRect.height;
        newRect.height = rect.height - newRect.height;
        updateSize(sizeInfo, lastPtr, end, newRect);
    }
}

void FolderWindow::updateChildrenPos(bool forceRepaint) {
    std::vector<SizeInfo> childSize(children.size());
    for (size_t i = 0; i < children.size(); i++) {
        childSize[i].size = children[i]->GetCurrentSize();
        childSize[i].folderWindow = children[i];
    }
    updateSize(childSize, 0, childSize.size(), SizeInfo(1, 1 + textBoxHeight, width - 5, height - 5 - textBoxHeight));
    for (size_t i = 0; i < childSize.size(); ++i) {
        SizeInfo rect = childSize[i];
        rect.folderWindow->Resize(rect.x + 1, rect.y + 1, rect.width - 2, rect.height - 2, forceRepaint);
    }
}

bool FolderWindow::tryUpdateText() {
    bool updated = false;
    if (pathInfo->isUpdated.load()) {
        size_t size = ::SendMessageW(textBoxHandle, WM_GETTEXTLENGTH, 0, 0);
        wchar_t * text = new wchar_t[size + 1];
        ::SendMessageW(textBoxHandle, WM_GETTEXT, size + 1, (LPARAM)text);
        if (pathInfo->path.filename() != std::wstring(text)) {
            updated = true;
            ::SendMessageW(textBoxHandle, WM_SETTEXT, NULL, (LPARAM)pathInfo->path.filename().c_str());
        }
        delete[] text;
    }
    return updated;
}

void FolderWindow::killChildren() {
    if (children.size() && children[0])
        for (size_t i = 0; i < children.size(); i++) {
            delete children[i];
            children[i] = NULL;
        }
}

void FolderWindow::birthChildren() {
    if (!children.size() && pathInfo->isUpdated.load() && pathInfo->children.size())
        children.assign(pathInfo->children.size(), NULL);
    if (children.size() && !children[0])
        for (size_t i = 0; i < children.size(); i++)
            children[i] = new FolderWindow(hWnd, pathInfo->children[i], visualFS);
}

void FolderWindow::Resize(int _x, int _y, int _width, int _height, bool forceRepaint) {
    bool isPosChanged = !(x == _x && y == _y && width == _width && height == _height) || forceRepaint;
    x = _x; y = _y; 
    height = _height; width = _width;
    if (height > 0 && width > 0 && isPosChanged)
        ::SetWindowPos(hWnd, 0, x, y, width, height, SWP_NOZORDER);
    ShowWindow(hWnd, height > 0 && width > 0 ? SW_SHOW : SW_HIDE);
    //::ShowWindow(hWnd, min(height, width) < minSize ? SW_HIDE : SW_SHOW);
    if (min(height, width) < minSize || !pathInfo->isUpdated.load() || !pathInfo->children.size()) {
        isSleeping = true;
        if (min(height, width) >= minSize && (isPosChanged || tryUpdateText())) {
            ::SetWindowPos(textBoxHandle, 0, 1, 1, width - 3, height - 3, SWP_NOZORDER);
            //::InvalidateRect(hWnd, NULL, false);
        }
        killChildren();
    } else {
        isSleeping = false;
        if (isPosChanged || tryUpdateText()) {
            ::SetWindowPos(textBoxHandle, 0, 1, 1, width - 3, textBoxHeight, SWP_NOZORDER);
            //::InvalidateRect(hWnd, NULL, false);
        }
        birthChildren();
        updateChildrenPos(isPosChanged);
    }
    //::InvalidateRect(hWnd, NULL, false);
}

void FolderWindow::Show(int cmdShow) {
    if (!isSleeping) {
        ShowWindow(textBoxHandle, cmdShow);
        ShowWindow(hWnd, cmdShow);
        if (children.size() && children[0])
            for (size_t i = 0; i < children.size(); i++)
                children[i]->Show(cmdShow);
    }
}

bool FolderWindow::RegisterClass() {
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
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

LRESULT CALLBACK FolderWindow::textBoxMyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    FolderWindow *folderWindow = (FolderWindow *) ::GetWindowLongPtr(::GetParent(hwnd), GWL_USERDATA);
    HDC hdcStatic;
    switch (msg)
    {/*
    case WM_CTLCOLORSTATIC:

        hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, RGB(0, 0, 0));
        SetBkMode(hdcStatic, TRANSPARENT);

        return (LRESULT)GetStockObject(NULL_BRUSH);
        */
    //case WM_ERASEBKGND:
    //    return 0;
    }
    return CallWindowProc(folderWindow->textBoxWndProc, hwnd, msg, wParam, lParam);
}

LRESULT __stdcall FolderWindow::MyWndowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

    FolderWindow *folderWindow = (FolderWindow *) ::GetWindowLongPtr(hWnd, GWL_USERDATA);
    HDC hdcStatic;

    switch (message) {
    case WM_NCCREATE:
        ::SetWindowLongPtr(hWnd, GWL_USERDATA, LONG(LPCREATESTRUCT(lParam)->lpCreateParams));
        ((FolderWindow*) ::GetWindowLongPtr(hWnd, GWL_USERDATA))->hWnd = hWnd;
        return DefWindowProcW(hWnd, message, wParam, lParam);
    case WM_CREATE:
        folderWindow->onCreate();
        return DefWindowProcW(hWnd, message, wParam, lParam);
    case WM_DESTROY:
        folderWindow->onDestroy();
        //PostQuitMessage(0);
        break;
    case WM_SIZE:
        folderWindow->onSize();
        break;
    case WM_CTLCOLORSTATIC:
        if (folderWindow->textBoxHandle == (HWND)lParam) {
            hdcStatic = (HDC)wParam;
            SetTextColor(hdcStatic, RGB(0, 0, 0));
            SetBkMode(hdcStatic, TRANSPARENT);

            return (LRESULT)GetStockObject(NULL_BRUSH);
        } else {
            return DefWindowProcW(hWnd, message, wParam, lParam);
        }
    //case WM_ERASEBKGND:
    //    return 1;
    //    break;
    case WM_CLOSE:
        folderWindow->onClose();
        break;
    case WM_LBUTTONDBLCLK:
        folderWindow->onMouseLButtonDblClk();
        break;
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
        break;
    }
    return 0;
}


void FolderWindow::onCreate() {
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    int width = clientRect.right - clientRect.left;
    int height = -(clientRect.top - clientRect.bottom);
    //HFONT hFont = CreateFont(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE,
    //    0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
    //    CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
    //    DEFAULT_PITCH | FF_SWISS, L"Tahoma");
    textBoxHandle = ::CreateWindowExW(WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_NOPARENTNOTIFY | WS_EX_TRANSPARENT,
        L"STATIC", L"", SS_EDITCONTROL | WS_CHILDWINDOW | WS_VISIBLE | ES_READONLY,
        0, 0, 0, 0, hWnd, NULL, ::GetModuleHandleW(NULL), NULL);

    textBoxWndProc = (WNDPROC)SetWindowLongPtr(textBoxHandle, GWL_WNDPROC, (LONG_PTR)((WNDPROC)textBoxMyWndProc));
    //SendMessageW(textBoxHandle, WM_SETFONT, (WPARAM)hFont, 0);
}

void FolderWindow::onMouseLButtonDblClk() {
    visualFS.SetFolderWindow(pathInfo->path, this);
}

void FolderWindow::onSize() {
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    int width = clientRect.right - clientRect.left;
    int height = -(clientRect.top - clientRect.bottom);
}

void FolderWindow::onClose() {
    ::DestroyWindow(hWnd);
}

void FolderWindow::onDestroy() {

}