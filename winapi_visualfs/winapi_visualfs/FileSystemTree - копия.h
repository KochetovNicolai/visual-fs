#pragma once
#include <filesystem>
#include <memory>
#include <atomic>
#include <map>
#include <vector>
#include <iostream>
#include <thread>

namespace fs = std::tr2::sys;

class FileSystemTree {
public:
    struct PathInfo {
        fs::wpath path;
        std::atomic<char> isUpdated;
        std::atomic<size_t> currSize;
        std::vector<std::shared_ptr<PathInfo>> children;
        PathInfo(const fs::wpath &path) :
            path(path), isUpdated(false), children(0), currSize(0) {
            std::wcerr << L"Enter: " << path.directory_string() << L"\n";
        }
        ~PathInfo() {
            std::wcerr << L"Leave: " << currSize.load() << L" " << path.directory_string() << L"\n";
        }
    };
    const std::shared_ptr<PathInfo>& OpenPath(const fs::wpath &path) {
        rootInfo.push_back(std::shared_ptr<PathInfo>(new PathInfo(path)));
        threads.push_back(std::shared_ptr<std::thread>(new std::thread(task, rootInfo.back())));
        threads.back()->detach();
        return rootInfo.back();
    } 
private:
    std::vector<std::shared_ptr<std::thread>> threads;
    std::vector<std::shared_ptr<PathInfo>> rootInfo;
    static void task(std::shared_ptr<PathInfo> &pathInfo) {
        pathInfo->currSize.store(fs::file_size(pathInfo->path));
        if (fs::is_directory(pathInfo->path)) {
            for (fs::wdirectory_iterator iter = fs::wdirectory_iterator(pathInfo->path); iter != fs::wdirectory_iterator(); ++iter) 
                pathInfo->children.push_back(std::shared_ptr<PathInfo>(new PathInfo(iter->path())));
            pathInfo->isUpdated.store(true);
            for (size_t i = 0; i < pathInfo->children.size(); i++) {
                task(pathInfo->children[i]);
                pathInfo->currSize.store(pathInfo->currSize.load() + pathInfo->children[i]->currSize.load());
            }
        }
        pathInfo->isUpdated.store(true);
        std::cerr.flush();
    }
};