// Автор: Николай Кочетов
// Описание: Визуализация дерева файловой системы

#pragma once
#include <Windows.h>
#include "FileSystemTree.h"
#include "FolderWindow.h"

class FolderWindow;

class VisualFS {

protected:
    HWND hWnd;
    FileSystemTree fsTree;
    FolderWindow *rootFolder;
    HWND pathTextBoxHandle;
    HWND returnButtonHandle;
    HWND openFileDlgButtonHandle;
    WNDPROC pathTextBoxWndProc;
    fs::wpath currentPath;
    const int textBoxWndProcHeight = 20;
    const int returnButtonWidth = 30;
    const int openFileDlgButtonWidth = 30;
public:
    VisualFS() : rootFolder(NULL) {}
    ~VisualFS() { if (rootFolder) delete rootFolder; }
    bool Create(const fs::wpath &path);
    void Show(int cmdShow);
    void SetFolderWindow(fs::wpath &newPath, FolderWindow *newFolder = NULL);
    static bool RegisterClass();
    static const wchar_t CLASS_NAME[];
    HWND GetHandle() { return hWnd; }
protected:
    static LRESULT __stdcall MyWndowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK pathTextBoxMyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void onCreate();
    void onDestroy();
    void onSize();
    void onClose();
    void onTimer();
    void onPathTextBoxEnter();
    void onPathTextBoxKillFocus();
    void onMouseLButtonDblClk() {}
    void onReturnButtonClicked();
    void onOpenDlgButtonClicked();
};
