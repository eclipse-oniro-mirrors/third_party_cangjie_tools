// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include<cstdio>
#include<cstdlib>
#include<iostream>
#include<string>
#include<unistd.h>
#include<algorithm>
#include<common.h>
#include<SingleInstance.h>
#include "../../src/languageserver/ArkLanguageServer.h"
#ifdef _WIN32
#else
#include<sys/types.h>
#include<sys/wait.h>
#endif

using namespace Cangjie::FileUtil;
using namespace std;


SingleInstance *SingleInstance::m_pInstance = nullptr;

namespace {
#ifdef _WIN32
    const std::string TEST_FILE_SEPERATOR = "\\";
#else
    const std::string TEST_FILE_SEPERATOR = "/";
#endif

    const std::string MACRO_PATH = "/Macro/";
    const int MAX_LEN = 1024 * 1024;

    std::string replaceCrLf(const std::string &str) {
        std::string result = str;
        size_t start_pos = 0;
        while ((start_pos = result.find("\\r\\n", start_pos)) != std::string::npos) {
            result.replace(start_pos, 4, "\\n");
            start_pos += 1;
        }
        return result;
    }

    void PrintJson(const nlohmann::json &exp, const std::string &prefix) {
        std::ostringstream actOs;
        actOs << exp.dump(-1);
        std::string lines = actOs.str();
        printf("%s=%s \n", prefix.c_str(), lines.c_str());
    }

    std::optional<std::string> GetRelativePathForTest(const std::string &basePath, const std::string &path) {
        if (basePath == path) {
            return "";
        }
        auto found = path.find(basePath);
        if (found != std::string::npos) {
            return path.substr(found + basePath.size());
        }
        return "";
    }

    std::string ChangeMessageUrlOfString(const std::string &projectPath, const std::string &tmp,
                                         std::string &rootUri,
                                         bool &isMultiModule) {
        std::string relativePath;
        std::string uri;
        if (isMultiModule) {
            relativePath = GetRelativePathForTest(rootUri, tmp).value();
        } else {
            auto index = tmp.find("src");
            if (index == std::string::npos) {
                return "";
            }
            relativePath = tmp.substr(tmp.find("src"));
        }
        uri = ark::PathWindowsToLinux(JoinPath(projectPath, relativePath));
        auto found = uri.find(':');
        if (found != std::string::npos) {
            uri.replace(found, 1, "%3A");
        }
        if (!uri.empty() && uri.back() == '/') {
            uri.pop_back();
        }
        if (TEST_FILE_SEPERATOR == "\\") {
            uri = "file:///" + uri;
        } else {
            uri = "file://" + uri;
        }
        return uri;
    }

    void ChangeMessageUrlOfTextDocument(const std::string &projectPath, nlohmann::json &root,
                                        std::string &rootUri,
                                        bool &isMultiModule) {
        if (root.empty()) {
            return;
        }
        std::string tmp = root.get<std::string>();
        if (tmp.empty()) {
            return;
        }
        std::string temp = ChangeMessageUrlOfString(projectPath, tmp, rootUri, isMultiModule);
        if (temp.empty()) {
            return;
        }
        root = temp;
    }

    // GCOVR_EXCL_START
    void ChangeMessageUrlOfChanges(const std::string &projectPath, nlohmann::json &root, std::string &rootUri,
                                   bool &isMultiModule) {
        auto size = static_cast<int>(root["params"]["changes"].size());
        for (int i = 0; i < size; i++) {
            ChangeMessageUrlOfTextDocument(projectPath, root["params"]["changes"][i]["uri"], rootUri, isMultiModule);
        }
    }

    // GCOVR_EXCL_STOP

    void ChangeMultiModuleOptionUrl(const std::string &projectPath, nlohmann::json &root, std::string &rootUri,
                                    bool &isMultiModule) {
        nlohmann::json multiModuleOptionTemp;
        if (!root.contains("initializationOptions") || !root["initializationOptions"].contains("multiModuleOption")) {
            return;
        }
        isMultiModule = true;
        auto multiModuleOption = root["initializationOptions"]["multiModuleOption"];
        auto moduleOptItems = multiModuleOption.items();
        for (const auto &item: moduleOptItems) {
            auto &moduleOptKey = item.key();
            if (multiModuleOption[moduleOptKey].contains("requires")) {
                auto requiresItems = multiModuleOption[moduleOptKey]["requires"].items();
                for (const auto &requiresItem: requiresItems) {
                    auto &requiresKey = requiresItem.key();
                    if (multiModuleOption[moduleOptKey]["requires"][requiresKey].contains("path")) {
                        ChangeMessageUrlOfTextDocument(projectPath,
                                                       multiModuleOption[moduleOptKey]["requires"][requiresKey]["path"],
                                                       rootUri, isMultiModule);
                    }
                }
            }
            if (multiModuleOption[moduleOptKey].contains("package_requires")) {
                if (multiModuleOption[moduleOptKey]["package_requires"].contains("package_option")) {
                    auto packageOptionItems = multiModuleOption[moduleOptKey]["package_requires"]["package_option"].
                            items();
                    for (const auto &packageOptionItem: packageOptionItems) {
                        auto &packageOptionKey = packageOptionItem.key();
                        ChangeMessageUrlOfTextDocument(projectPath,
                                                       multiModuleOption[moduleOptKey]["package_requires"][
                                                           "package_option"][packageOptionKey],
                                                       rootUri, isMultiModule);
                    }
                }
                if (multiModuleOption[moduleOptKey]["package_requires"].contains("path_option") &&
                    multiModuleOption[moduleOptKey]["package_requires"]["path_option"].is_array()) {
                    for (auto &pathOption: multiModuleOption[moduleOptKey]["package_requires"]["path_option"]) {
                        ChangeMessageUrlOfTextDocument(projectPath, pathOption, rootUri, isMultiModule);
                    }
                }
            }
            if (multiModuleOption[moduleOptKey].contains("src_path")) {
                ChangeMessageUrlOfTextDocument(projectPath, multiModuleOption[moduleOptKey]["src_path"], rootUri,
                                               isMultiModule);
            }
            auto key = ChangeMessageUrlOfString(projectPath, moduleOptKey, rootUri, isMultiModule);
            multiModuleOptionTemp[key] = multiModuleOption[moduleOptKey];
        }
        root["initializationOptions"]["multiModuleOption"] = multiModuleOptionTemp;
    }

    std::string ChangeMessageUrlForTestFile(const std::string &message, const std::string &proFolder,
                                            std::string &rootUri,
                                            bool &isMultiModule) {
        std::string projectPath = SingleInstance::GetInstance()->workPath + "/test/testChr/" + proFolder;

        nlohmann::json root;
        std::string errs;
        try {
            if (message.empty()) {
                return "";
            }
            root = nlohmann::json::parse(message);
        } catch (nlohmann::detail::parse_error &errs) {
            return "";;
        }
        if (root.empty() || root.contains("caseFolder")) {
            return "";
        }
        if (!root.contains("method")) {
            std::ostringstream os;
            os << root.dump(-1);
            return os.str();
        }

        if (root.value("method", "") == "initialize") {
            rootUri = root["params"].value("rootUri", "");
            root["params"]["rootPath"] = projectPath;
            root["params"]["initializationOptions"]["targetLib"] = SingleInstance::GetInstance()->binaryPath;
            auto projectUri = projectPath;
            auto found = projectPath.find(':');
            if (found != std::string::npos) {
                projectUri.replace(found, 1, "%3A");
            }
            if (TEST_FILE_SEPERATOR == "\\") {
                root["params"]["rootUri"] = "file:///" + projectUri;
            } else {
                root["params"]["rootUri"] = "file://" + projectPath; // GCOVR_EXCL_LINE
            }
            ChangeMultiModuleOptionUrl(projectPath, root["params"], rootUri, isMultiModule);
            if (root["params"].contains("initializationOptions") && root["params"]["initializationOptions"].contains(
                    "conditionCompilePaths")) {
                root["params"]["initializationOptions"]["conditionCompilePaths"][0] = projectPath;
            }
            if (root["params"].contains("initializationOptions") && root["params"]["initializationOptions"].contains(
                    "stdCjdPathOption")) {
                root["params"]["initializationOptions"]["stdCjdPathOption"] = JoinPath(
                    JoinPath(SingleInstance::GetInstance()->pathPwd, "stdlib"), "cjdecl");
            }
            if (root["params"].contains("initializationOptions") && root["params"]["initializationOptions"].contains(
                    "ohosCjdPathOption")) {
                root["params"]["initializationOptions"]["ohosCjdPathOption"] = JoinPath(
                    JoinPath(SingleInstance::GetInstance()->pathPwd, "ohoslib"), "cjdecl");
            }
            if (root["params"].contains("initializationOptions") && root["params"]["initializationOptions"].contains(
                    "cjdCachePathOption")) {
                root["params"]["initializationOptions"]["cjdCachePathOption"] = JoinPath(
                    SingleInstance::GetInstance()->pathPwd, "cjdIdx");
            }
        } else {
            if (root["params"].contains("changes")) {
                ChangeMessageUrlOfChanges(projectPath, root, rootUri, isMultiModule); // GCOVR_EXCL_LINE
            }

            if (root["params"].contains("textDocument")) {
                ChangeMessageUrlOfTextDocument(projectPath, root["params"]["textDocument"]["uri"], rootUri,
                                               isMultiModule);
            }

            if (root["params"].contains("item")) {
                ChangeMessageUrlOfTextDocument(projectPath, root["params"]["item"]["uri"], rootUri, isMultiModule);
            }
        }
        std::ostringstream os;
        os << root.dump(-1);
        return os.str();
    }


