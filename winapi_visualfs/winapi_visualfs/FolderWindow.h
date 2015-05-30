// Автор: Николай Кочетов
// Описание: Визуализация дерева файловой системы

#pragma once
#include <Windows.h>
#include "FileSystemTree.h"
#include "VisualFS.h"
#include <cmath>

class VisualFS;

class FolderWindow {

protected:
    const int minSize = 5;
    const int textBoxHeight = 15;
    const int borderSize = 1;
    bool isSleeping;
    int x, y;
    int height, width;
    VisualFS &visualFS;

    std::vector<FolderWindow*> children;
    struct SizeInfo {
        FolderWindow *folderWindow;
        size_t size;
        int x, y;
        int height, width;
        SizeInfo(int x, int y, int width, int height) : x(x), y(y), width(width), height(height) {}
        SizeInfo() {}
        bool operator<(const SizeInfo &info) const { return size < info.size; }
    };
    const std::shared_ptr<FileSystemTree::PathInfo> &pathInfo;

    HWND hWnd;
    HWND textBoxHandle;
    WNDPROC textBoxWndProc;
    HWND parentHandle;
public:
    FolderWindow(HWND parentHandle, const std::shared_ptr<FileSystemTree::PathInfo> &pathInfo, VisualFS &visualFS);
    ~FolderWindow();
    size_t GetCurrentSize() { return pathInfo->currSize.load(); }
    fs::wpath GetPath() { return pathInfo->path; }
    void Resize(int x, int y, int width, int height, bool forceRepaint = false);

    void Show(int cmdShow);
    static bool RegisterClass();
    static const wchar_t CLASS_NAME[];
    HWND GetHandle() { return hWnd; }
protected:
    void updateSize(std::vector<SizeInfo> &sizeInfo, size_t begin, size_t end, const SizeInfo &rect);
    static LRESULT __stdcall MyWndowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK textBoxMyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    size_t weightApproximation(size_t weight) { return std::sqrt(weight + 1.0); }
    bool tryUpdateText();
    void killChildren();
    void birthChildren();
    void updateChildrenPos(bool forceRepaint);
    void onCreate();
    void onDestroy();
    void onSize();
    void onClose();
    void onMouseLButtonDblClk();
};
