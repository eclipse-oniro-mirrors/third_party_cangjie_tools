// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "SyscapCheck.h"
#include "Utils.h"

using namespace Cangjie::AST;
using namespace Cangjie::FileUtil;

namespace {
const std::string CFG_PARAM_SYSCAP_NAME = "APILevel_syscap";
struct JsonObject;

struct JsonPair {
    std::string key;
    std::vector<std::string> valueStr;
    std::vector<OwnedPtr<JsonObject>> valueObj;
    std::vector<uint64_t> valueNum;
};

struct JsonObject {
    std::vector<OwnedPtr<JsonPair>> pairs;
};

enum class StringMod {
    KEY,
    VALUE,
};

std::string ParseJsonString(size_t& pos, const std::vector<uint8_t>& in)
{
    std::stringstream str;
    if (pos < in.size() && in[pos] == '"') {
        ++pos;
        while (pos < in.size() && in[pos] != '"') {
            str << in[pos];
            ++pos;
        }
    }
    return str.str();
}

uint64_t ParseJsonNumber(size_t& pos, const std::vector<uint8_t>& in)
{
    if (pos >= in.size() || in[pos] < '0' || in[pos] > '9') {
        return 0;
    }
    std::stringstream num;
    while (pos < in.size() && in[pos] >= '0' && in[pos] <= '9') {
        num << in[pos];
        ++pos;
    }
    if (num.str().size()) {
        --pos;
    }
    try {
        return std::stoull(num.str());
    } catch (...) {
        return 0;
    }
}

OwnedPtr<JsonObject> ParseJsonObject(size_t& pos, const std::vector<uint8_t>& in);
void ParseJsonArray(size_t& pos, const std::vector<uint8_t>& in, Ptr<JsonPair> value)
{
    if (pos >= in.size() || in[pos] != '[') {
        return;
    }
    ++pos;
    while (pos < in.size()) {
        if (in[pos] == ' ' || in[pos] == '\n') {
            ++pos;
            continue;
        }
        if (in[pos] == '"') {
            value->valueStr.emplace_back(ParseJsonString(pos, in));
        }
        if (in[pos] == '{') {
            value->valueObj.emplace_back(ParseJsonObject(pos, in));
        }
        if (in[pos] == ']') {
            return;
        }
        ++pos;
    }
}

OwnedPtr<JsonObject> ParseJsonObject(size_t& pos, const std::vector<uint8_t>& in)
{
    if (pos >= in.size() || in[pos] != '{') {
        return nullptr;
    }
    ++pos;
    auto ret = MakeOwned<JsonObject>();
    auto mod = StringMod::KEY;
    while (pos < in.size()) {
        if (in[pos] == ' ' || in[pos] == '\n') {
            ++pos;
            continue;
        }
        if (in[pos] == '}') {
            return ret;
        }
        if (in[pos] == ':') {
            mod = StringMod::VALUE;
        }
        if (in[pos] == ',') {
            mod = StringMod::KEY;
        }
        if (in[pos] == '"') {
            if (mod == StringMod::KEY) {
                auto newData = MakeOwned<JsonPair>();
                newData->key = ParseJsonString(pos, in);
                ret->pairs.emplace_back(std::move(newData));
            } else {
                ret->pairs.back()->valueStr.emplace_back(ParseJsonString(pos, in));
            }
        }
        if (in[pos] >= '0' && in[pos] <= '9') {
            ret->pairs.back()->valueNum.emplace_back(ParseJsonNumber(pos, in));
        }
        if (in[pos] == '{') {
            // The pos will be updated to the pos of matched '}'.
            ret->pairs.back()->valueObj.emplace_back(ParseJsonObject(pos, in));
        }
        if (in[pos] == '[') {
            // The pos will be updated to the pos of matched ']'.
            ParseJsonArray(pos, in, ret->pairs.back().get());
        }
        ++pos;
    }
    return ret;
}

std::vector<std::string> GetJsonString(Ptr<JsonObject> root, const std::string& key)
{
    for (auto& v : root->pairs) {
        if (v->key == key) {
            return v->valueStr;
        }
        for (auto& o : v->valueObj) {
            auto ret = GetJsonString(o.get(), key);
            if (!ret.empty()) {
                return ret;
            }
        }
    }
    return {};
}

Ptr<JsonObject> GetJsonObject(Ptr<JsonObject> root, const std::string& key, const size_t index)
{
    for (auto& v : root->pairs) {
        if (v->key == key && v->valueObj.size() > index) {
            return v->valueObj[index].get();
        }
        for (auto& o : v->valueObj) {
            auto ret = GetJsonObject(o.get(), key, index);
            if (ret) {
                return ret;
            }
        }
    }
    return nullptr;
}

std::unordered_set<std::string> ParseSyscap(Ptr<JsonObject> deviceSysCapObj)
{
    std::unordered_set<std::string> intersectionSet{};
    for (auto& subObj : deviceSysCapObj->pairs) {
        for (auto& path : subObj->valueStr) {
            std::vector<uint8_t> buffer;
            std::string failedReason;
            Cangjie::FileUtil::ReadBinaryFileToBuffer(path, buffer, failedReason);
            if (!failedReason.empty()) {
                continue;
            }
            size_t startPos = static_cast<size_t>(std::find(buffer.begin(), buffer.end(), '{') - buffer.begin());
            auto rootOneDevice = ParseJsonObject(startPos, buffer);
            auto curSyscaps = GetJsonString(rootOneDevice, "SysCaps");
            for (auto syscap : curSyscaps) {
                intersectionSet.emplace(syscap);
            }
        }
    }
    return intersectionSet;
}
}