    std::pair<std::string, std::string> GetCaseAndBinaryFolder(const std::string message) {
        std::pair<std::string, std::string> pair;
        std::string caseFolder;

        nlohmann::json root;
        std::string errs;

        try {
            if (message.empty()) {
                return pair;
            }
            root = nlohmann::json::parse(message);
        } catch (nlohmann::detail::parse_error &errs) {
            return pair; // GCOVR_EXCL_LINE
        }

        if (!root.contains("caseFolder")) {
            return pair; // GCOVR_EXCL_LINE
        }

        caseFolder = root.value("caseFolder", "");
        size_t index = caseFolder.find("//");
        if (index != std::string::npos) {
            caseFolder.replace(index, std::string("//").size(), "/");
        }
        pair.first = caseFolder;
        if (root.contains("macroBinPath")) {
            pair.second = root.value("macroBinPath", "");
        }
        return pair;
    }

    std::string GetFileUrl(const std::string &file) {
        std::string filePath;
        auto loc = file.find("test/testChr");
        if (loc == std::string::npos) {
            if (file.find("output/bin/stdlib/cjdecl/") != std::string::npos) {
                return file;
            }
            return filePath;
        }

        std::string subUri = "/" + file.substr(loc, file.length());
        filePath = SingleInstance::GetInstance()->workPath + subUri;
        return filePath;
    }

    struct SemanticTokensFormat {
        int line;
        int startChar;
        int length;
        int tokenType;
        int tokenTypeModifier;

        bool operator==(const SemanticTokensFormat &right) const;

        bool operator<(const SemanticTokensFormat &right) const;
    };

    // GCOVR_EXCL_START
    bool SemanticTokensFormat::operator==(const SemanticTokensFormat &right) const // GCOVR_EXCL_LINE
    {
        return this->line == right.line && this->startChar == right.startChar &&
               this->length == right.length && this->tokenType == right.tokenType;
    }

    bool SemanticTokensFormat::operator<(const SemanticTokensFormat &right) const // GCOVR_EXCL_LINE
    {
        if (this->line < right.line) {
            return true;
        }
        if (this->line == right.line && (this->startChar < right.startChar ||
                                         this->startChar == right.startChar && this->length < right.length)) {
            return true; // GCOVR_EXCL_LINE
        }
        return false;
    }

    // GCOVR_EXCL_STOP

    void AdaptCjvmTest(string &baseFile, const SingleInstance *p, string &expectedFile) {
#ifdef CANGJIE_CODEGEN_CJVM_BACKEND
    string tempFile = p->messagePath + "/" + baseFile.substr(0, baseFile.rfind(".")) + "-cjvm.base";
    if (!FileExist(tempFile)) {
        expectedFile = p->messagePath + "/" + baseFile.substr(0, baseFile.rfind(".")) + ".base";
    } else {
        expectedFile = p->messagePath + "/" + baseFile.substr(0, baseFile.rfind(".")) + "-cjvm.base";
        baseFile = baseFile.substr(0, baseFile.rfind(".")) + "-cjvm.base";
    }
#else
        expectedFile = p->messagePath + "/" + baseFile.substr(0, baseFile.rfind(".")) + ".base";
#endif
    }

    std::vector<std::string> SplitString(const std::string &value, char delimiter) {
        std::vector<std::string> subValue;
        std::string singleLetter = "";
        int index = 0;
        while (value[index] != '\0') {
            if (value[index] != delimiter) {
                singleLetter += value[index];
            } else if (value[index] == delimiter && value[index + 1] == delimiter) {
                index++;
            } else {
                subValue.push_back(singleLetter);
                singleLetter = "";
            }
            index++;
        }
        subValue.push_back(singleLetter);
        return std::move(subValue);
    }

    // convert to row and column numbers
    // input  1,7,4,16,0,
    //       0,0,4,16,0,
    //       0,5,1,16,0
    // output [2,7,4,16,0],[2,7,4,16,0],[2,12,1,16,0]
    std::vector<SemanticTokensFormat> SemanticTokensAdaptor(const std::string &value) {
        std::vector<SemanticTokensFormat> semaTokensRawVec;
        auto vecStr = SplitString(value, ',');
        auto length = vecStr.size();
        decltype(length) groupLength = 5;
        if (length % groupLength != 0) {
            return semaTokensRawVec;
        }
        for (decltype(length) i = 0; i < length; i += groupLength) {
            decltype(length) indexOfGroup = 0;
            SemanticTokensFormat semanticTokensFormat = {
                std::stoi(vecStr[i]),
                std::stoi(vecStr[i + 1]),
                std::stoi(vecStr[i + 2]),
                std::stoi(vecStr[i + 3]),
                std::stoi(vecStr[i + 4])
            };
            auto count = semaTokensRawVec.size();
            if (i == 0) {
                // The starting row adds one
                semanticTokensFormat.line++;
            } else {
                if (semanticTokensFormat.line == 0) {
                    semanticTokensFormat.startChar += semaTokensRawVec[count - 1].startChar;
                }
                semanticTokensFormat.line += semaTokensRawVec[count - 1].line;
            }
            semaTokensRawVec.push_back(semanticTokensFormat);
        }
        return semaTokensRawVec;
    }

    std::string GetStringSemanticResult(const std::vector<SemanticTokensFormat> &semaTokensRawVec) {
        std::string result = "";
        auto semaTokensCount = semaTokensRawVec.size();
        for (int j = 0; j < semaTokensCount; j++) {
            auto semanticTokensFormat = semaTokensRawVec[j];
            std::string tmp = "[" + std::to_string(semanticTokensFormat.line) + ",";
            result.append(tmp);
            tmp = std::to_string(semanticTokensFormat.startChar) + ",";
            result.append(tmp);
            tmp = std::to_string(semanticTokensFormat.length) + ",";
            result.append(tmp);
            tmp = std::to_string(semanticTokensFormat.tokenType) + ",";
            result.append(tmp);
            tmp = std::to_string(semanticTokensFormat.tokenTypeModifier) + "]";
            if (j < semaTokensCount - 1) {
                tmp.append(",");
            }
            result.append(tmp);
        }
        return result;
    }

    std::pair<std::string, std::string> CompareSemanticTokens(
        const std::vector<SemanticTokensFormat> &expect,
        const std::vector<SemanticTokensFormat> &actual) {
        std::vector<SemanticTokensFormat> expectVector;
        std::vector<SemanticTokensFormat> actualVector;
        auto expectIter = expect.begin();
        auto actualIter = actual.begin();
        while (expectIter != expect.end() && actualIter != actual.end()) {
            if (*expectIter == *actualIter) {
                ++expectIter;
                ++actualIter;
                continue;
            }
            if (*expectIter < *actualIter) {
                expectVector.push_back(*expectIter);
                ++expectIter;
            } else {
                actualVector.push_back(*actualIter);
                ++actualIter;
            }
        }
        if (expectIter != expect.end()) {
            while (expectIter != expect.end()) {
                expectVector.push_back(*expectIter);
                ++expectIter;
            }
        }
        if (actualIter != actual.end()) {
            while (actualIter != actual.end()) {
                actualVector.push_back(*actualIter);
                ++actualIter;
            }
        }
        std::string expectString = GetStringSemanticResult(expectVector);
        std::string actualString = GetStringSemanticResult(actualVector);
        std::pair<std::string, std::string> result = {expectString, actualString};
        return result;
    }

    void PrintSematicResult(const std::vector<SemanticTokensFormat> &expect,
                            const std::vector<SemanticTokensFormat> &actual) {
        auto expectString = GetStringSemanticResult(expect);
        auto actualString = GetStringSemanticResult(actual);
        auto compareResult = CompareSemanticTokens(expect, actual);
        printf("inBase= %s\nresult= %s\n", expectString.c_str(),
               actualString.c_str());
        printf("different result \ninBase= %s\nresult= %s\n",
               compareResult.first.c_str(),
               compareResult.second.c_str());
    }

    // GCOVR_EXCL_START
    ark::Range CreateRangeOrSelectionRangeStruct(const nlohmann::json &exp) {
        ark::Range result;
        if (exp.contains("range")) {
            result.start.column = exp["range"]["start"].value("character", -1);
            result.start.line = exp["range"]["start"].value("line", -1);
            result.end.column = exp["range"]["end"].value("character", -1);
            result.end.line = exp["range"]["end"].value("line", -1);
        } else if (exp.contains("selectionRange")) {
            result.start.column = exp["selectionRange"]["start"].value("character", -1);
            result.start.line = exp["selectionRange"]["start"].value("line", -1);
            result.end.column = exp["selectionRange"]["end"].value("character", -1);
            result.end.line = exp["selectionRange"]["end"].value("line", -1);
        }
        return result;
    }

    // GCOVR_EXCL_STOP

    ark::Range CreateSelectionRangeStruct(const nlohmann::json &exp) {
        ark::Range result;
        if (exp.contains("selectionRange")) {
            result.start.column = exp["selectionRange"]["start"].value("character", -1);
            result.start.line = exp["selectionRange"]["start"].value("line", -1);
            result.end.column = exp["selectionRange"]["end"].value("character", -1);
            result.end.line = exp["selectionRange"]["end"].value("line", -1);
        }
        return result;
    }

    ark::Range CreateRangeStruct(const nlohmann::json &exp) {
        ark::Range result;
        if (exp.contains("range")) {
            result.start.column = exp["range"]["start"].value("character", -1);
            result.start.line = exp["range"]["start"].value("line", -1);
            result.end.column = exp["range"]["end"].value("character", -1);
            result.end.line = exp["range"]["end"].value("line", -1);
        }
        return result;
    }

    template<typename T>
    bool CompareRange(const T &letf, const T &right) {
        return letf.range < right.range;
    }

    template<typename T>
    bool CompareTypeHierarchyItem(const T &letf, const T &right) {
        if (letf.name != right.name) {
            return letf.name < right.name;
        } else {
            return letf.range < right.range;
        }
    }

    template<typename T>
    bool CompareCallHierarchyIncomingCall(const T &letf, const T &right) {
        return letf.from.range < right.from.range;
    }

    template<typename T>
    bool CompareCallHierarchyOutgoingCall(const T &letf, const T &right) {
        return letf.to.range < right.to.range;
    }

