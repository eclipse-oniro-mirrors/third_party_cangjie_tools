// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "PrepareRename.h"
#include "../../common/Inherit/InheritImpl.h"

using namespace Cangjie;
using namespace CONSTANTS;
namespace ark {
Range PrepareRename::PrepareImpl(const ArkAST &ast, Cangjie::Position pos, MessageErrorDetail &errorInfo)
{
    Logger &logger = Logger::Instance();
    logger.LogMessage(MessageType::MSG_LOG, "PrepareRename::PrepareImpl in.");
    Range range = {.start = {0, -1, -1}, .end = {0, -1, -1}};

    // adjust position from IDE to AST
    pos = PosFromIDE2Char(pos);
    PositionIDEToUTF8(ast.tokens, pos, *ast.file);

    // check current token is the kind required in function CheckTokenKind(TokenKind)
    int index = ast.GetCurTokenByPos(pos, 0, static_cast<int>(ast.tokens.size()) - 1, true);
    if (index == -1 || !ast.packageInstance) {
        return range;
    }
    std::string query = "_ = (" + std::to_string(pos.fileID) + ", "
                        + std::to_string(pos.line) + ", " + std::to_string(pos.column) + ")";

    auto syms = SearchContext(ast.packageInstance->ctx, query);
    if (syms.empty()) {
        logger.LogMessage(MessageType::MSG_LOG, "the result of search is empty.");
        return range;
    }
    Ptr<Cangjie::AST::Node> node = syms[0]->node;
    if (!node) {
        return range;
    }
    bool invalid = node->symbol && node->symbol->name == "init" &&
                   !node->TestAttr(Cangjie::AST::Attribute::PRIMARY_CONSTRUCTOR);
    if (invalid) {
        return range;
    }
    std::vector<Ptr<Decl> > decls;
    Ptr<Decl> decl = ast.GetDeclByPosition(pos, syms, decls, {true, true});
    invalid = decls.empty() || !decl || IsMarkPos(node, pos);
    if (invalid) {
        return range;
    }
    invalid = !decl || !IsFromSrcOrNoSrc(decl) || !IsNotFromLibInherit(decl) ||
              decl->IsInvalid();
    if (invalid) {
        return range; // if it's imported from an .cjo file
    }
    if (IsFromMacroCallFile(decl)) {
        errorInfo.message = "Renaming is not supported for elements located in macro-call file.";
        errorInfo.code = ErrorCode::INVALID_RENAME_FOR_MACRO_CALL_FILE;
        return range;
    }
    invalid = decl && decl->ty && decl->ty->IsInvalid();
    if (invalid) {
        return range;
    }
    auto *funcDecl = dynamic_cast<const FuncDecl*>(decl.get());
    invalid = funcDecl && (funcDecl->isGetter || funcDecl->isSetter ||
        (funcDecl->outerDecl && IsFromMacroCallFile(funcDecl->outerDecl)));
    if (invalid) {
        return range;
    }
    invalid = funcDecl && funcDecl->funcBody && funcDecl->funcBody->retType && funcDecl->funcBody->retType->ty &&
              funcDecl->funcBody->retType->ty->IsInvalid();
    if (invalid) {
        return range;
    }
    auto curToken = ast.tokens[static_cast<unsigned int>(index)];
    range.start = curToken.Begin();
    range.end = {
        curToken.Begin().fileID, curToken.Begin().line,
        curToken.Begin().column + static_cast<int>(CountUnicodeCharacters(curToken.Value()))
    };
    // deal rename dialog like $Sth
    if (curToken.kind == Cangjie::TokenKind::DOLLAR_IDENTIFIER) {
        range.start = { curToken.Begin().fileID, curToken.Begin().line, curToken.Begin().column + 1 };
    }

    // deal rename dialog like """ abc\r\ndef """
    if (curToken.kind == Cangjie::TokenKind::MULTILINE_STRING) {
        range.start = node->begin;
        range.end = node->end;
    }

    SetRangForInterpolatedStrInRename(curToken, node, range, pos);

    if (curToken.Value().substr(0, 1) == "`" && curToken.Value().substr(curToken.Value().size() - 1, 1) == "`") {
        range.start = range.start + 1;
        range.end = range.end - 1;
    }
    if (node->symbol && node->symbol->name != curToken.Value()) {
        UpdateRange(ast.tokens, range, *node, false);
    } else {
        UpdateRange(ast.tokens, range, *node);
    }
    range = TransformFromChar2IDE(range);
    return range;
}

bool PrepareRename::IsNotFromLibInherit(Ptr<Decl> decl)
{
    if (decl->astKind != Cangjie::AST::ASTKind::PROP_DECL && decl->astKind != Cangjie::AST::ASTKind::FUNC_DECL) {
        return true;
    }
    Ptr<InheritableDecl> classLikeOrStructDecl{nullptr};
    if (decl->astKind == Cangjie::AST::ASTKind::FUNC_DECL) {
        auto *pFuncDecl = dynamic_cast<FuncDecl*>(decl.get());
        classLikeOrStructDecl = dynamic_cast<InheritableDecl*>(decl->outerDecl.get());
        if (!pFuncDecl || !classLikeOrStructDecl) { return true; }
        std::vector<Ptr<InheritableDecl> > topClasses = GetTopClassDecl(*classLikeOrStructDecl, true);
        for (const auto &topDecl: topClasses) {
            for (const auto &memberDecl: topDecl->GetMemberDecls()) {
                auto *pDecl = dynamic_cast<FuncDecl*>(memberDecl.get().get());
                if (pDecl && IsFuncSignatureIdentical(*pFuncDecl, *pDecl)) {
                    return false;
                }
            }
        }
        return true;
    }
    auto *propDecl = dynamic_cast<PropDecl*>(decl.get());
    classLikeOrStructDecl = dynamic_cast<InheritableDecl*>(decl->outerDecl.get());
    if (!propDecl || !classLikeOrStructDecl) { return true; }
    std::vector<Ptr<InheritableDecl> > topClasses = GetTopClassDecl(*classLikeOrStructDecl, true);
    for (const auto &topDecl: topClasses) {
        for (const auto &memberDecl: topDecl->GetMemberDecls()) {
            if (!Cangjie::Is<PropDecl>(memberDecl.get())) { continue; }
            auto *pDecl = dynamic_cast<PropDecl*>(memberDecl.get().get());
            if (propDecl->identifier == pDecl->identifier) {
                return false;
            }
        }
    }
    return true;
}

bool PrepareRename::IsFromMacroCallFile(Ptr<Decl> decl)
{
    if (!decl || !decl->TestAttr(AST::Attribute::MACRO_EXPANDED_NODE)) {
        return false;
    }
    if (decl->TestAttr(Cangjie::AST::Attribute::COMPILER_ADD)) {
        if (decl->identifier == "init") {
            auto funcDecl = dynamic_cast<const FuncDecl*>(decl.get());
            if (funcDecl == nullptr || funcDecl->funcBody == nullptr) {
                return true;
            }
            unsigned int fileID = 0;
            if (funcDecl->funcBody->parentClassLike != nullptr) {
                fileID = funcDecl->funcBody->parentClassLike->GetIdentifierPos().fileID;
            } else if (funcDecl->funcBody->parentStruct != nullptr) {
                fileID = funcDecl->funcBody->parentStruct->GetIdentifierPos().fileID;
            }
            return fileID != decl->identifier.Begin().fileID;
        }
        return true;
    }
    if (decl->curMacroCall) {
        auto newPos = decl->curMacroCall->GetMacroCallPos(decl->identifier.Begin());
        if (newPos.fileID != decl->identifier.Begin().fileID) {
            return true;
        }
    }
    // deal decl in .macroCall
    auto index = ark::CompilerCangjieProject::GetInstance()->GetIndex();
    if (!index) {
        return true;
    }
    auto symFromIndex = index->GetAimSymbol(*decl);
    if (!symFromIndex.location.fileUri.empty()) {
        auto path = symFromIndex.location.fileUri;
        if (Cangjie::FileUtil::HasExtension(path, CANGJIE_MACRO_FILE_EXTENSION)) {
            return true;
        }
    }
    return false;
}
}
