// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "DocCache.h"
#include <iostream>
#include <utility>
#include "logger/Logger.h"
#include "common/BasicHelper.h"

namespace ark {
    using namespace Cangjie;
    static void UpdateVersion(DocCache::Doc &doc, int64_t version)
    {
        version++;
        if (doc.version == -1) {
            ++doc.version;
        } else {
            // We treat versions as opaque, but the protocol says they increase.
            if (version <= doc.version) {
                std::stringstream message;
                message << "warning:File version went from" << doc.version << "to" << version;
                Logger::Instance().LogMessage(MessageType::MSG_WARNING, message.str());
            }
            doc.version = version;
        }
    }

    int64_t DocCache::AddDoc(const std::string &file, int64_t version, std::string contents)
    {
        std::lock_guard<std::mutex> lock(mutex);
        Doc &doc = Docs[file];
        UpdateVersion(doc, version);
        doc.contents = std::move(contents);
        return doc.version;
    }

    void DocCache::AddDocWhenInitCompile(const std::string &file)
    {
        std::lock_guard<std::mutex> lock(mutex);
        Doc &doc = Docs[file];
        doc.isInitCompiled = true;
    }

    bool DocCache::UpdateDoc(const std::string &file, int64_t version, bool needReParser,
                             const std::vector<TextDocumentContentChangeEvent> &contentChanges)
    {
        std::lock_guard<std::mutex> lock(mutex);
        const auto entryIt = Docs.find(file);
        if (entryIt == Docs.end()) {
            return false;
        }
        Doc &curDoc = entryIt->second;
        std::string contents = entryIt->second.contents;
        for (const TextDocumentContentChangeEvent &change : contentChanges) {
            if (!change.range.has_value()) {
                contents = change.text;
                continue;
            }

            const Position &start = change.range.value().start;
            const int32_t startIndex = GetOffsetFromPosition(contents, start);
            if (startIndex < 0) {
                return false;
            }
            const Position &end = change.range.value().end;
            const int32_t endIndex = GetOffsetFromPosition(contents, end);
            if (endIndex < 0 || endIndex < startIndex || endIndex > static_cast<int>(contents.length())) {
                return false;
            }

            std::string newContents;
            newContents.reserve(static_cast<std::string::size_type>(startIndex) + change.text.length()
            + (contents.length() - static_cast<std::string::size_type>(endIndex)));
            newContents = startIndex == 0 ? "" : contents.substr(0,
                                                                 static_cast<std::string::size_type>(startIndex));
            newContents += change.text;
            newContents += contents.substr(static_cast<std::string::size_type>(endIndex));

            contents = std::move(newContents);
        }
        UpdateVersion(curDoc, version);
        curDoc.needReParser = needReParser;
        curDoc.contents = std::move(contents);
        return true;
    }

    void DocCache::UpdateDocNeedReparse(const std::string &file, int64_t version, bool needReParser)
    {
        std::lock_guard<std::mutex> lock(mutex);
        const auto entryIt = Docs.find(file);
        if (entryIt == Docs.end()) {
            return;
        }
        Doc &curDoc = entryIt->second;
        if (version == curDoc.version) {
            curDoc.needReParser = needReParser;
        }
    }

    void DocCache::RemoveDoc(const std::string &file)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (Docs.find(file) == Docs.end()) {
            return;
        }
        (void)Docs.erase(file);
    }

    DocCache::Doc DocCache::GetDoc(const std::string &file)
    {
        std::lock_guard<std::mutex> lock(mutex);
        const auto iter = Docs.find(file);
        if (iter == Docs.end()) {
            Doc doc;
            doc.contents = "";
            doc.version = -1;
            return doc;
        }
        return iter->second;
    }
} // namespace ark