    template<typename T>
    bool CompareLocation(const T &letf, const T &right) {
        return letf.location < right.location;
    }

    bool CompTypeHierarchyVector(const TypeHierarchyInfo &a, const TypeHierarchyInfo &b) {
        return (a.name < b.name);
    }

    void replaceFileContent() {
    }

    std::unique_ptr<HoverInfo> CreateHoverStruct(const nlohmann::json &exp) {
        auto result = std::make_unique<HoverInfo>();
        nlohmann::json tmp = exp["result"];
        if (!tmp.contains("contents")) {
            return nullptr;
        }

        std::stringstream stream(tmp["contents"].value("value", ""));
        std::string word;
        while (stream >> word) {
            result->message += word;
        }

        result->range = CreateRangeOrSelectionRangeStruct(tmp);
        return result;
    }

    std::vector<DiagnosticInfo> CreateDiagnosticStruct(const nlohmann::json &exp) {
        std::vector<DiagnosticInfo> result;
        DiagnosticInfo item;
        if (!exp.contains("params")) {
            return result;
        }

        if (!exp["params"].empty()) {
            for (int i = 0; i < exp["params"]["diagnostics"].size(); i++) {
                item.diagInfo.range = CreateRangeOrSelectionRangeStruct(exp["params"]["diagnostics"][i]);
                item.diagInfo.code = exp["params"]["diagnostics"][i].value("code", -1);
                item.diagInfo.severity = exp["params"]["diagnostics"][i].value("severity", -1);
                item.diagInfo.source = exp["params"]["diagnostics"][i].value("source", "");
                item.diagInfo.message = exp["params"]["diagnostics"][i].value("message", "");
                item.diagInfo.category = exp["params"]["diagnostics"][i].value("category", -1);
                result.push_back(item);
            }
        }
        return result;
    }

    bool CompWorkspaceSymbol(const ark::SymbolInformation &left, const ark::SymbolInformation &right) {
        return (left.name < right.name);
    }

    ark::Location CreateLocation(const nlohmann::json &locationJson) {
        std::vector<ark::Location> result;
        ark::Location location;
        if (locationJson.contains("range")) {
            location.range = CreateRangeOrSelectionRangeStruct(locationJson);
        }
        if (locationJson.contains("uri")) {
            std::string uri = GetFileUrl(locationJson.value("uri", ""));
            test::common::LowFileName(uri);
            location.uri.file = ark::URI::URIFromAbsolutePath(uri).ToString();
        }
        return std::move(location);
    }

    std::vector<ark::DocumentSymbol> CreateDocumentSymbolStruct(const nlohmann::json &data) {
        std::vector<ark::DocumentSymbol> result;
        ark::DocumentSymbol documentSymbol;
        for (auto &item: data) {
            documentSymbol.name = item.value("name", "");
            documentSymbol.kind = static_cast<ark::SymbolKind>(item.value("kind", -1));
            documentSymbol.detail = item.value("detail", "");
            documentSymbol.range = CreateRangeOrSelectionRangeStruct(item);
            documentSymbol.selectionRange = CreateSelectionRangeStruct(item);
            if (item.contains("children")) {
                documentSymbol.children = CreateDocumentSymbolStruct(item["children"]);
            }
            result.emplace_back(documentSymbol);
        }
        return std::move(result);
    }

    ark::Signatures CreateSignaturesStruct(const nlohmann::json &exp) {
        ark::Signatures result;
        result.label = exp.value("label", "");
        result.parameters = {};
        if (exp.contains("parameters")) {
            for (int i = 0; i < exp["parameters"].size(); i++) {
                result.parameters.push_back(exp["parameters"][i].value("label", ""));
            }
        }
        return result;
    }

    ark::SignatureHelp CreateSignatureHelpStruct(const nlohmann::json &exp) {
        ark::SignatureHelp result;
        if (exp.contains("activeParameter")) {
            result.activeParameter = exp.value("activeParameter", -1);
        }
        if (exp.contains("activeSignature")) {
            result.activeSignature = exp.value("activeSignature", -1);
        }
        result.signatures = {};
        if (exp.contains("signatures")) {
            for (int i = 0; i < exp["signatures"].size(); ++i) {
                result.signatures.push_back(CreateSignaturesStruct(exp["signatures"][i]));
            }
        }
        return result;
    }


    std::set<ark::ExecutableRange> CreateExecutableRangeStruct(const nlohmann::json &exp) {
        std::set<ark::ExecutableRange> result;
        ark::ExecutableRange executableRange;
        executableRange.uri = GetFileUrl(exp.value("uri", ""));
        executableRange.projectName = exp.value("projectName", "");
        executableRange.packageName = exp.value("packageName", "");
        executableRange.className = exp.value("className", "");
        executableRange.functionName = exp.value("functionName", "");
        executableRange.range = CreateRangeStruct(exp);
        result.insert(executableRange);
        return result;
    }

    ark::Command CreateCommandStruct(const nlohmann::json &exp) {
        ark::Command result;
        if (exp.contains("command")) {
            result.command = exp.value("command", "");
            result.title = exp.value("title", "");
            result.arguments = CreateExecutableRangeStruct(exp["arguments"][0]);
        }
        return result;
    }
}


namespace test::common {
    void SetUpConfig(const std::string &featureName) {
        SingleInstance *p = SingleInstance::GetInstance();
        p->testFolder = featureName;
        p->pathIn = GetRealPath(p->testFolder + "_freopen.in");
        p->pathOut = GetRealPath(p->testFolder + "_freopen.out");
        p->pathPwd = GetPwd();
        p->workPath = GetRootPath(p->pathPwd);
        p->messagePath = p->workPath + "/test/message/" + p->testFolder;
#if _WIN32
        p->pathBuildScript = GetRealPath(p->testFolder + "_build.bat");
#else
    p->pathBuildScript = GetRealPath(p->testFolder + "_build.sh");
#endif
    }

