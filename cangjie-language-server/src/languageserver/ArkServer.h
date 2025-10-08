// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#ifndef LSPSERVER_ARKSERVER_H
#define LSPSERVER_ARKSERVER_H

#include <cstdint>
#include <sstream>
#include "../json-rpc/Protocol.h"
#include "../json-rpc/Transport.h"
#include "ArkScheduler.h"
#include "CompilerCangjieProject.h"
#include "common/BasicHelper.h"
#include "capabilities/semanticHighlight/SemanticHighlightImpl.h"
#include "capabilities/documentHighlight/DocumentHighlightImpl.h"
#include "capabilities/completion/CompletionImpl.h"
#include "capabilities/prepareRename/PrepareRename.h"
#include "capabilities/rename/RenameImpl.h"
#include "capabilities/definition/LocateSymbolAtImpl.h"
#include "capabilities/reference/FindReferencesImpl.h"
#include "capabilities/breakpoints/BreakpointsImpl.h"
#include "capabilities/callHierarchy/CallHierarchyImpl.h"
#include "capabilities/codeLens/CodeLensImpl.h"

namespace ark {
enum class CompleteMode {
    SEMA_COMPLETE,
    PARSE_COMPLETE,
    NONE_COMPLETE
};
class ArkServer {
public:
    explicit ArkServer(Callbacks *callbacks = nullptr);

    // Get document highlights for a given position.
    void FindDocumentHighlights(const std::string &file, const TextDocumentPositionParams &params,
                                const Callback<ValueOrError> &reply) const;

    void FindTypeHierarchys(const std::string &file, const TextDocumentPositionParams &params,
                            const Callback<ValueOrError> &reply) const;

    void
    FindSuperTypes(const std::string &file, const TypeHierarchyItem &params,
                   const Callback<ValueOrError> &reply) const;

    void
    FindSubTypes(const std::string &file, const TypeHierarchyItem &params,
                 const Callback<ValueOrError> &reply) const;

    void FindCallHierarchys(const std::string &file, const TextDocumentPositionParams &params,
                            const Callback<ValueOrError> &reply) const;

    void
    FindOnIncomingCalls(const std::string &file, const CallHierarchyItem &params,
                        const Callback<ValueOrError> &reply) const;

    void
    FindOnOutgoingCalls(const std::string &file, const CallHierarchyItem &params,
                        const Callback<ValueOrError> &reply) const;

    void FindReferences(const std::string &file, const TextDocumentPositionParams &params,
                        const Callback<ValueOrError> &reply) const;

    void LocateSymbolAt(const std::string &file, const TextDocumentPositionParams &params,
                        const Callback<ValueOrError> &reply) const;
    // Get semantic tokens for the full doc
    void FindSemanticTokensHighlight(const std::string &file, const Callback<ValueOrError> &reply) const;

    void FindDocumentLink(const std::string &file, const Callback<ValueOrError> &reply) const;

    void FindCompletion(const CompletionParams &params, const std::string &file,
                        const Callback <ValueOrError> &reply) const;

    void PrepareRename(const std::string &file,
                       const TextDocumentPositionParams &params, const Callback<ValueOrError> &reply) const;

    void Rename(const std::string &file, const RenameParams &params,
                const Callback <ValueOrError> &reply) const;

    void FindBreakpoints(const std::string &file, const Callback <ValueOrError> &reply) const;

    void FindSignatureHelp(const SignatureHelpParams &params, const std::string &file,
                           const Callback<ValueOrError> &reply) const;

    void FindCodeLens(const std::string &file, const Callback <ValueOrError> &reply) const;

    void FindWorkspaceSymbols(const std::string &query, const Callback<ValueOrError> &reply) const;

    // Get hover for a given position.
    void FindHover(const std::string &file,
                   const TextDocumentPositionParams &params, const Callback<ValueOrError> &reply) const;

    void ChangeWatchedFiles(const std::string &file, FileChangeType type, DocCache *docMgr) const;

    void AddDoc(const std::string &file, const std::string &contents, int64_t version, NeedDiagnostics needDiagnostics,
                bool forceRebuild) const;

    void FindDocumentSymbol(const DocumentSymbolParams &params, const Callback<ValueOrError> &reply) const;

    Cangjie::Position AlterPosition(const std::string &file, const TextDocumentPositionParams &params) const;

    void UpdateModifierDiag(const InputsAndAST &inputAST, std::vector<DiagnosticToken> &diagnostics) const;

private:
    Callbacks *callback = nullptr;
    std::unique_ptr<ark::ArkScheduler> arkScheduler;
    std::unique_ptr<ark::ArkScheduler> arkSchedulerOfComplete;
    std::unique_ptr<ark::ArkScheduler> arkSchedulerOfSignature;
};
} // namespace ark


#endif // LSPSERVER_ARKSERVER_H
