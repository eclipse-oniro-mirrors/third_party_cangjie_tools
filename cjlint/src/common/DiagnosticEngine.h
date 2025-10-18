// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#ifndef COMMON_DIAGNOSTICENGINE_H
#define COMMON_DIAGNOSTICENGINE_H

// code come from DiagnosticEngine.h

#include <atomic>
#include <fstream>
#include <list>
#include <mutex>
#include <utility>
#include <variant>
#include <vector>

#include "cangjie/Basic/Position.h"
#include "cangjie/Basic/Print.h"
#include "cangjie/Basic/SourceManager.h"
#include "cangjie/Basic/Utils.h"
#include "cangjie/Utils/FileUtil.h"
#include "nlohmann/json.hpp"

namespace Cangjie::CodeCheck {
using ArgType = std::variant<int, std::string, char, Position>;

// In future, this implement need to replace by thread safe log system, we can see spdlog, Log4cplus, glog,
// log4cxx

/**
 * CodeCheckDiagArgument is a struct packing the argument of Diagnose function.
 */
struct CodeCheckDiagArgument {
public:
    ArgType arg;
    CodeCheckDiagArgument() = default;
    CodeCheckDiagArgument(int args) : arg(args) {}
    CodeCheckDiagArgument(std::string args) : arg(args) {}
    CodeCheckDiagArgument(const char* args) : arg(args) {}
    CodeCheckDiagArgument(int64_t args) : arg(static_cast<int>(args)) {}
    CodeCheckDiagArgument(size_t args) : arg(static_cast<int>(args)) {}
    CodeCheckDiagArgument(char args) : arg(args) {}
    CodeCheckDiagArgument(Position pos) : arg(pos) {}
    ~CodeCheckDiagArgument() {}
};

struct CjlintIgnoreInfo {
    std::string path;
    int pos;
    std::vector<std::string> rules;
    int start = 0;
    int end = 0;
};

/*
 * DiagKind is the specific diagnostic kind.
 */
enum class CodeCheckDiagKind {
#define MANDATORY(Kind, Info) Kind,
#define SUGGESTIONS(Kind, Info) Kind,
#include "common/DiagnosticsAll.def"
#undef MANDATORY
#undef SUGGESTIONS
};

enum class CodeCheckClearanceRequirement { MANDATORY, SUGGESTIONS };

constexpr const char* const CODE_CHECK_CLEARANCE_REQUIREMENT_NAMES[] = {"MANDATORY", "SUGGESTIONS"};

enum class CodeCheckDiagCategory { MANDATORY, SUGGESTIONS };
constexpr const CodeCheckClearanceRequirement DIAG_CLEARANCE_REQUIREMENTS[]{
#define MANDATORY(Kind, Info) CodeCheckClearanceRequirement::MANDATORY,
#define SUGGESTIONS(Kind, Info) CodeCheckClearanceRequirement::SUGGESTIONS,
#include "common/DiagnosticsAll.def"
#undef MANDATORY
#undef SUGGESTIONS
};

constexpr const char* const CODE_CHECK_DIAG_MESSAGES[] = {
#define MANDATORY(Kind, Info) Info,
#define SUGGESTIONS(Kind, Info) Info,
#include "common/DiagnosticsAll.def"
#undef MANDATORY
#undef SUGGESTIONS
};

constexpr const char* const CODE_CHECK_DIAG_NAMES[] = {
#define MANDATORY(Kind, Info) #Kind,
#define SUGGESTIONS(Kind, Info) #Kind,
#include "common/DiagnosticsAll.def"
#undef MANDATORY
#undef SUGGESTIONS
};

/**
 * Diagnostic contains all diagnostic information.
 */
class CodeCheckDiagnostic {
public:
    template <typename... Args>
    CodeCheckDiagnostic(const Position pos, const CodeCheckDiagKind kind, std::string p, const Args... args)
        : start(pos),
          end(pos + 1),
          kind(kind),
          path(std::move(p)),
          args{args...},
          diagCategory(GetDiagnoseCategory(kind)),
          clearanceRequirement(DIAG_CLEARANCE_REQUIREMENTS[static_cast<int>(kind)]){};
    template <typename... Args>
    CodeCheckDiagnostic(
        const Position start, const Position end, const CodeCheckDiagKind kind, std::string p, const Args... args)
        : start(start),
          end(end),
          kind(kind),
          path(std::move(p)),
          args{args...},
          diagCategory(GetDiagnoseCategory(kind)),
          clearanceRequirement(DIAG_CLEARANCE_REQUIREMENTS[static_cast<int>(kind)]){};
    ~CodeCheckDiagnostic() = default;
    Position start;         /* *< Diagnostic start position */
    Position end;           /* *< Diagnostic end position */
    CodeCheckDiagKind kind; /* *< Diagnostic kind, corresponding coding style standards */
    std::string path{};
    std::vector<CodeCheckDiagArgument> args;
    std::string diagMessage{};
    CodeCheckDiagCategory diagCategory;
    CodeCheckClearanceRequirement clearanceRequirement;

