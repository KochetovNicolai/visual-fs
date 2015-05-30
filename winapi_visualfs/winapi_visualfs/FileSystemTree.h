#pragma once
#include <filesystem>
#include <memory>
#include <atomic>
#include <map>
#include <vector>
#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

namespace fs = std::tr2::sys;

class FileSystemTree {
public:
    struct PathInfo {
        fs::wpath path;
        std::atomic<char> isUpdated;
        std::atomic<size_t> currSize;
        std::vector<std::shared_ptr<PathInfo>> children;
        std::weak_ptr<PathInfo> parent;
        PathInfo(const fs::wpath &path, const std::weak_ptr<PathInfo> &parent) :
            path(path), parent(parent), isUpdated(false), children(0), currSize(0) {
            std::wcerr << L"Enter: " << path.directory_string() << L"\n";
        }
        ~PathInfo() {
            std::wcerr << L"Leave: " << currSize.load() << L" " << path.directory_string() << L"\n";
        }
    };
    FileSystemTree() {
        rootPathInfo = nullptr;
        currentPathInfo = nullptr;
        onExit = false;
        thread = std::shared_ptr<std::thread>(new std::thread(task, this));
    }
    ~FileSystemTree() {
        {
            std::unique_lock<std::mutex> lock(mutex);
            onExit = true;
            wait.notify_one();
        }
        thread->join();
    }
    const std::shared_ptr<PathInfo>& OpenPath(const fs::wpath &path) {
        if (currentPathInfo != nullptr && currentPathInfo->path == path)
            return currentPathInfo;
        {
            std::unique_lock<std::mutex> lock(mutex);
            rootPathInfo = currentPathInfo = std::shared_ptr<PathInfo>(new PathInfo(path, std::weak_ptr<PathInfo>()));
            wait.notify_one();
        }
        return currentPathInfo;
    } 
    const std::shared_ptr<PathInfo>& OpenChildPath(const fs::wpath &path) {
        if (currentPathInfo == nullptr)
            return OpenPath(path);
        std::shared_ptr<PathInfo> pathInfo = currentPathInfo;
        while (true) {
            std::unique_lock<std::mutex> lock(mutex);
            if (!pathInfo->isUpdated.load())
                break;
            while (true) {
                if (pathInfo->path == path) {
                    currentPathInfo = pathInfo;
                    wait.notify_one();
                    return currentPathInfo;
                }
                bool foundSubPath = false;
                for (size_t i = 0; !foundSubPath && i < pathInfo->children.size(); i++)
                    if (pathInfo->children[i]->isUpdated.load() && isSubPath(path, pathInfo->children[i]->path)) {
                        foundSubPath = true;
                        pathInfo = pathInfo->children[i];
                    }
                if (!foundSubPath)
                    break;
            }
            break;
        }
        return OpenPath(path);
    }
    const std::shared_ptr<PathInfo>& OpenRootPath() {
        if (currentPathInfo == nullptr || !currentPathInfo->path.has_parent_path())
            return currentPathInfo;
        {
            std::unique_lock<std::mutex> lock(mutex);
            if (currentPathInfo != rootPathInfo) {
                currentPathInfo = std::shared_ptr<PathInfo>(currentPathInfo->parent);
            } else {
                rootPathInfo = std::shared_ptr<PathInfo>(new PathInfo(currentPathInfo->path.parent_path(), std::weak_ptr<PathInfo>()));
                currentPathInfo->parent = rootPathInfo;
                rootPathInfo->currSize.store(currentPathInfo->currSize.load());
                currentPathInfo = rootPathInfo;
            }
            wait.notify_one();
        }
        return currentPathInfo;
    }
private:
    std::mutex mutex;
    std::condition_variable wait;
    std::shared_ptr<std::thread> thread;
    std::shared_ptr<PathInfo> rootPathInfo;
    std::shared_ptr<PathInfo> currentPathInfo;
    bool onExit;
    bool isSubPath(fs::wpath path, fs::wpath root) {
        if (path == root)
            return true;
        while (path.has_parent_path()) {
            path = path.parent_path();
            if (path == root) {
                return true;
            }
        }
        return false;
    }
    static void task(FileSystemTree *fsTree) {
        std::shared_ptr<PathInfo> rootPathInfo(nullptr);
        while (true) {
            std::queue<std::shared_ptr<PathInfo> > queue;
            {
                std::unique_lock<std::mutex> lock(fsTree->mutex);
                while ((fsTree->currentPathInfo == nullptr || fsTree->currentPathInfo == rootPathInfo) && !fsTree->onExit)
                    fsTree->wait.wait(lock);
                if (fsTree->onExit)
                    return;
                rootPathInfo = fsTree->currentPathInfo;
            }
            queue.push(rootPathInfo);
            while (!queue.empty()) {
                if (fsTree->currentPathInfo != rootPathInfo || fsTree->onExit)
                    break;
                std::shared_ptr<PathInfo> pathInfo = queue.front();
                queue.pop();
                if (!pathInfo->isUpdated.load()) {
                    pathInfo->currSize.store(fs::file_size(pathInfo->path));
                    if (fs::is_directory(pathInfo->path)) {
                        fs::wdirectory_iterator it;
                        for (it = fs::wdirectory_iterator(pathInfo->path); it != fs::wdirectory_iterator() && !fsTree->onExit; ++it)
                            pathInfo->children.push_back(std::shared_ptr<PathInfo>(new PathInfo(it->path(), pathInfo)));
                    }
                    pathInfo->isUpdated.store(true);
                    if (!pathInfo->children.size()) {
                        size_t addSize = pathInfo->currSize.load();
                        std::weak_ptr<PathInfo> parent(pathInfo->parent);
                        while (!parent.expired()) {
                            std::shared_ptr<PathInfo> ptr(parent);
                            ptr->currSize.store(ptr->currSize.load() + addSize);
                            parent = ptr->parent;
                        }
                    }
                }
                for (size_t i = 0; i < pathInfo->children.size() && !fsTree->onExit; i++)
                    queue.push(pathInfo->children[i]);
            }
        }
    }
};