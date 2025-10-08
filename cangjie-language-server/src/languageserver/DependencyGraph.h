// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#ifndef LSPSERVER_DEPENDENCY_GRAPH_H
#define LSPSERVER_DEPENDENCY_GRAPH_H

#include <algorithm>
#include <iostream>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "logger/Logger.h"

namespace ark {
class DependencyGraph {
public:
    // Get all packages that package 'a' directly depends on
    std::unordered_set<std::string> GetDependencies(const std::string &a) const
    {
        std::lock_guard<std::mutex> lock(graphMutex);
        auto it = dependencies.find(a);
        return it != dependencies.end() ? it->second : std::unordered_set<std::string>();
    }

    // Get all packages that depend on package 'a'
    std::unordered_set<std::string> GetDependents(const std::string &a) const
    {
        std::lock_guard<std::mutex> lock(graphMutex);
        auto it = reverseDependencies.find(a);
        return it != reverseDependencies.end() ? it->second : std::unordered_set<std::string>();
    }

    // Update dependencies for a package
    void UpdateDependencies(const std::string &package, const std::set<std::string> &newDependencies)
    {
        std::lock_guard<std::mutex> lock(graphMutex);

        // First, remove all existing dependencies
        if (dependencies.find(package) != dependencies.end()) {
            for (const auto &dep : dependencies[package]) {
                reverseDependencies[dep].erase(package);
            }
            dependencies[package].clear();
        }

        // Add new dependencies
        if (newDependencies.empty()) {
            dependencies[package].insert({});
        }

        for (const auto &newDep : newDependencies) {
            auto ret = dependencies[package].insert(newDep);
            if (!ret.second) {
                Trace::Elog("dependencies insert failed");
            }
            reverseDependencies[newDep].insert(package);
        }
    }

    // Find all dependencies (direct and transitive) of a given package using DFS
    std::unordered_set<std::string> FindAllDependencies(const std::string &a) const
    {
        std::unordered_set<std::string> allDeps;
        std::unordered_set<std::string> visited;

        std::lock_guard<std::mutex> lock(graphMutex);
        DfsDependencies(a, visited, allDeps);

        return allDeps;
    }

    // Find all dependents (direct and transitive) of a given package using DFS
    std::unordered_set<std::string> FindAllDependents(const std::string &a) const
    {
        std::unordered_set<std::string> allDeps;
        std::unordered_set<std::string> visited;

        std::lock_guard<std::mutex> lock(graphMutex);
        DfsDependent(a, visited, allDeps);

        return allDeps;
    }

    // Topological Sort
    std::vector<std::string> TopologicalSort() const
    {
        std::unordered_set<std::string> visited;
        std::unordered_set<std::string> recStack;
        std::vector<std::string> result;

        std::lock_guard<std::mutex> lock(graphMutex);
        for (const auto &pair : dependencies) {
            if (visited.find(pair.first) == visited.end()) {
                if (!TopoSortUtil(pair.first, visited, recStack, result)) {
                    Trace::Elog("Return empty for cyclic graph");
                    return {};
                }
            }
        }

        return result;
    }

    // Find a cycle that includes the given package
    std::pair<std::vector<std::vector<std::string>>, bool> FindCycles() const
    {
        std::lock_guard<std::mutex> lock(graphMutex);
        std::vector<std::vector<std::string>> allCycles;
        std::unordered_set<std::string> visited;
        std::unordered_set<std::string> inPath;

        for (const auto &pair : dependencies) {
            if (visited.find(pair.first) == visited.end()) {
                std::vector<std::string> path;
                CyclesDFS(pair.first, visited, inPath, path, allCycles);
            }
        }

        return {allCycles, !allCycles.empty()};
    }

    // For debug: Print all dependencies in the graph
    void PrintDependencies() const
    {
        std::lock_guard<std::mutex> lock(graphMutex);
        std::cerr << "Dependency Graph:\n";
        for (const auto &pair : dependencies) {
            const std::string &package = pair.first;
            const std::unordered_set<std::string> &deps = pair.second;

            std::cerr << package << " depends on: ";
            if (deps.empty()) {
                std::cerr << "No dependencies";
            } else {
                for (const auto &dep : deps) {
                    std::cerr << dep << " ";
                }
            }
            std::cerr << "\n";
        }
    }

private:
    std::unordered_map<std::string, std::unordered_set<std::string>> dependencies;        // 上游包
    std::unordered_map<std::string, std::unordered_set<std::string>> reverseDependencies; // 下游包
    mutable std::mutex graphMutex;

    // Helper function for find all dependencies, will be called with pre-acquired lock
    void DfsDependencies(const std::string &package,
        std::unordered_set<std::string> &visited,
        std::unordered_set<std::string> &result) const
    {
        if (visited.find(package) != visited.end()) {
            return;
        }
        visited.insert(package);

        // Access dependencies within locked scope
        auto it = dependencies.find(package);
        if (it != dependencies.end()) {
            for (const auto &dep : it->second) {
                if (visited.find(dep) == visited.end()) {
                    result.insert(dep);
                    DfsDependencies(dep, visited, result);
                }
            }
        }
    }

    // Helper function for find all dependent, will be called with pre-acquired lock
    void DfsDependent(const std::string &package,
        std::unordered_set<std::string> &visited,
        std::unordered_set<std::string> &result) const
    {
        if (visited.find(package) != visited.end()) {
            return;
        }
        visited.insert(package);

        // Access dependencies within locked scope
        auto it = reverseDependencies.find(package);
        if (it != reverseDependencies.end()) {
            for (const auto &dep : it->second) {
                if (visited.find(dep) == visited.end()) {
                    result.insert(dep);
                    DfsDependent(dep, visited, result);
                }
            }
        }
    }

    void CyclesDFS(const std::string &package,
        std::unordered_set<std::string> &visited,
        std::unordered_set<std::string> &inPath,
        std::vector<std::string> &path,
        std::vector<std::vector<std::string>> &cycles) const
    {
        visited.insert(package);
        inPath.insert(package);
        path.push_back(package);

        auto it = dependencies.find(package);
        if (it != dependencies.end()) {
            for (const auto &dep : it->second) {
                if (inPath.find(dep) != inPath.end()) {
                    // Found a cycle
                    auto cycleStart = std::find(path.begin(), path.end(), dep);
                    cycles.push_back(std::vector<std::string>(cycleStart, path.end()));
                } else if (visited.find(dep) == visited.end()) {
                    CyclesDFS(dep, visited, inPath, path, cycles);
                }
            }
        }

        path.pop_back();
        inPath.erase(package);
    }

    // Helper function for Topological Sort
    bool TopoSortUtil(const std::string &node,
        std::unordered_set<std::string> &visited,
        std::unordered_set<std::string> &recStack,
        std::vector<std::string> &result,
        const bool isCycleDependencies = false) const
    {
        if (recStack.find(node) != recStack.end()) {
            if (isCycleDependencies) {
                return true;
            }
            return false; // detect cycle
        }

        if (visited.find(node) == visited.end()) {
            visited.insert(node);
            recStack.insert(node);

            // Access dependencies within locked scope
            auto it = dependencies.find(node);
            if (it != dependencies.end()) {
                for (const std::string &neighbor : it->second) {
                    if (!TopoSortUtil(neighbor, visited, recStack, result, neighbor == node)) {
                        return false;
                    }
                }
            }

            recStack.erase(node);
            result.push_back(node); // our current finish order
        }
        return true;
    }
};
} // namespace ark
#endif // LSPSERVER_DEPENDENCY_GRAPH_H