    nlohmann::ordered_json ToJsonValue() const;
    std::string ToCsvValue() const;
    static CodeCheckDiagCategory GetDiagnoseCategory(CodeCheckDiagKind diagKind);
};

/**
 * DiagnosticEngine is main diagnostic processing center. It is responsible for handle diagnositcs and emit
 * diagnostic information.
 */
class CodeCheckDiagnosticEngine {
public:
    CodeCheckDiagnosticEngine() : reportFile(""), reportToFile(false) {}
    explicit CodeCheckDiagnosticEngine(std::string reportFile) : reportFile(std::move(reportFile)), reportToFile(true)
    {
    }
    ~CodeCheckDiagnosticEngine();
    void DiagnosticToFile();
    void SetReportToFile(const std::string& fileOfReport, const std::string& formatType)
    {
        this->reportFile = fileOfReport;
        this->format = formatType.empty() ? "json" : formatType;
        this->reportToFile = true;
    }

    void SetJsonInfo(nlohmann::json& jsonData) { this->jsonInfo = jsonData; }

    void SetCjlintIgnoreInfos(const std::vector<CjlintIgnoreInfo>& ignoreInfos)
    {
        this->cjlintIgnoreInfos = ignoreInfos;
    }

    void SetSourceManager(SourceManager* s)
    {
        InternalError(s != nullptr, "source manager is empty when set in diagnostic engine");
        this->sm = s;
    }

    SourceManager& GetSourceManager()
    {
        InternalError(sm != nullptr, "source manager is empty in diagnostic engine");
        return *sm;
    }

    bool PushFlag(const std::string& ruleType, const CodeCheckDiagnostic& diagnostic)
    {
        // if 'exclude_lists.json' don't have 'reuleType', it will return JSON_ERR
        // it means that there is no diag of 'ruleType' should be block
        // return true to push diag to diagList
        if (!jsonInfo.contains(ruleType)) {
            return true;
        }
        const auto& num = jsonInfo[ruleType];
        for (const auto& item : num) {
            std::string stringCol, stringLine, stringPath;

            if (item.contains("column")) {
                stringCol = item["column"].get<std::string>();
            }
            if (item.contains("line")) {
                stringLine = item["line"].get<std::string>();
            }
            if (item.contains("path")) {
                stringPath = item["path"].get<std::string>();
            }

            if (!stringCol.empty() && std::to_string(diagnostic.start.column) != stringCol) {
                continue;
            }
            std::string endStr = ".cj";
            std::string normalizePath = FileUtil::Normalize(stringPath);
            if (std::equal(endStr.rbegin(), endStr.rend(), stringPath.rbegin()) &&
                (sm->GetSource(diagnostic.start.fileID).path.find(normalizePath) != std::string::npos) &&
                std::to_string(diagnostic.start.line) == stringLine) {
                return false;
            }
        }
        return true;
    }