namespace ark {
std::unordered_map<std::string, SysCapSet> SyscapCheck::module2SyscapsMap{};

SyscapCheck::SyscapCheck(const std::string& moduleName)
{
    if (module2SyscapsMap.find(moduleName) != module2SyscapsMap.end()) {
        intersectionSet = module2SyscapsMap[moduleName];
    }
}

void SyscapCheck::SetIntersectionSet(const std::string& moduleName)
{
    if (module2SyscapsMap.find(moduleName) != module2SyscapsMap.end()) {
        intersectionSet = module2SyscapsMap[moduleName];
    }
}

void SyscapCheck::ParseCondition(const std::unordered_map<std::string, std::string>& passedWhenKeyValue)
{
    auto found = passedWhenKeyValue.find(CFG_PARAM_SYSCAP_NAME);
    if (found != passedWhenKeyValue.end()) {
        auto syscapsCfgPath = found->second;
        std::vector<uint8_t> jsonContent;
        std::string failedReason;
        Cangjie::FileUtil::ReadBinaryFileToBuffer(syscapsCfgPath, jsonContent, failedReason);
        if (!failedReason.empty()) {
            return;
        }
        ParseJsonFile(jsonContent);
    }
}

void SyscapCheck::ParseJsonFile(const std::vector<uint8_t>& in)
{
    size_t startPos = static_cast<size_t>(std::find(in.begin(), in.end(), '{') - in.begin());
    auto root = ParseJsonObject(startPos, in);
    auto modulesObj = GetJsonObject(root, "Modules", 0);
    for (auto& subObj : modulesObj->pairs) {
        if (subObj == nullptr || subObj->valueObj.empty()) {
            continue;
        }
        auto name = subObj->key;
        if (MessageHeaderEndOfLine::GetIsDeveco()) {
            name = "ohos_app_cangjie_" + name;
        }
        auto deviceSysCapObj = GetJsonObject(subObj->valueObj.front(), "deviceSysCap", 0);
        auto syscapSet = ParseSyscap(deviceSysCapObj);
        module2SyscapsMap[name] = syscapSet;
    }
}

bool SyscapCheck::CheckSysCap(Ptr<Cangjie::AST::Node> node)
{
    auto decl = Cangjie::DynamicCast<Decl*>(node.get());
    if (decl == nullptr) {
        return true;
    }
    return CheckSysCap(*decl);
}

bool SyscapCheck::CheckSysCap(Ptr<Cangjie::AST::Node> node, bool& hasAPILevel) const
{
    auto decl = Cangjie::DynamicCast<Decl*>(node.get());
    if (decl == nullptr) {
        return true;
    }
    for (auto& annotation : decl->annotations) {
        auto name = annotation->identifier;
        if (annotation->identifier != "APILevel") {
            continue;
        }
        hasAPILevel = true;
        for (auto& arg : annotation->args) {
            if (arg->name == "syscap") {
                if (!arg->expr) {
                    return true;
                }
                auto syscapName = ark::GetSysCap(*arg->expr.get());
                if (!syscapName.has_value()) {
                    return true;
                }
                if (intersectionSet.find(syscapName.value()) == intersectionSet.end()) {
                    return false;
                }
                break;
            }
        }
        break;
    }
    return true;
}

bool SyscapCheck::CheckSysCap(Ptr<Cangjie::AST::Decl> decl) const
{
    if (decl == nullptr) {
        return true;
    }
    return CheckSysCap(*decl);
}

bool SyscapCheck::CheckSysCap(const Cangjie::AST::Decl& decl) const
{
    for (auto& annotation : decl.annotations) {
        auto name = annotation->identifier;
        if (annotation->identifier != "APILevel") {
            continue;
        }
        for (auto& arg : annotation->args) {
            if (arg->name != "syscap") {
                continue;
            }
            if (!arg->expr) {
                return true;
            }
            auto syscapName = ark::GetSysCap(*arg->expr.get());
            if (!syscapName.has_value()) {
                return true;
            }
            if (intersectionSet.find(syscapName.value()) == intersectionSet.end()) {
                return false;
            }
        }
        break;
    }
    return true;
}

bool SyscapCheck::CheckSysCap(const std::string& syscapName)
{
    return intersectionSet.find(syscapName) != intersectionSet.end();
}
} // namespace ark