// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "common/ConfigContext.h"
#include "cangjie/Utils/FileUtil.h"
#include "checker/Checker.h"

using namespace Cangjie::CodeCheck;

void ConfigContext::SetSrcFileDir(size_t index, std::string srcFile)
{
    srcFileDir[index] = srcFile;
}

void ConfigContext::AddSrcFileDir(const std::string srcFileDirInput)
{
    auto srcFileList = Utils::SplitString(srcFileDirInput, " ");
    for (auto srcFile: srcFileList) {
        srcFileDir.emplace_back(srcFile);
    }
}

void ConfigContext::SetCjoPath(const std::string cjoPathStr)
{
    cjoPath = cjoPathStr;
}

void ConfigContext::SetConfigFileDir(const std::string configFileDirInput)
{
    this->configFileDir = std::move(configFileDirInput);
}

void ConfigContext::SetModulesDir(const std::string moduleFileDirInput)
{
    this->modulesDir = std::move(moduleFileDirInput);
}

void ConfigContext::AddExcludeList(const std::string excludeFile)
{
    if (excludeFile.rfind(DEFAULT_EXCLUSION_FORMAT) == excludeFile.size() - DEFAULT_EXCLUSION_FORMAT.size()) {
        // config file
        this->defaultExclusion = false;
        auto absConfigDir = FileUtil::GetAbsPath(FileUtil::JoinPath(this->srcFileDir[0], excludeFile));
        if (!absConfigDir.has_value()) {
            return;
        }
        auto configDir = absConfigDir.value();
        std::ifstream configInput(configDir);
        std::string configInputLine;
        auto separatorIndex = configDir.rfind(PATH_SEPARATOR);
        if (separatorIndex != std::string::npos) {
            configDir = configDir.substr(0, separatorIndex);
        }
        while (std::getline(configInput, configInputLine)) {
            this->AddExcludeRule(configDir, configInputLine);
        }
    } else {
        this->AddExcludeRule(this->srcFileDir[0], excludeFile);
    }
}

void ConfigContext::SetReportFile(const std::string reportFileInput)
{
    this->reportFile = reportFileInput;
}

bool ConfigContext::SetReportFormat(const std::string reportFormatInput)
{
    if (reportFormatInput != "json" && reportFormatInput != "csv") {
        return false;
    }
    this->reportFormat = reportFormatInput;
    return true;
}

void ConfigContext::SetCangjieHome(const std::string cangjieHomePath)
{
    this->cangjieHome = std::move(cangjieHomePath);
}

const std::vector<std::string>& ConfigContext::GetSrcFileDir() const
{
    return srcFileDir;
}

const std::string& ConfigContext::GetConfigFileDir() const
{
    return configFileDir;
}
const std::string& ConfigContext::GetModulesDir() const
{
    return modulesDir;
}

const std::string& ConfigContext::GetCjoPath() const
{
    return cjoPath;
}

const std::vector<ExcludeRule>& ConfigContext::GetExcludeList() const
{
    return excludeList;
}

const std::string& ConfigContext::GetReportFile() const
{
    return reportFile;
}

const std::string& ConfigContext::GetReportFormat() const
{
    return reportFormat;
}

const std::string& ConfigContext::GetCangjieHome() const
{
    return cangjieHome;
}

const std::string ConfigContext::GetCangjieHomePath() const
{
    std::string path;
    if (!cangjieHome.empty() && FileUtil::GetAbsPath(cangjieHome).has_value()) {
        auto absCangjieHomePath = FileUtil::GetAbsPath(cangjieHome);
        path = absCangjieHomePath->c_str();
    } else {
        path = ".";
    }
    return path;
}

std::string ConfigContext::GetCjlintConfigPath()
{
    auto newPath = GetCangjieHomePath();
    if (newPath != ".") {
        newPath += configPath;
    }
    return newPath;
}

const std::vector<std::string>& ConfigContext::GetSrcFileList() const
{
    return this->srcFileList;
}

void ConfigContext::SetSrcFileList(const std::vector<std::string>& srcFileList)
{
    this->srcFileList.clear();
    for (auto srcFile: srcFileList) {
        this->srcFileList.emplace_back(srcFile);
    }
}

void ConfigContext::FilterCompileList(const std::vector<std::string> &fileList)
{
    this->srcFileList.clear();
    for (auto &file: fileList) {
        size_t index;
        for (index = this->excludeList.size(); index > 0; --index) {
            if (this->excludeList[index - 1].IsMatched(file)) {
                break;
            }
        }
        if (index == 0) {
            // Nothing is matched
            this->srcFileList.emplace_back(file);
            continue;
        }
        auto matchedRule = this->excludeList[index - 1];
        if (matchedRule.GetRuleType() == ExcludeRule::ExcludeRuleType::IGNORE) {
            // Match a rule with type IGNORE, exclude this file
            continue;
        }
        for (--index; index > 0; --index) {
            // Match a rule with type INCLUDE, check if its directory is excluded
            if (this->excludeList[index - 1].IsMatched(file)
                && (this->excludeList[index - 1].GetRuleTarget() == ExcludeRule::ExcludeRuleTarget::DIRECTORY)) {
                break;
            }
        }
        if (index == 0) {
            // Parent directory is not excluded
            this->srcFileList.emplace_back(file);
        }
    }
}

void ConfigContext::AddExcludeRule(const std::string &excludeDir, const std::string &exclude)
{
    ExcludeRule rule(excludeDir, exclude);
    if (rule.isValid) {
        this->excludeList.emplace_back(rule);
    }
}

void ConfigContext::CheckDefaultExclusion()
{
    if (this->defaultExclusion) {
        this->AddExcludeList(DEFAULT_EXCLUSION_FILE);
    }
}
