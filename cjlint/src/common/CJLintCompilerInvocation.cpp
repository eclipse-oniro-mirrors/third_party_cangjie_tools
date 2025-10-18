// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "common/CJLintCompilerInvocation.h"
#include "cangjie/Basic/Print.h"
#include "cangjie/Utils/FileUtil.h"
#include "common/CommonFunc.h"
#include "common/ConfigContext.h"

using namespace Cangjie::CodeCheck;
static const int SINGLE = 1;

// Helper function to split a string by a delimiter
static std::vector<std::string> Split(const std::string& str, std::string delimiter)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter[0])) {
        tokens.push_back(token);
    }
    return tokens;
}

// Function to find the smallest common path
static std::string FindCommonPath(const std::vector<std::string>& paths)
{
    if (paths.empty()) {
        return "";
    }

    // Determine the delimiter based on the first path
    std::string delimiter = (paths[0].find('\\') != std::string::npos) ? "\\" : "/";

    // Split the first path to initialize the common path
    std::vector<std::string> commonComponents = Split(paths[0], delimiter);

    for (const auto& path : paths) {
        std::vector<std::string> components = Split(path, delimiter);
        // Compare each component with the common components
        for (size_t i = 0; i < commonComponents.size(); ++i) {
            if (i >= components.size() || commonComponents[i] != components[i]) {
                // Truncate the common components to the matched part
                commonComponents.resize(i);
                break;
            }
        }
    }

    // Join the common components back into a path
    std::string commonPath;
    for (const auto& component : commonComponents) {
        if (component.empty()) {
            continue;
        }
        if (!commonPath.empty()) {
            commonPath += delimiter;
        } else if (delimiter == "/") {
            commonPath = delimiter;
        }
        commonPath += component;
    }
    return commonPath;
}

static void AddFileToSrcFiles(std::string& srcDir, std::vector<std::string>& srcDirs)
{
    auto files = Cangjie::FileUtil::GetAllFilesUnderCurrentPath(srcDir, "cj", false);
    if (!files.empty()) {
        auto it = std::find(srcDirs.begin(), srcDirs.end(), srcDir);
        if (it == srcDirs.end()) {
            srcDirs.emplace_back(srcDir);
        }
    }
}

std::unique_ptr<CJLintCompilerInstance> CJLintCompilerInvocation::PrePareCompilerInstance(DiagnosticEngine& diag)
{
    ConfigContext &configContext = ConfigContext::GetInstance();
    auto moduleDir = configContext.GetModulesDir();
    invocation = std::make_unique<CompilerInvocation>();
#ifdef __ARM__
    invocation->globalOptions.target = Triple::Info{ Triple::ArchType::AARCH64, Triple::Vendor::UNKNOWN,
        Triple::OSType::LINUX, Triple::Environment::GNU };
#endif
    invocation->globalOptions.compilePackage = true;
    invocation->globalOptions.outputMode = GlobalOptions::OutputMode::SHARED_LIB;
    auto fileDirs = configContext.GetSrcFileDir();
    std::vector<std::string> srcDirs;
    for (auto fileDir : fileDirs) {
        for (auto& srcDir : FileUtil::GetAllDirsUnderCurrentPath(fileDir)) {
            AddFileToSrcFiles(srcDir, srcDirs);
        }
        AddFileToSrcFiles(fileDir, srcDirs);
    }
    std::string commonPath = fileDirs[0];
    if (fileDirs.size() != SINGLE) {
        auto commonPath = FindCommonPath(fileDirs);
    }
    auto it = std::find(srcDirs.begin(), srcDirs.end(), commonPath);
    if (it == srcDirs.end()) {
        srcDirs.emplace_back(commonPath);
    }
    invocation->globalOptions.packagePaths = srcDirs;
    invocation->globalOptions.srcFiles = configContext.GetSrcFileList();
    invocation->globalOptions.implicitPrelude = true;
    invocation->globalOptions.moduleSrcPath = commonPath;
    invocation->globalOptions.chirWFC = false;
    invocation->globalOptions.disableInstantiation = true;
    invocation->globalOptions.parseTest = true;
    invocation->globalOptions.enableAddCommentToAst = true;
    auto cjoPath = configContext.GetCjoPath();
    if (!cjoPath.empty()) {
        auto cjoPathList = Utils::SplitString(cjoPath, " ");
        for (auto cjoDir: cjoPathList) {
            auto maybePath = invocation->globalOptions.CheckDirectoryPath(cjoDir);
            if (maybePath.has_value()) {
                invocation->globalOptions.importPaths.emplace_back(maybePath.value());
            }
        }
    }
    invocation->globalOptions.selectedCHIROpts.insert(GlobalOptions::OptimizationFlag::CONST_PROPAGATION);
    auto libPathName = invocation->globalOptions.GetCangjieLibTargetPathName();
    invocation->globalOptions.executablePath =
        FileUtil::JoinPath(ConfigContext::GetInstance().GetCangjieHome(), CJC_PATH);
    auto compilerInstance = std::make_unique<CJLintCompilerInstance>(*invocation, diag);

    diag.SetSourceManager(&compilerInstance->GetSourceManager());
    compilerInstance->cangjieHome = ConfigContext::GetInstance().GetCangjieHome();
    compilerInstance->cangjieModules = FileUtil::JoinPath(FileUtil::JoinPath(moduleDir, "modules"), libPathName);
    return std::move(compilerInstance);
}

CJLintCompilerInvocation::CJLintCompilerInvocation() = default;
CJLintCompilerInvocation::~CJLintCompilerInvocation() = default;

void CJLintCompilerInvocation::DesInitRuntime()
{
    RuntimeInit::GetInstance().CloseRuntime();
}
