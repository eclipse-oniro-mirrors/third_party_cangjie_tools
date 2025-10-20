// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#ifndef LSPSERVER_FINDREFERENCESIMPL_H
#define LSPSERVER_FINDREFERENCESIMPL_H

#include "../../../json-rpc/Protocol.h"
#include "../../ArkAST.h"
#include "../../common/Utils.h"
#include "../../index/Ref.h"
#include "../../logger/Logger.h"
#include "cangjie/AST/ASTContext.h"
#include "cangjie/AST/Match.h"
#include "cangjie/AST/Symbol.h"
#include "cangjie/Basic/DiagnosticEngine.h"
#include "cangjie/Basic/Match.h"
#include "cangjie/Lex/Token.h"
#include "cangjie/Modules/ImportManager.h"
#include "cangjie/Parse/Parser.h"
#include "cangjie/Utils/FileUtil.h"

namespace ark {
struct ReferencesResult {
    std::set<Location> References{};
};

class FindReferencesImpl {
public:
    static std::string curFilePath;

    static void FindReferences(const ArkAST &ast, ReferencesResult &result, Cangjie::Position pos);

    static void FindFileReferences(const ArkAST &ast, ReferencesResult &result);

    static void GetCurPkgUesage(Ptr<Decl> decl, const ArkAST &ast, ReferencesResult &result);

    static std::unordered_set<std::string> GetSelectedUesScopeNames(Ptr<Decl> decl, const ArkAST &ast, Range &range);

    static void DealGenericParamDecl(
        const ArkAST &ast, ReferencesResult &result, Ptr<Decl> oldDecl, std::vector<Symbol *> &syms);

    static bool IsInvalidRef(const lsp::Ref& ref, Position pos, int curIdx, const ArkAST &ast);

    static void CompileDownStreamPackage(const std::vector<Ptr<Cangjie::AST::Decl>> &decls);
};
} // namespace ark

#endif // LSPSERVER_FINDREFERENCESIMPL_H