    void StartLspServer() {
        SingleInstance *p = SingleInstance::GetInstance();
#ifdef _WIN32
        std::string cmdLine = "cmd.exe /c " + p->pathPwd + "\\LSPServer.exe --test < " + p->testFolder +
                              "_freopen.in > " +
                              p->testFolder + "_freopen.out";

        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        // Start the child process.
        if (!CreateProcess(NULL, (TCHAR *) cmdLine.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            // GCOVR_EXCL_START
            printf(cmdLine.c_str());
            printf("StartLspServer fisrt failed (%d).\n", GetLastError());
            if (!CreateProcess(NULL, (TCHAR *) cmdLine.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                printf(cmdLine.c_str());
                printf("\n");
                printf("StartLspServer failed (%d).\n", GetLastError());
                return;
            }
            // GCOVR_EXCL_STOP
        }

        // Wait until child process exits.
        WaitForSingleObject(pi.hProcess, INFINITE);

        // Close process and thread handles.
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
#else
    std::string cmd =
        p->pathPwd + "/LSPServer --test < " + p->testFolder + "_freopen.in > " + p->testFolder + "_freopen.out";

    int status = system(cmd.c_str());
    if (-1 == status) {
        perror("Failed to wait for the subprocess to end.");
    }
    return;
#endif
    }

    void ShowDiff(const nlohmann::json &inBase, const nlohmann::json &result, const TestParam &param,
                  const std::string &baseUrl) {
        std::string testFile = baseUrl + "/" + param.testFile;
        std::string baseFile = baseUrl + "/" + param.baseFile;
        PrintJson(inBase, "inBase");
        PrintJson(result, "result");
        std::printf("curTestFile:\nfile:///%s\ncurBaseFile:\nfile:///%s\n", testFile.c_str(), baseFile.c_str());
    }

    std::string GetPwd() {
        std::string result{};
        char *buffer = getcwd(nullptr, 0);
        if (buffer == nullptr) {
            return result;
        }
        result = buffer;
        return result;
    }

    std::string GetRealPath(const std::string &fileName) {
        std::string realPath = GetPwd() + "/" + fileName;
        return realPath;
    }

    nlohmann::json ReadFileById(const std::string &file, const std::string &id) {
        std::string message;
        std::ifstream infile;
        nlohmann::json root;
        std::string errs;
        std::regex quotePattern("\"");

        infile.open(file);
        if (!infile.is_open()) {
            return {}; // GCOVR_EXCL_LINE
        }
        while (infile.good() && !infile.eof()) {
            char buf[MAX_LEN] = {0};
            infile.getline(buf, MAX_LEN);
            message = std::string(buf);
            message = replaceCrLf(message);
            auto loc = message.find("Content-Length");
            if (loc != std::string::npos) {
                std::string tmp = message.substr(0, loc);
                try {
                    if (tmp.empty()) {
                        continue;
                    }
                    root = nlohmann::json::parse(tmp);
                    if ((root.contains("id") && std::regex_replace(root["id"].dump(), quotePattern, "") == id) ||
                        (!root.contains("id") && id.empty())) {
                        return root;
                    }
                } catch (nlohmann::detail::parse_error &errs) {
                    continue;
                }
            } else {
                try {
                    if (message.empty()) {
                        continue;
                    }
                    root = nlohmann::json::parse(message);
                    if ((root.contains("id") && std::regex_replace(root["id"].dump(), quotePattern, "") == id) ||
                        (!root.contains("id") && id.empty())) {
                        return root;
                    }
                } catch (nlohmann::detail::parse_error &errs) {
                    continue;
                }
            }
        }
        infile.close(); // GCOVR_EXCL_LINE
        return {}; // GCOVR_EXCL_LINE
    }

    std::string GetRootPath(const std::string &workPath) {
        std::string result;
        auto loc = workPath.rfind("output");
        if (loc > 0) {
            std::string tmp = workPath.substr(0, loc);
            if (TEST_FILE_SEPERATOR == "\\") {
                std::istringstream iss(tmp);
                std::string token;
                std::vector<std::string> tokens;
                while (std::getline(iss, token, *TEST_FILE_SEPERATOR.c_str())) {
                    if (!token.empty()) {
                        tokens.push_back(token);
                    }
                }
                result = tokens.empty() ? "" : tokens[0];
                transform(result.begin(), result.end(), result.begin(), ::tolower);
                for (size_t i = 1; i < tokens.size(); ++i) {
                    result += "/" + tokens[i];
                }
            } else {
                result = workPath.substr(0, loc - 1);
            }
        }

        return result;
    }

    void ChangeMessageUrlForBaseFile(const std::string &testFilePath, nlohmann::json &resultBase, std::string &rootUri,
                                     bool &isMultiModule) {
        std::ifstream infile;
        std::string message;
        std::string caseFolder;
        infile.open(testFilePath);
        if (infile.is_open() && infile.good() && !infile.eof()) {
            char buf[MAX_LEN] = {0};
            infile.getline(buf, MAX_LEN);
            message = std::string(buf);
            if (message.length() == 0) {
                return; // GCOVR_EXCL_LINE
            }
            std::pair<std::string, std::string> caseAndBinaryFolder = GetCaseAndBinaryFolder(message);
            caseFolder = caseAndBinaryFolder.first;
            infile.close();
        }
        std::string projectPath = SingleInstance::GetInstance()->workPath + "/test/testChr/" + caseFolder;
        if (resultBase.empty()) {
            return;
        }
        if (resultBase.contains("result")) {
            if (resultBase["result"].is_null()) {
                return;
            }
            if (resultBase["result"].is_array()) {
                for (auto &member: resultBase["result"]) {
                    if (member.contains("uri")) {
                        ChangeMessageUrlOfTextDocument(projectPath, member["uri"], rootUri, isMultiModule);
                    }
                    if (member.contains("location") && member["location"].contains("uri")) {
                        ChangeMessageUrlOfTextDocument(projectPath, member["location"]["uri"], rootUri, isMultiModule);
                    }
                }
            } else if (resultBase["result"].contains("documentChanges")) {
                for (auto &member: resultBase["result"]["documentChanges"]) {
                    ChangeMessageUrlOfTextDocument(projectPath, member["textDocument"]["uri"], rootUri, isMultiModule);
                }
            } else {
                ChangeMessageUrlOfTextDocument(projectPath, resultBase["result"]["uri"], rootUri, isMultiModule);
            }
        } else if (resultBase.contains("method") && resultBase["method"] == "textDocument/extendPublishDiagnostics") {
            ChangeMessageUrlOfTextDocument(projectPath, resultBase["params"]["uri"], rootUri, isMultiModule);
        }
    }

    void HandleCjdExpLines(nlohmann::json &expLines) {
        if (expLines.contains("result") && expLines["result"].contains("uri") && !expLines["result"]["uri"].empty()) {
            auto rawUri = expLines["result"].value("uri", "");
            auto loc = rawUri.find("cangjie/compiler/lib/cjdecl");
            if (loc != std::string::npos) {
                auto subUri = rawUri.substr(loc + strlen("cangjie/compiler/lib"), rawUri.length());
                auto newUri = SingleInstance::GetInstance()->pathPwd + +"/stdlib" + subUri;
                newUri = ark::PathWindowsToLinux(newUri);
                if (newUri.find(':') < newUri.find('/')) {
                    newUri[0] = tolower(newUri[0]);
                }
                expLines["result"]["uri"] = newUri;
            }
        }
    }

    // GCOVR_EXCL_START
    bool CheckResult(nlohmann::json exp, nlohmann::json &result, TestType testType, std::string &reason) {
        std::string jsonStr;
        std::ostringstream expOs;
        std::ostringstream actOs;

        if (exp.contains("result")) {
            nlohmann::json value = exp["result"];
            if (result.contains("result") && value.is_null() && result["result"].is_null()) {
                return true;
            }
            if (!value.is_array() && value.contains("uri")) {
                std::string uri = GetFileUrl(value.value("uri", ""));
                LowFileName(uri);
                exp["result"]["uri"] = ark::URI::URIFromAbsolutePath(uri).ToString();
            }
            if (testType == TestType::Completion && result.contains("result") && result["result"].is_array()) {
                for (auto &item: result["result"]) {
                    if (item.contains("sortText")) {
                        item["sortText"] = "";
                    }
                }
            }

            actOs << result.dump(-1);
            std::string lines = actOs.str();
            expOs << exp.dump(-1);
            std::string expLines = expOs.str();
            if (lines != expLines) {
                if (testType == TestType::SemanticHighlight) {
                    auto getData = [](const nlohmann::json &jsonLines) {
                        std::string dataLines;
                        if (jsonLines.contains("result") && jsonLines["result"].contains("data")) {
                            nlohmann::json data = jsonLines["result"]["data"];
                            std::ostringstream expOs;
                            expOs << data.dump(-1);
                            dataLines = expOs.str();
                            auto length = dataLines.length();
                            // {"id":"2","jsonrpc":"2.0","result":{"data":[1,7,4,16,0]
                            // remove"[" and “]” in data
                            // dataLines = "1,7,4,16,0"
                            if (length >= 2) {
                                dataLines.erase(length - 1);
                                dataLines.erase(0, 1);
                            }
                        }
                        return dataLines;
                    };
                    auto expect = SemanticTokensAdaptor(getData(exp));
                    auto actual = SemanticTokensAdaptor(getData(result));
                    PrintSematicResult(expect, actual);
                }
                reason = "the number of lines is different between expected and actual";
                return false;
            }
            return true;
        }
        reason = " nlohmann::json exp hasn't member 'result' ";
        return false;
    }

    // GCOVR_EXCL_STOP

    bool CreateBuildScript(const std::string &execScriptPath, const std::string &testFile) {
        std::string caseScriptPath = testFile.substr(0, testFile.rfind('.')) + ".build";
        std::string caseProPath = SingleInstance::GetInstance()->caseProPath;
        std::ofstream out(execScriptPath);
        std::string pattern = R"(\$\{([^}]+)\})";
        std::regex regex(pattern);
        std::smatch match;

        if (out.is_open()) {
            std::ifstream infile;
            std::string message;
            std::string caseFolder;
            infile.open(caseScriptPath);
            if (infile.is_open()) {
                while (infile.good() && !infile.eof()) {
                    char buf[MAX_LEN] = {0};
                    infile.getline(buf, MAX_LEN);
                    message = std::string(buf);
                    std::string result;
                    if (message.length() == 0) {
                        continue;
                    }
                    auto begin = message.cbegin();
                    auto end = message.cend();
                    while (std::regex_search(begin, end, match, regex)) {
                        // append the part before the match to the result
                        result.append(match.prefix().str());

                        if (match.size() > 1) {
                            std::string key = match[1].str();
                            if (SingleInstance::GetInstance()->replaceMap.find(key) != SingleInstance::GetInstance()->
                                replaceMap.end()) {
                                result.append(SingleInstance::GetInstance()->replaceMap[key]);
                            } else {
                                result.append(match[0].str());
                            }
                        }

                        begin = match.suffix().first;
                    }
                    // append remaining unmatched parts
                    result.append(begin, end);
                    out << result << std::endl;
                }
                infile.close();
            } else {
                out.close(); // GCOVR_EXCL_LINE
                return false;
            }
        } else {
            return false;
        }
        return true;
    }

    void BuildDynamicBinary(const std::string &execScriptPath) {
        std::string cmd;
#ifdef _WIN32
        // Windows 平台
        // 如果你要执行 sh 脚本，需要确保系统有 Git Bash 或 Cygwin 等
        cmd = execScriptPath;
#else
    // Linux/Unix 平台
    cmd = "/bin/sh " + execScriptPath;
#endif
        system(cmd.c_str());
    }

    bool CreateMsg(const std::string &path, const std::string &testFile, std::string &rootUri, bool &isMultiModule,
                   const std::string &symbolId) {
        std::ofstream out(path);
        if (out.is_open()) {
            std::ifstream infile;
            std::string message;
            std::string caseFolder;
            infile.open(testFile);
            if (infile.is_open()) {
                while (infile.good() && !infile.eof()) {
                    char buf[MAX_LEN] = {0};
                    infile.getline(buf, MAX_LEN);
                    message = std::string(buf);
                    if (message.length() == 0) {
                        continue;
                    }
                    if (caseFolder.empty()) {
                        std::pair<std::string, std::string> caseAndBinaryFolder = GetCaseAndBinaryFolder(message);
                        caseFolder = caseAndBinaryFolder.first;
                        SingleInstance::GetInstance()->caseProPath =
                                SingleInstance::GetInstance()->workPath + "/test/testChr/" + caseFolder;
                        SingleInstance::GetInstance()->replaceMap.insert_or_assign(
                            "caseProPath", SingleInstance::GetInstance()->caseProPath);
                        if (!caseAndBinaryFolder.second.empty()) {
                            SingleInstance::GetInstance()->binaryPath =
                                    SingleInstance::GetInstance()->caseProPath + "/" + caseAndBinaryFolder.second;
                            SingleInstance::GetInstance()->replaceMap.insert_or_assign(
                                "binaryPath", SingleInstance::GetInstance()->binaryPath);
                        }
                    }
                    if (caseFolder.empty()) {
                        break;
                    }
                    std::string tmp = ChangeMessageUrlForTestFile(message, caseFolder, rootUri, isMultiModule);
                    if (!symbolId.empty()) {
                        std::string str = "\"symbolId\":\"0\"";
                        std::string replaceStr = "\"symbolId\":\"" + symbolId + "\"";
                        size_t pos = tmp.find(str);
                        if (pos != std::string::npos) {
                            tmp.replace(pos, str.length(), replaceStr);
                        }
                    }
                    auto length = tmp.length();

                    out << "Content-Length:" << length << std::endl;
                    out << std::endl;
                    out << tmp << std::endl;
                }
                infile.close();
            } else {
                out.close(); // GCOVR_EXCL_LINE
                return false;
            }
        } else {
            return false;
        }
        out.close();
        return true;
    }

    nlohmann::json ReadExpectedResult(std::string &baseFile) {
        nlohmann::json result;
        SingleInstance *p = SingleInstance::GetInstance();
        std::string expectedFile = "";
        AdaptCjvmTest(baseFile, p, expectedFile);
        nlohmann::json root;
        std::string errs;
        std::ifstream infile;
        infile.open(expectedFile);
        if (!infile.is_open()) {
            return result;
        }

        while (infile.good() && !infile.eof()) {
            char buf[MAX_LEN] = {0};
            infile.getline(buf, MAX_LEN);
            std::string message = std::string(buf);

            try {
                if (message.empty()) {
                    continue;
                }
                root = nlohmann::json::parse(message);
            } catch (nlohmann::detail::parse_error &errs) {
                continue;
            }
            if (root.empty()) {
                continue;
            }
            result = root;
            break;
        }
        infile.close();

        return result;
    }

    std::vector<ark::DocumentHighlight> ReadExpectedVector(std::string &baseFile) {
        std::vector<ark::DocumentHighlight> result;
        SingleInstance *p = SingleInstance::GetInstance();
        std::string expectedFile = "";
        AdaptCjvmTest(baseFile, p, expectedFile);
        nlohmann::json root;
        std::string errs;
        std::ifstream infile;
        infile.open(expectedFile);

        if (!infile.is_open()) {
            return result;
        }

        while (infile.good() && !infile.eof()) {
            char buf[MAX_LEN] = {0};
            infile.getline(buf, MAX_LEN);
            std::string message = std::string(buf);
            try {
                if (message.empty()) {
                    continue;
                }
                root = nlohmann::json::parse(message);
            } catch (nlohmann::detail::parse_error &errs) {
                continue;
            }
            if (root.empty()) {
                continue;
            }
            result = CreateDocumentHighlightStruct(root);
            break;
        }

        infile.close();
        return result;
    }

    std::vector<ark::DocumentHighlight> CreateDocumentHighlightStruct(const nlohmann::json &exp) {
        std::vector<ark::DocumentHighlight> result;
        ark::DocumentHighlight Item;
        if (exp.contains("result")) {
            for (int i = 0; i < exp["result"].size(); i++) {
                int kind = exp["result"][i].value("kind", -1);
                Item.kind = ark::DocumentHighlightKind(kind);
                Item.range = CreateRangeOrSelectionRangeStruct(exp["result"][i]);
                result.push_back(Item);
            }
        }
        return result;
    }

    void CreatePrepareTypeHierarchyStruct(const nlohmann::json &exp, TypeHierarchyResult &actual) {
        if (exp.contains("result")) {
            for (int i = 0; i < exp["result"].size(); i++) {
                auto symbolId = exp["result"][i]["data"].value("symbolId", "");
                if (!symbolId.empty()) {
                    actual.symbolId = std::stoull(symbolId);
                }
            }
        }
    }

    std::vector<ark::TypeHierarchyItem> CreateTypeHierarchyStruct(const nlohmann::json &exp,
                                                                  TypeHierarchyResult &expect) {
        std::vector<ark::TypeHierarchyItem> result;
        ark::TypeHierarchyItem typeHierarchyItem;
        if (exp.contains("result")) {
            for (int i = 0; i < exp["result"].size(); i++) {
                int kind = exp["result"][i].value("kind", -1);
                typeHierarchyItem.kind = ark::SymbolKind(kind);
                typeHierarchyItem.name = exp["result"][i].value("name", "");
                typeHierarchyItem.range = CreateRangeStruct(exp["result"][i]);
                typeHierarchyItem.selectionRange = CreateSelectionRangeStruct(exp["result"][i]);
                typeHierarchyItem.uri.file = GetFileUrl(exp["result"][i].value("uri", ""));
                auto symbolId = exp["result"][i]["data"].value("symbolId", "");
                if (!symbolId.empty()) {
                    expect.symbolId = std::stoull(symbolId);
                }
                result.push_back(typeHierarchyItem);
            }
        }
        return std::move(result);
    }

    // GCOVR_EXCL_START
    bool CheckDocumentHighlight(std::vector<ark::DocumentHighlight> exp,
                                std::vector<ark::DocumentHighlight> act,
                                std::string &reason) {
        if (!CheckResultCount(exp, act, false)) {
            reason = "expect and actual DocumentHighlight number is different";
            return false;
        }

        std::sort(exp.begin(), exp.end(), CompareRange<ark::DocumentHighlight>);
        std::sort(act.begin(), act.end(), CompareRange<ark::DocumentHighlight>);
        for (int i = 0; i < exp.size(); i++) {
            if (exp[i].kind != act[i].kind) {
                reason = "the expect and actual DocumentHighlight " + std::to_string(i) + " kind is different";
                return false;
            }

            if ((exp[i].range.start.column != act[i].range.start.column) ||
                (exp[i].range.end.column != act[i].range.end.column)) {
                reason = "the expect and actual DocumentHighlight " + std::to_string(i) +
                         " range start or end column is different";
                return false;
            }

            if ((exp[i].range.start.line != act[i].range.start.line) ||
                (exp[i].range.end.line != act[i].range.end.line)) {
                reason = "the expect and actual DocumentHighlight " + std::to_string(i) +
                         " range start or end line is different";
                return false;
            }
        }

        return true;
    }

    // GCOVR_EXCL_STOP

    std::vector<ark::Location> CreateLocationStruct(const nlohmann::json &exp) {
        std::vector<ark::Location> result;
        ark::Location Item;
        if (exp.contains("result") && exp["result"].is_array()) {
            for (int i = 0; i < exp["result"].size(); i++) {
                Item.range = CreateRangeOrSelectionRangeStruct(exp["result"][i]);
                std::string uri = GetFileUrl(exp["result"][i].value("uri", ""));
                LowFileName(uri);
                Item.uri.file = ark::URI::URIFromAbsolutePath(uri).ToString();
                result.push_back(Item);
            }
        }
        return result;
    }

    std::vector<ark::Location> ReadLocationExpectedVector(const std::string &testFile, std::string &baseFile,
                                                          std::string &rootUri, bool &isMultiModule) {
        std::vector<ark::Location> result;
        nlohmann::json expLines = ReadExpectedResult(baseFile);
        ChangeMessageUrlForBaseFile(testFile, expLines, rootUri, isMultiModule);
        result = CreateLocationStruct(expLines);
        return result;
    }

    bool CheckLocationVector(std::vector<ark::Location> exp, std::vector<ark::Location> act, std::string &reason) {
        if (exp.empty()) {
            return true;
        }
        if (!CheckResultCount(exp, act)) {
            reason = "expect and actual Location number is different";
            return false;
        }

        std::sort(exp.begin(), exp.end());
        std::sort(act.begin(), act.end());

        for (int i = 0; i < exp.size(); i++) {
            // GCOVR_EXCL_START
            if ((exp[i].range.start.column != act[i].range.start.column) ||
                (exp[i].range.end.column != act[i].range.end.column)) {
                reason = "the expect and actual Location " + std::to_string(i) +
                         " range start or end column is different";
                return false;
            }

            if ((exp[i].range.start.line != act[i].range.start.line) ||
                (exp[i].range.end.line != act[i].range.end.line)) {
                reason = "the expect and actual Location " + std::to_string(i) +
                         " range start or end line is different";
                return false;
            }

            if (exp[i].uri.file != act[i].uri.file) {
                reason = "the expect and actual Location " + std::to_string(i) + " uri.file is different";
                return false;
            }
            // GCOVR_EXCL_STOP
        }

        return true;
    }

    // GCOVR_EXCL_START
    std::vector<TextDocumentEditInfo> CreateTextDocumentEditStruct(const nlohmann::json &exp) {
        std::vector<TextDocumentEditInfo> result;
        TextDocumentEditInfo Item;
        if (exp.contains("result")) {
            if (!exp["result"].contains("documentChanges")) {
                return result;
            }

            nlohmann::json tmp = exp["result"]["documentChanges"];
            for (auto &item: tmp) {
                for (int j = 0; j < item["edits"].size(); j++) {
                    Item.location.range = CreateRangeOrSelectionRangeStruct(item["edits"][j]);
                    std::string uri = GetFileUrl(item["textDocument"].value("uri", ""));
                    LowFileName(uri);
                    Item.location.uri.file = ark::URI::URIFromAbsolutePath(uri).ToString();
                    Item.message = item["edits"][0].value("newText", "");
                    result.push_back(Item);
                }
            }
        }

        return result;
    }

    std::vector<TextDocumentEditInfo> ReadTextDocumentEditVector(const std::string &testFile, std::string &baseFile,
                                                                 std::string &rootUri, bool &isMultiModule) {
        std::vector<TextDocumentEditInfo> result;
        nlohmann::json expLines = ReadExpectedResult(baseFile);
        ChangeMessageUrlForBaseFile(testFile, expLines, rootUri, isMultiModule);
        result = CreateTextDocumentEditStruct(expLines);
        return result;
    }

    bool CheckTextDocumentEditVector(std::vector<TextDocumentEditInfo> exp, std::vector<TextDocumentEditInfo> act,
                                     std::string &reason) {
        if (!CheckResultCount(exp, act, false)) {
            reason = "expect and actual TextDocumentEditInfo number is different";
            return false;
        }

        std::sort(exp.begin(), exp.end(), CompareLocation<TextDocumentEditInfo>);
        std::sort(act.begin(), act.end(), CompareLocation<TextDocumentEditInfo>);

        for (int i = 0; i < exp.size(); i++) {
            if ((exp[i].location.range.start.column != act[i].location.range.start.column) ||
                (exp[i].location.range.end.column != act[i].location.range.end.column)) {
                reason = "the expect and actual " + std::to_string(i) +
                         " location range start or end column is different";
                return false;
            }

            if ((exp[i].location.range.start.line != act[i].location.range.start.line) ||
                (exp[i].location.range.end.line != act[i].location.range.end.line)) {
                reason = "the expect and actual " + std::to_string(i) +
                         " location range start or end line is different";
                return false;
            }

            if (exp[i].location.uri.file != act[i].location.uri.file) {
                reason = "the expect and actual " + std::to_string(i) + " location uri file is different";
                return false;
            }

            if (exp[i].message != act[i].message) {
                reason = "the expect and actual " + std::to_string(i) + " message is different";
                return false;
            }
        }

        return true;
    }


    std::vector<TypeHierarchyInfo> CreateTypeHierarchyStruct(const nlohmann::json &exp) {
        std::vector<TypeHierarchyInfo> result;
        TypeHierarchyInfo item;

        if (!exp.contains("child")) {
            return result;
        }

        if (!exp["child"].empty()) {
            for (const auto &i: exp["child"]) {
                std::vector<TypeHierarchyInfo> tmp = CreateTypeHierarchyStruct(i);
                result.insert(result.end(), tmp.begin(), tmp.end());
            }
        }

        item.name = exp.value("name", "");
        item.kind = exp.value("kind", -1);
        result.push_back(item);
        return result;
    }

    // GCOVR_EXCL_STOP

    // GCOVR_EXCL_START
    bool CheckTypeHierarchyResult(TypeHierarchyResult &actualReulst, TypeHierarchyResult &expectReulst,
                                  std::string &reason) {
        auto actual = actualReulst.subOrSuperTypes;
        auto expect = actualReulst.subOrSuperTypes;
        if (!CheckResultCount(expect, actual, false)) {
            reason = "expect and actual TypeHierarchyItem number is different";
            return false;
        }
        std::sort(expect.begin(), expect.end(), CompareTypeHierarchyItem<ark::TypeHierarchyItem>);
        std::sort(actual.begin(), actual.end(), CompareTypeHierarchyItem<ark::TypeHierarchyItem>);
        for (int i = 0; i < expect.size(); i++) {
            if (expect[i].kind != actual[i].kind) {
                reason = "the expect and actual TypeHierarchyItem " + std::to_string(i) + " kind is different";
                return false;
            }

            if ((expect[i].range.start.column != actual[i].range.start.column) ||
                (expect[i].range.end.column != actual[i].range.end.column)) {
                reason = "the expect and actual TypeHierarchyItem " + std::to_string(i) +
                         " range start or end column is different";
                return false;
            }

            if ((expect[i].range.start.line != actual[i].range.start.line) ||
                (expect[i].range.end.line != actual[i].range.end.line)) {
                reason = "the expect and actual TypeHierarchyItem " + std::to_string(i) +
                         " range start or end line is different";
                return false;
            }

            if ((expect[i].selectionRange.start.column != actual[i].selectionRange.start.column) ||
                (expect[i].selectionRange.end.column != actual[i].selectionRange.end.column)) {
                reason = "the expect and actual TypeHierarchyItem " + std::to_string(i) +
                         " selectionRange start or end column is different";
                return false;
            }

            if ((expect[i].selectionRange.start.line != actual[i].selectionRange.start.line) ||
                (expect[i].selectionRange.end.line != actual[i].selectionRange.end.line)) {
                reason = "the expect and actual TypeHierarchyItem " + std::to_string(i) +
                         " selectionRange start or end line is different";
                return false;
            }
        }
        return true;
    }

    // GCOVR_EXCL_STOP

    std::vector<TestParam> GetTestCaseList(const std::string &key) {
        std::vector<TestParam> vecParam;
        TestParam item;
        std::string pathPwd = GetPwd();
        std::string workPath = GetRootPath(pathPwd);
        std::string testFilePath = workPath + "/test/testCase.list";
        nlohmann::json root;
        std::string errs;
        std::ifstream infile;
        infile.open(testFilePath);

        if (!infile.is_open()) {
            return vecParam;
        }
        SingleInstance *p = SingleInstance::GetInstance();
        while (infile.good() && !infile.eof()) {
            char buf[MAX_LEN] = {0};
            infile.getline(buf, MAX_LEN);
            std::string message = std::string(buf);
            try {
                if (message.empty()) {
                    continue;
                }
                root = nlohmann::json::parse(message);
            } catch (nlohmann::detail::parse_error &errs) {
                continue;
            }
            if (root.contains(key)) {
                item.testFile = root[key].value("testFile", "");
                item.baseFile = item.testFile.substr(0, item.testFile.rfind(".")) + ".base";
                item.preId = root[key].value("preId", "");
                item.id = root[key].value("id", "");
                vecParam.push_back(item);
            }
        }
        infile.close();
        return vecParam;
    }

    bool CheckHoverResult(const nlohmann::json &expect, const nlohmann::json &actual, std::string &reason) {
        if (!expect.contains("result") || !actual.contains("result")) {
            reason = "expect or actual nlohmann::json hasn't member 'result' ";
            return false;
        }

        std::unique_ptr<HoverInfo> exp = CreateHoverStruct(expect);
        std::unique_ptr<HoverInfo> act = CreateHoverStruct(actual);

        if (exp == nullptr && act == nullptr) {
            return true;
        }

        if (exp == nullptr || act == nullptr) {
            reason = " expect or actual HoverInfo is neither nullptr ";
            return false;
        }

        if (exp->message != act->message) {
            reason = " expect or actual HoverInfo message is different";
            return false;
        }

        if ((exp->range.start.column != act->range.start.column) ||
            (exp->range.end.column != act->range.end.column)) {
            reason = " expect and actual HoverInfo range start or end column is different";
            return false;
        }

        if ((exp->range.start.line != act->range.start.line) ||
            (exp->range.end.line != act->range.end.line)) {
            reason = " expect and actual HoverInfo range start or end line is different";
            return false;
        }

        return true;
    }

    // GCOVR_EXCL_START
    bool CheckDiagnosticResult(const nlohmann::json &expect, const nlohmann::json &actual, std::string &reason) {
        if (!expect.contains("params") || !actual.contains("params")) {
            reason = "expect or actual has no params";
            return false;
        }

        std::vector<DiagnosticInfo> exp = CreateDiagnosticStruct(expect);
        std::vector<DiagnosticInfo> act = CreateDiagnosticStruct(actual);

        if (!CheckResultCount(exp, act, false)) {
            reason = "expect and actual DiagnosticsInfo number is different";
            return false;
        }

        for (int i = 0; i < exp.size(); i++) {
            if (exp[i].uri != act[i].uri) {
                reason = "the expect and actual DiagnosticsInfo " + std::to_string(i) + " uri is different";
                return false;
            }
            if (exp[i].diagInfo.message != act[i].diagInfo.message) {
                reason = "the expect and actual DiagnosticsInfo " + std::to_string(i) +
                         " diagInfo.message is different";
                return false;
            }
            if (exp[i].diagInfo.source != act[i].diagInfo.source) {
                reason = "the expect and actual DiagnosticsInfo " + std::to_string(i) + " diagInfo.source is different";
                return false;
            }
            if (exp[i].diagInfo.severity != act[i].diagInfo.severity) {
                reason = "the expect and actual DiagnosticsInfo " + std::to_string(i) +
                         " diagInfo.severity is different";
                return false;
            }
            if (exp[i].diagInfo.range != act[i].diagInfo.range) {
                reason = "the expect and actual DiagnosticsInfo " + std::to_string(i) + " diagInfo.range is different";
                return false;
            }
        }
        return true;
    }

    // GCOVR_EXCL_STOP

    std::vector<ark::SymbolInformation> CreateWorkspaceSymbolStruct(const nlohmann::json &data) {
        std::vector<ark::SymbolInformation> result;
        if (data.contains("result")) {
            auto resultJson = data["result"];
            for (auto &item: resultJson) {
                ark::SymbolInformation symbolInformation;
                if (item.contains("location")) {
                    symbolInformation.location = CreateLocation(item["location"]);
                }
                symbolInformation.containerName = item.value("containerName", "");
                symbolInformation.name = item.value("name", "");
                symbolInformation.kind = static_cast<ark::SymbolKind>(item.value("kind", -1));
                result.emplace_back(symbolInformation);
            }
        }
        return std::move(result);
    }

    // GCOVR_EXCL_START
    bool CheckWorkspaceSymbolResult(const nlohmann::json &expect, const nlohmann::json &actual, std::string &reason) {
        if (!expect.contains("result") || !actual.contains("result")) {
            return false;
        }

        std::vector<ark::SymbolInformation> exp = CreateWorkspaceSymbolStruct(expect);
        std::vector<ark::SymbolInformation> act = CreateWorkspaceSymbolStruct(actual);

        if (!CheckResultCount(exp, act)) {
            reason = "expect and actual WorkspaceSymbol number is different";
            return false;
        }
        std::sort(exp.begin(), exp.end());
        std::sort(act.begin(), act.end());

        for (int i = 0; i < exp.size(); i++) {
            if (!(exp[i] == act[i])) {
                return false;
            }
        }
        return true;
    }

    // GCOVR_EXCL_STOP

    bool CheckDocumentSymbolResult(const nlohmann::json &expect, const nlohmann::json &actual, std::string &reason) {
        if (!expect.contains("result") || !actual.contains("result")) {
            reason = "expect or actual nlohmann::json hasn't member 'result' ";
            return false;
        }

        std::vector<ark::DocumentSymbol> exp = CreateDocumentSymbolStruct(expect["result"]);
        std::vector<ark::DocumentSymbol> act = CreateDocumentSymbolStruct(actual["result"]);
        if (!CheckResultCount(exp, act)) {
            reason = "expect and actual DiagnosticsInfo number is different";
            return false;
        }
        std::sort(exp.begin(), exp.end());
        std::sort(act.begin(), act.end());

        for (int i = 0; i < exp.size(); i++) {
            if (!(exp[i] == act[i])) {
                return false;
            }
        }
        return true;
    }

    std::vector<ark::TypeHierarchyItem> ReadExpectedTypeHierarchyResult(
        std::string &baseFile, TypeHierarchyResult &expect) {
        std::vector<ark::TypeHierarchyItem> result;
        SingleInstance *p = SingleInstance::GetInstance();
        std::string expectedFile = "";
        AdaptCjvmTest(baseFile, p, expectedFile);
        nlohmann::json root;
        std::string errs;
        std::ifstream infile;
        infile.open(expectedFile);

        if (!infile.is_open()) {
            return result;
        }

        while (infile.good() && !infile.eof()) {
            char buf[MAX_LEN] = {0};
            infile.getline(buf, MAX_LEN);
            std::string message = std::string(buf);
            try {
                if (message.empty()) {
                    continue;
                }
                root = nlohmann::json::parse(message);
            } catch (nlohmann::detail::parse_error &errs) {
                continue;
            }
            if (root.empty()) {
                continue;
            }
            result = CreateTypeHierarchyStruct(root, expect);
            break;
        }

        infile.close();
        return result;
    }


    bool CheckSignatureHelpResult(const nlohmann::json &expect, const nlohmann::json &actual, std::string &reason) {
        if (!expect.contains("result") || !actual.contains("result")) {
            reason = "expect or actual nlohmann::json hasn't member 'result' ";
            return false;
        }

        ark::SignatureHelp exp = CreateSignatureHelpStruct(expect["result"]);
        ark::SignatureHelp act = CreateSignatureHelpStruct(actual["result"]);

        std::sort(exp.signatures.begin(), exp.signatures.end());
        std::sort(act.signatures.begin(), act.signatures.end());
        if (exp != act) {
            return false;
        }

        return true;
    }

    std::vector<ark::BreakpointLocation> CreateBreakpointStruct(const nlohmann::json &exp) {
        std::vector<ark::BreakpointLocation> result;
        ark::BreakpointLocation breakpointLocationItem;
        if (exp.contains("result")) {
            if (!exp["result"].contains("breakpointLocation")) {
                return result;
            }
        }

        nlohmann::json tmp = exp["result"]["breakpointLocation"];
        for (auto &item: tmp) {
            breakpointLocationItem.uri = GetFileUrl(item.value("uri", ""));
            breakpointLocationItem.range = CreateRangeStruct(item);
            result.push_back(breakpointLocationItem);
        }
        return std::move(result);
    }

    std::vector<ark::BreakpointLocation> ReadExpectedBreakpointLocationItems(std::string &baseFile) {
        std::vector<ark::BreakpointLocation> result;
        SingleInstance *p = SingleInstance::GetInstance();
        std::string expectedFile = "";
        AdaptCjvmTest(baseFile, p, expectedFile);
        nlohmann::json root;
        std::string errs;
        std::ifstream infile;
        infile.open(expectedFile);

        if (!infile.is_open()) {
            return result;
        }

        while (infile.good() && !infile.eof()) {
            char buf[MAX_LEN] = {0};
            infile.getline(buf, MAX_LEN);
            std::string message = std::string(buf);
            try {
                if (message.empty()) {
                    continue;
                }
                root = nlohmann::json::parse(message);
            } catch (nlohmann::detail::parse_error &errs) {
                continue;
            }
            if (root.empty()) {
                continue;
            }
            result = CreateBreakpointStruct(root);
            break;
        }

        infile.close();
        return result;
    }

    // GCOVR_EXCL_START
    bool CheckBreakpointResult(std::vector<ark::BreakpointLocation> exp, std::vector<ark::BreakpointLocation> act,
                               std::string &reason) {
        if (!CheckResultCount(exp, act, false)) {
            reason = "expect and actual BreakpointLocation number is different";
            return false;
        }

        std::sort(exp.begin(), exp.end(), CompareRange<ark::BreakpointLocation>);
        std::sort(act.begin(), act.end(), CompareRange<ark::BreakpointLocation>);

        for (int i = 0; i < exp.size(); i++) {
            if ((exp[i].range.start.column != act[i].range.start.column) ||
                (exp[i].range.end.column != act[i].range.end.column)) {
                reason = "the expect and actual " + std::to_string(i) + " range start or end column is different";
                return false;
            }

            if ((exp[i].range.start.line != act[i].range.start.line) ||
                (exp[i].range.end.line != act[i].range.end.line)) {
                reason = "the expect and actual " + std::to_string(i) + " range start or end line is different";
                return false;
            }

            if (exp[i].uri != act[i].uri) {
                reason = "the expect and actual " + std::to_string(i) + " uri file is different";
                return false;
            }
        }

        return true;
    }

    // GCOVR_EXCL_STOP


    std::vector<ark::CodeLens> CreateCodeLensStruct(const nlohmann::json &exp) {
        std::vector<ark::CodeLens> result;
        ark::CodeLens codeLensItem;

        nlohmann::json tmp = exp["result"];
        for (auto &item: tmp) {
            codeLensItem.command = CreateCommandStruct(item["command"]);
            codeLensItem.range = CreateRangeStruct(item);
            result.push_back(codeLensItem);
        }
        return std::move(result);
    }

    std::vector<ark::CodeLens> ReadExpectedCodeLensItems(std::string &baseFile) {
        std::vector<ark::CodeLens> result;
        SingleInstance *p = SingleInstance::GetInstance();
        std::string expectedFile = "";
        AdaptCjvmTest(baseFile, p, expectedFile);
        nlohmann::json root;
        std::string errs;
        std::ifstream infile;
        infile.open(expectedFile);

        if (!infile.is_open()) {
            return result;
        }

        while (infile.good() && !infile.eof()) {
            char buf[MAX_LEN] = {0};
            infile.getline(buf, MAX_LEN);
            std::string message = std::string(buf);
            try {
                if (message.empty()) {
                    continue;
                }
                root = nlohmann::json::parse(message);
            } catch (nlohmann::detail::parse_error &errs) {
                continue;
            }
            if (root.empty()) {
                continue;
            }
            result = CreateCodeLensStruct(root);
            break;
        }

        infile.close();
        return result;
    }

    // GCOVR_EXCL_START
    bool CheckCodeLensResult(std::vector<ark::CodeLens> exp, std::vector<ark::CodeLens> act,
                             std::string &reason) {
        if (!CheckResultCount(exp, act, false)) {
            reason = "expect and actual BreakpointLocation number is different";
            return false;
        }

        std::sort(exp.begin(), exp.end(), CompareRange<ark::CodeLens>);
        std::sort(act.begin(), act.end(), CompareRange<ark::CodeLens>);

        for (int i = 0; i < exp.size(); i++) {
            if ((exp[i].range.start.column != act[i].range.start.column) ||
                (exp[i].range.end.column != act[i].range.end.column)) {
                reason = "the expect and actual " + std::to_string(i) + " range start or end column is different";
                return false;
            }

            if ((exp[i].range.start.line != act[i].range.start.line) ||
                (exp[i].range.end.line != act[i].range.end.line)) {
                reason = "the expect and actual " + std::to_string(i) + " range start or end line is different";
                return false;
            }

            if (exp[i].command.title != act[i].command.title) {
                reason = "the expect and actual " + std::to_string(i) + " command title is different";
                return false;
            }

            if (exp[i].command.command != act[i].command.command) {
                reason = "the expect and actual " + std::to_string(i) + " command command is different";
                return false;
            }

            if (exp[i].command.arguments.size() != act[i].command.arguments.size() ||
                exp[i].command.arguments.size() == 0 || act[i].command.arguments.size() == 0) {
                reason = "the expect and actual " + std::to_string(i) + " command arguments is different";
                return false;
            }

            if (exp[i].command.arguments.begin()->uri != act[i].command.arguments.begin()->uri) {
                reason = "the expect and actual " + std::to_string(i) + " arguments uri is different";
                return false;
            }

            if (exp[i].command.arguments.begin()->projectName != act[i].command.arguments.begin()->projectName) {
                reason = "the expect and actual " + std::to_string(i) + " arguments projectName is different";
                return false;
            }

            if (exp[i].command.arguments.begin()->packageName != act[i].command.arguments.begin()->packageName) {
                reason = "the expect and actual " + std::to_string(i) + " arguments packageName is different";
                return false;
            }

            if (exp[i].command.arguments.begin()->className != act[i].command.arguments.begin()->className) {
                reason = "the expect and actual " + std::to_string(i) + " arguments className is different";
                return false;
            }

            if (exp[i].command.arguments.begin()->functionName != act[i].command.arguments.begin()->functionName) {
                reason = "the expect and actual " + std::to_string(i) + " arguments functionName is different";
                return false;
            }
        }

        return true;
    }

    // GCOVR_EXCL_STOP

    void LowFileName(std::basic_string<char> &filePath) {
#ifdef _WIN32
        auto len = filePath.length();
        if (len == 0) { return; }
        for (auto i = len - 1; i >= 0; i--) {
            if (filePath[i] == '/' || filePath[i] == '\\') { break; }
            if (isupper(filePath[i])) {
                filePath[i] = tolower(filePath[i]);
            }
        }
#endif
    }

    bool IsMacroExpandTest(const std::string &path) {
        // just filter test in cjnative_backend
#ifdef  CANGJIE_CODEGEN_CJVM_BACKEND
    return false;
#else
        std::regex pathRegex(MACRO_PATH);
        std::vector<std::string> vectorString(std::sregex_token_iterator(path.begin(), path.end(), pathRegex, -1),
                                              std::sregex_token_iterator());
        const size_t NUM = 2; // path need have only one Macro folder, so vectorString size is 2
        if (vectorString.size() == NUM) {
            return true;
        }
        return false;
#endif
    }

    void ReadExpectedCallHierarchyResult(std::string &baseFile, CallHierarchyResult &expect) {
        SingleInstance *p = SingleInstance::GetInstance();
        std::string expectedFile = "";
        AdaptCjvmTest(baseFile, p, expectedFile);
        nlohmann::json root;
        std::string errs;
        std::ifstream infile;
        infile.open(expectedFile);

        if (!infile.is_open()) {
            return;
        }
        while (infile.good() && !infile.eof()) {
            char buf[MAX_LEN] = {0};
            infile.getline(buf, MAX_LEN);
            std::string message = std::string(buf);
            try {
                if (message.empty()) {
                    continue;
                }
                root = nlohmann::json::parse(message);
            } catch (nlohmann::detail::parse_error &errs) {
                continue;
            }
            if (root.empty()) {
                continue;
            }
            CreateCallHierarchyStruct(root, expect);
            break;
        }
        infile.close();
    }

    void CreatePrePareCallHierarchyStruct(const nlohmann::json &exp, CallHierarchyResult &actual) {
        if (exp.contains("result")) {
            for (int i = 0; i < exp["result"].size(); i++) {
                auto symbolId = exp["result"][i]["data"].value("symbolId", "");
                if (!symbolId.empty()) {
                    actual.symbolId = std::stoull(symbolId);
                }
            }
        }
    }

    void CreateCallHierarchyStruct(const nlohmann::json &exp, CallHierarchyResult &actual) {
        if (exp.contains("result")) {
            for (int i = 0; i < exp["result"].size(); i++) {
                if (exp["result"][i].contains("to")) {
                    ark::CallHierarchyOutgoingCall outgoingCall;
                    outgoingCall.to.name = exp["result"][i]["to"].value("name", "");
                    outgoingCall.to.kind = ark::SymbolKind(exp["result"][i]["to"].value("kind", -1));
                    outgoingCall.to.detail = exp["result"][i]["to"].value("detail", "");
                    outgoingCall.to.uri.file = GetFileUrl(exp["result"][i]["to"].value("uri", ""));
                    outgoingCall.to.range = CreateRangeStruct(exp["result"][i]["to"]);
                    outgoingCall.to.selectionRange = CreateSelectionRangeStruct(exp["result"][i]["to"]);
                    auto symbolId = exp["result"][i]["to"]["data"].value("symbolId", "");
                    if (!symbolId.empty()) {
                        actual.symbolId = std::stoull(symbolId);
                    }
                    if (exp["result"][i].contains("fromRanges")) {
                        for (int j = 0; j < exp["result"][i]["fromRanges"].size(); ++j) {
                            ark::Range tempRange;
                            tempRange.start.column = exp["result"][i]["fromRanges"][j]["start"].value("character", -1);
                            tempRange.start.line = exp["result"][i]["fromRanges"][j]["start"].value("line", -1);
                            tempRange.end.column = exp["result"][i]["fromRanges"][j]["end"].value("character", -1);
                            tempRange.end.line = exp["result"][i]["fromRanges"][j]["end"].value("line", -1);
                            outgoingCall.fromRanges.emplace_back(tempRange);
                        }
                    }
                    actual.outgoingCall.emplace_back(outgoingCall);
                } else {
                    ark::CallHierarchyIncomingCall incomingCall;
                    incomingCall.from.name = exp["result"][i]["from"].value("name", "");
                    incomingCall.from.kind = ark::SymbolKind(exp["result"][i]["from"].value("kind", -1));
                    incomingCall.from.detail = exp["result"][i]["from"].value("detail", "");
                    incomingCall.from.uri.file = GetFileUrl(exp["result"][i]["from"].value("uri", ""));
                    incomingCall.from.range = CreateRangeStruct(exp["result"][i]["from"]);
                    incomingCall.from.selectionRange = CreateSelectionRangeStruct(exp["result"][i]["from"]);
                    auto symbolId = exp["result"][i]["from"]["data"].value("symbolId", "");
                    if (!symbolId.empty()) {
                        actual.symbolId = std::stoull(symbolId);
                    }
                    if (exp["result"][i].contains("fromRanges")) {
                        for (int j = 0; j < exp["result"][i]["fromRanges"].size(); ++j) {
                            ark::Range tempRange;
                            tempRange.start.column = exp["result"][i]["fromRanges"][j]["start"].value("character", -1);
                            tempRange.start.line = exp["result"][i]["fromRanges"][j]["start"].value("line", -1);
                            tempRange.end.column = exp["result"][i]["fromRanges"][j]["end"].value("character", -1);
                            tempRange.end.line = exp["result"][i]["fromRanges"][j]["end"].value("line", -1);
                            incomingCall.fromRanges.emplace_back(tempRange);
                        }
                    }
                    actual.incomingCall.emplace_back(incomingCall);
                    actual.isOutgoingCall = false;
                }
            }
        }
    }

    // GCOVR_EXCL_START
    bool CheckCallHierarchyResult(CallHierarchyResult &actual, CallHierarchyResult &expect, std::string &reason) {
        if (actual.isOutgoingCall && expect.isOutgoingCall) {
            std::vector<ark::CallHierarchyOutgoingCall> actualOutgoingCall = actual.outgoingCall;
            std::vector<ark::CallHierarchyOutgoingCall> expectOutgoingCall = expect.outgoingCall;
            if (!CheckResultCount(actualOutgoingCall, expectOutgoingCall, false)) {
                reason = "expect and actual CallHierarchyOutgoingCall number is different";
                return false;
            }
            for (int i = 0; i < actualOutgoingCall.size(); ++i) {
                for (int j = 0; j < actualOutgoingCall[i].fromRanges.size(); ++j) {
                    if (actualOutgoingCall[i].fromRanges[j] != expectOutgoingCall[i].fromRanges[j]) {
                        reason = actualOutgoingCall[i].to.name + " and " + expectOutgoingCall[i].to.name +
                                 " have different fromRanges.";
                        return false;
                    }
                }
                if (actualOutgoingCall[i].to.name != expectOutgoingCall[i].to.name) {
                    reason = actualOutgoingCall[i].to.name + " and " + expectOutgoingCall[i].to.name +
                             " have different names.";
                    return false;
                }
                if (actualOutgoingCall[i].to.kind != expectOutgoingCall[i].to.kind) {
                    reason = actualOutgoingCall[i].to.name + " and " + expectOutgoingCall[i].to.name +
                             " have different kind.";
                    return false;
                }
                if (actualOutgoingCall[i].to.detail != expectOutgoingCall[i].to.detail) {
                    reason = actualOutgoingCall[i].to.name + " and " + expectOutgoingCall[i].to.name +
                             " have different detail.";
                    return false;
                }
                if (actualOutgoingCall[i].to.uri.file != expectOutgoingCall[i].to.uri.file) {
                    reason = actualOutgoingCall[i].to.name + " and " + expectOutgoingCall[i].to.name +
                             " have different uri.";
                    return false;
                }
                if (actualOutgoingCall[i].to.range != expectOutgoingCall[i].to.range) {
                    reason = actualOutgoingCall[i].to.name + " and " + expectOutgoingCall[i].to.name +
                             " have different range.";
                    return false;
                }
                if (actualOutgoingCall[i].to.selectionRange != expectOutgoingCall[i].to.selectionRange) {
                    reason = actualOutgoingCall[i].to.name + " and " + expectOutgoingCall[i].to.name +
                             " have different selectionRange.";
                    return false;
                }
            }
        } else if (!actual.isOutgoingCall || !expect.isOutgoingCall) {
            std::sort(actual.incomingCall.begin(), actual.incomingCall.end(),
                      CompareCallHierarchyIncomingCall<ark::CallHierarchyIncomingCall>);
            std::sort(expect.incomingCall.begin(), expect.incomingCall.end(),
                      CompareCallHierarchyIncomingCall<ark::CallHierarchyIncomingCall>);
            std::vector<ark::CallHierarchyIncomingCall> actualIncomingCall = actual.incomingCall;
            std::vector<ark::CallHierarchyIncomingCall> expectIncomingCall = expect.incomingCall;
            if (!CheckResultCount(actualIncomingCall, expectIncomingCall, false)) {
                reason = "expect and actual CallHierarchyOutgoingCall number is different";
                return false;
            }
            for (int i = 0; i < actualIncomingCall.size(); ++i) {
                for (int j = 0; j < actualIncomingCall[i].fromRanges.size(); ++j) {
                    if (actualIncomingCall[i].fromRanges[j] != expectIncomingCall[i].fromRanges[j]) {
                        reason = actualIncomingCall[i].from.name + " and " + expectIncomingCall[i].from.name +
                                 " have different fromRanges.";
                        return false;
                    }
                }
                if (actualIncomingCall[i].from.name != expectIncomingCall[i].from.name) {
                    reason = actualIncomingCall[i].from.name + " and " + expectIncomingCall[i].from.name +
                             " have different names.";
                    return false;
                }
                if (actualIncomingCall[i].from.kind != expectIncomingCall[i].from.kind) {
                    reason = actualIncomingCall[i].from.name + " and " + expectIncomingCall[i].from.name +
                             " have different kind.";
                    return false;
                }
                if (actualIncomingCall[i].from.detail != expectIncomingCall[i].from.detail) {
                    reason = actualIncomingCall[i].from.name + " and " + expectIncomingCall[i].from.name +
                             " have different detail.";
                    return false;
                }
                if (actualIncomingCall[i].from.uri.file != expectIncomingCall[i].from.uri.file) {
                    reason = actualIncomingCall[i].from.name + " and " + expectIncomingCall[i].from.name +
                             " have different uri.";
                    return false;
                }
                if (actualIncomingCall[i].from.range != expectIncomingCall[i].from.range) {
                    reason = actualIncomingCall[i].from.name + " and " + expectIncomingCall[i].from.name +
                             " have different range.";
                    return false;
                }
                if (actualIncomingCall[i].from.selectionRange != expectIncomingCall[i].from.selectionRange) {
                    reason = actualIncomingCall[i].from.name + " and " + expectIncomingCall[i].from.name +
                             " have different selectionRange.";
                    return false;
                }
            }
        } else {
            reason = "CallHierarchyIncomingCall and CallHierarchyOutgoingCall cannot be compared with each other.";
            return false;
        }
        return true;
    }

    // GCOVR_EXCL_STOP
}