    bool PushCjlintIgnoreFlag(const std::string& ruleType, const CodeCheckDiagnostic& diagnostic)
    {
        for (auto& info : cjlintIgnoreInfos) {
            for (auto& rule : info.rules) {
                if (rule != ruleType ||
                    sm->GetSource(diagnostic.start.fileID).path.find(info.path) == std::string::npos) {
                    continue;
                }
                if (info.start != 0 && info.end != 0 && diagnostic.start.line > info.start &&
                    diagnostic.start.line < info.end) {
                    return false;
                }
                if (diagnostic.start.line == info.pos) {
                    return false;
                }
            }
        }
        return true;
    }

    void ProcessDiagnostic(CodeCheckDiagnostic& diagnostic)
    {
        ConvertToDiagMessage(diagnostic);

        switch (diagnostic.clearanceRequirement) {
            case CodeCheckClearanceRequirement::MANDATORY: {
                IncreaseErrorCount();
                break;
            }
            case CodeCheckClearanceRequirement::SUGGESTIONS: {
                IncreaseWarningCount();
                break;
            }
        }
    }

    /* *
     * Diagnose API.
     * @param pos The Diagnostic start position.
     * @param kind The Diagnostic kind.
     * @param args The Diagnostic format arguments.
     * @return
     */
    template <typename... Args>
    void Diagnose(const Position start, const Position end, CodeCheckDiagKind kind, const Args... args)
    {
        CodeCheckDiagnostic diagnostic(start, end, kind, sm->GetSource(start.fileID).path, args...);
        ProcessDiagnostic(diagnostic);

        auto ruleType = Cangjie::Utils::SplitString(diagnostic.diagMessage, ":");
        if (diagnostic.start == Position{0, 0, 0}) {
            return;
        }
        if (PushFlag(ruleType[0], diagnostic) && PushCjlintIgnoreFlag(ruleType[0], diagnostic)) {
            PushToDiagnosticList(diagnostic);
        }
    }

    template <typename... Args> void Diagnose(const Position pos, CodeCheckDiagKind kind, const Args... args)
    {
        CodeCheckDiagnostic diagnostic(pos, kind, sm->GetSource(pos.fileID).path, args...);
        ProcessDiagnostic(diagnostic);

        auto ruleType = Cangjie::Utils::SplitString(diagnostic.diagMessage, ":");
        if (diagnostic.start == Position{0, 0, 0}) {
            return;
        }
        if (PushFlag(ruleType[0], diagnostic) && PushCjlintIgnoreFlag(ruleType[0], diagnostic)) {
            PushToDiagnosticList(diagnostic);
        }
    }

private:
    static std::atomic<unsigned> errorCount;
    static std::atomic<unsigned> warningCount;
    std::string reportFile;
    std::string format;
    bool reportToFile;
    std::list<CodeCheckDiagnostic> diagnosticList;
    nlohmann::json jsonInfo;
    std::vector<CjlintIgnoreInfo> cjlintIgnoreInfos;
    std::mutex lock;
    SourceManager* sm = nullptr;

    void DiagnosticPrint(const CodeCheckDiagnostic& diag);
    void PushToDiagnosticList(CodeCheckDiagnostic& diag);

    /* *
     * Convert unformat diagnostic message to real diagnostic message.
     */
    void ConvertToDiagMessage(CodeCheckDiagnostic& diagnostic) noexcept;
    std::string ConvertDiagListToJsonString();
    std::string ConvertDiagListToCsvString();
    static void IncreaseErrorCount() { ++errorCount; }
    static void IncreaseWarningCount() { ++warningCount; }
};
} // namespace Cangjie::CodeCheck

#endif // COMMON_DIAGNOSTICENGINE_H