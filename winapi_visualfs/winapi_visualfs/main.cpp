#include "FileSystemTree.h"
#include "VisualFS.h"
#include <iostream>
#include <dos.h>
#include <cstdio>
#include <Windows.h>

int __stdcall  WinMain(HINSTANCE hInstance, HINSTANCE hPrevIns, LPSTR commandLine, int nCMdShow) {

    VisualFS::RegisterClass();
    FolderWindow::RegisterClass();

    wchar_t buf[100];
    ::GetFullPathNameW(L".", 100, buf, NULL);
    std::wstring wstr(buf);
    VisualFS visualFS;
  
    visualFS.Create(fs::wpath(wstr.c_str()));
    visualFS.Show(nCMdShow);

    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

/*
int main() {
    wchar_t buf[100];
    ::GetFullPathName(L".", 100, buf, NULL);
    std::locale::global(std::locale(""));
    fs::wpath p = fs::wpath(buf);
    std::wcout << p << std::endl;
    std::wcout << p.parent_path() << std::endl;
    std::wcout << p.has_root_name() << ' ' << fs::wpath(L".").has_root_name() << std::endl;
    //std::cout << fs::current_path() << std::endl;
    
    std::cout << fs::file_size(fs::wpath(L"C:\\Users\\Николай\\Downloads\\фото\\tmp\\1.jpg")) << std::endl;
    fs::wpath path(L"C:\\Users\\Николай\\Downloads\\фото\\tmp\\1.jpg");
    std::wcout << path.filename() << std::endl;
    std::wcout << path.directory_string() << std::endl;
    std::wcout << path.branch_path() << std::endl;
    std::wcout << path.external_file_string() << std::endl;
    {
        FileSystemTree fsTree;
        fsTree.OpenPath(fs::wpath(L"C:\\Users\\Николай\\Documents\\Visual Studio 2013\\Projects\\winapi_visualfs"));
        std::cin.get();
    }
    system("pause");
    return 0;
}
*/