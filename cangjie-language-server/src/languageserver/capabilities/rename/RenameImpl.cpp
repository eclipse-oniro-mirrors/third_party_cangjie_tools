// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "RenameImpl.h"

using namespace Cangjie;
using namespace Cangjie::AST;
namespace ark {

std::string RenameImpl::curFilePath = "";

void HandleResult(const std::string path, const std::set<TextEdit> &testEdit, std::vector<TextDocumentEdit> &result,
                  Callbacks &cb)
{
    TextDocumentEdit textDocumentEdit;
    std::string uri = URI::URIFromAbsolutePath(path).ToString();
    textDocumentEdit.textDocument.uri.file = uri;
    textDocumentEdit.textDocument.version = cb.GetVersionByFile(path);
    textDocumentEdit.textEdits.assign(testEdit.begin(), testEdit.end());
    result.push_back(textDocumentEdit);
}

std::string RenameImpl::GetRealName(std::vector<TextDocumentEdit> &result, Callbacks &cb,
                                    DocumentChanges &documentChanges)
{
    EditMap usersEditNotInDef;
    // merge result
    for (const auto &iter : documentChanges.usersEditMap) {
        if (documentChanges.defineEditMap.find(iter.first) == documentChanges.defineEditMap.end()) {
            (void)usersEditNotInDef.emplace(iter);
            continue;
        }
        for (auto &editMap : iter.second) {
            (void)documentChanges.defineEditMap[iter.first].emplace(editMap);
        }
    }
    for (const auto &iter : documentChanges.defineEditMap) {
        HandleResult(iter.first, iter.second, result, cb);
    }
    for (const auto &iter : usersEditNotInDef) {
        if (!iter.second.empty()) {
            HandleResult(iter.first, iter.second, result, cb);
        }
    }
    if (!documentChanges.defineEditMap.empty()) {
        auto iter = documentChanges.defineEditMap.begin();
        return iter->first;
    }
    return "";
}

void RenameImpl::UpdateUserMap(ark::DocumentChanges &documentChanges, std::string file, TextEdit t)
{
    CompilerCangjieProject::GetInstance()->GetRealPath(file);
    auto found = documentChanges.usersEditMap.find(file);
    if (found == documentChanges.usersEditMap.end()) {
        std::set<TextEdit> textEdits;
        (void)textEdits.emplace(t);
        documentChanges.usersEditMap[file] = textEdits;
    } else {
        (void)documentChanges.usersEditMap[file].emplace(t);
    }
}

void RenameImpl::UpdateDefineMap(ark::DocumentChanges &documentChanges, std::string file, TextEdit t)
{
    CompilerCangjieProject::GetInstance()->GetRealPath(file);
    auto found = documentChanges.defineEditMap.find(file);
    if (found == documentChanges.defineEditMap.end()) {
        std::set<TextEdit> textEdits;
        (void)textEdits.emplace(t);
        documentChanges.defineEditMap[file] = textEdits;
    } else {
        (void)documentChanges.defineEditMap[file].emplace(t);
    }
}

void RenameImpl::GetLocalVarUesage(Ptr<Decl> decl, const ArkAST &ast, ark::DocumentChanges &documentChanges,
                                   const std::string &newName)
{
    if (!decl || !ast.file || !ast.file->curPackage) {
        return;
    }

    auto user = FindDeclUsage(*decl, *ast.file->curPackage);
    for (const auto &U : user) {
        if (U->astKind == ASTKind::MEMBER_ACCESS) {
            continue;
        }
        bool samePackage = false;
        if (U->curFile && U->curFile->curPackage && ast.file->curPackage) {
            std::string targetPackageName = U->curFile->curPackage->fullPackageName;
            std::string packageName = (*(ast.file->curPackage)).fullPackageName;
            samePackage = targetPackageName == packageName;
        }
        TextEdit t{GetProperRange(U, ast.tokens, samePackage), newName};
        UpdateUserMap(documentChanges, curFilePath, t);
    }
}

void RenameImpl::HandleGeneric(Ptr<Decl> defineDecl, const ArkAST &ast, DocumentChanges &documentChanges,
                               const std::string &newName, const std::vector<Symbol *> &syms)
{
    if (auto temp = ast.FindRealGenericParamDeclForExtend(defineDecl->identifier, syms)) {
        defineDecl = temp;
    }
    TextEdit t{TransformFromChar2IDE({defineDecl->begin, defineDecl->end}), newName};
    UpdateDefineMap(documentChanges, curFilePath, t);
    GetLocalVarUesage(defineDecl, ast, documentChanges, newName);
    auto inheritable = dynamic_cast<InheritableDecl*>(defineDecl->outerDecl.get());
    if (!inheritable) {
        return;
    }
    std::string pkgName = GetPkgNameFromNode(ast.file);
    auto extendDecls = CompilerCangjieProject::GetInstance()->GetExtendDecls(inheritable, pkgName);
    for (const auto &extendDecl : extendDecls) {
        if (!extendDecl || !extendDecl->generic) {
            continue;
        }
        for (const auto &genericDecl : extendDecl->generic->typeParameters) {
            if (genericDecl && genericDecl->identifier == defineDecl->identifier) {
                GetLocalVarUesage(genericDecl.get(), ast, documentChanges, newName);
            }
        }
    }
}

void RenameImpl::RenameByIndex(lsp::SymbolID id, DocumentChanges &documentChanges, const std::string &newName)
{
    std::unordered_set<lsp::SymbolID> relationIds;
    lsp::SymbolID topId = id;
    auto index = ark::CompilerCangjieProject::GetInstance()->GetIndex();
    if (!index) {
        return;
    }
    index->FindRiddenUp(id, relationIds, topId);
    index->FindRiddenDown(topId, relationIds);
    (void)relationIds.emplace(topId);

    std::unordered_set<lsp::SymbolID> temIds;
    (void)temIds.emplace(id);
    Range range;
    lsp::LookupRequest req{temIds};
    index->Lookup(req, [&documentChanges, &newName, &range](const lsp::Symbol &sym) {
        if (sym.name == "init") {
            return;
        } // init func could not to be renamed
        std::string file = sym.location.fileUri;
        TextEdit t{TransformFromChar2IDE({sym.location.begin, sym.location.end}), newName};
        range = t.range;
        UpdateDefineMap(documentChanges, file, t);
        return;
    });

    lsp::LookupRequest lookUpReq{relationIds};
    index->Lookup(lookUpReq, [&documentChanges, &newName](const lsp::Symbol &sym) {
        if (sym.name == "init") {
            return;
        } // init func could not to be renamed
        std::string file = sym.location.fileUri;
        TextEdit t{TransformFromChar2IDE({sym.location.begin, sym.location.end}), newName};
        UpdateUserMap(documentChanges, file, t);
    });

    lsp::RefsRequest refsRequest{relationIds, lsp::RefKind::REFERENCE};
    index->Refs(refsRequest, [&documentChanges, &newName, &range](const lsp::Ref &ref) {
        if (ref.isSuper) {
            return;
        }
        std::string file = ref.location.fileUri;
        TextEdit t{TransformFromChar2IDE({ref.location.begin, ref.location.end}), newName};
        if (range != t.range) {
            UpdateUserMap(documentChanges, file, t);
        }
    });
}

template <typename T> void HandleClassOrStruct(Ptr<Decl> defineDecl, std::unordered_set<lsp::SymbolID> &defIds)
{
    if (auto classDecl = dynamic_cast<T *>(defineDecl.get())) {
        (void)defIds.emplace(GetSymbolId(*classDecl));
        for (const auto &md : classDecl->GetMemberDeclPtrs()) {
            if (md->identifier == "init") {
                (void)defIds.emplace(GetSymbolId(*md));
            }
        }
    }
}

void HandleInit(Ptr<Decl> defineDecl, std::unordered_set<lsp::SymbolID> &defIds)
{
    if (defineDecl->astKind == ASTKind::CLASS_DECL) {
        HandleClassOrStruct<ClassDecl>(defineDecl, defIds);
    }
    if (defineDecl->astKind == ASTKind::STRUCT_DECL) {
        HandleClassOrStruct<StructDecl>(defineDecl, defIds);
    }
    if (defineDecl->astKind == ASTKind::FUNC_DECL && defineDecl->identifier == "init") {
        CJC_NULLPTR_CHECK(defineDecl->outerDecl);
        (void)defIds.emplace(GetSymbolId(*defineDecl->outerDecl));
        for (const auto &md : defineDecl->outerDecl->GetMemberDeclPtrs()) {
            if (md->identifier == "init") {
                (void)defIds.emplace(GetSymbolId(*md));
            }
        }
    }
}

std::string RenameImpl::Rename(const ArkAST &ast, std::vector<TextDocumentEdit> &result, Position pos,
                               const std::string &newName, Callbacks &cb)
{
    bool invalid = !ast.file || !ast.packageInstance;
    if (invalid) { return ""; }
    Logger &logger = Logger::Instance();
    logger.LogMessage(MessageType::MSG_LOG, "RenameImpl::Rename in.");

    // update input id
    pos.fileID = ast.fileID;
    // adjust position from IDE to AST
    pos = PosFromIDE2Char(pos);
    PositionIDEToUTF8(ast.tokens, pos, *ast.file);
    int idx = ast.GetCurTokenByPos(pos, 0, static_cast<int>(ast.tokens.size()) - 1);
    // get curFilePath
    curFilePath = ast.file->filePath;
    std::vector<Symbol *> syms;
    std::vector<Ptr<Cangjie::AST::Decl> > decls;
    DocumentChanges documentChanges;
    Ptr<Decl> defineDecl = ast.GetDeclByPosition(pos, syms, decls, {true, true});
    Token curToken = ast.tokens[static_cast<size_t>(idx)];
    if (IsModifierBeforeDecl(defineDecl, curToken.Begin())) {
        logger.LogMessage(MessageType::MSG_WARNING, "this token does not need to rename");
        return "";
    }

    // Searching for local variables by ast
    if (defineDecl->astKind == ASTKind::GENERIC_PARAM_DECL) {
        HandleGeneric(defineDecl, ast, documentChanges, newName, syms);
        return GetRealName(result, cb, documentChanges);
    }

    if (!IsGlobalOrMemberOrItsParam(*defineDecl)) {
        Position definePos = defineDecl->identifier.Begin();
        if (defineDecl->curMacroCall && defineDecl->curMacroCall->GetInvocation()) {
            auto inv = defineDecl->curMacroCall->GetInvocation();
            // revise origin position before macro expand
            if (inv->new2originPosMap.find(defineDecl->identifier.Begin().column) !=
                inv->new2originPosMap.end()) {
                definePos = inv->new2originPosMap[defineDecl->identifier.Begin().column];
            }
        }
        TextEdit t{TransformFromChar2IDE({definePos, definePos + CountUnicodeCharacters(defineDecl->identifier)}),
                   newName};
        UpdateDefineMap(documentChanges, curFilePath, t);
        GetLocalVarUesage(defineDecl, ast, documentChanges, newName);
        return GetRealName(result, cb, documentChanges);
    }

    // Searching for nonlocal variables by index
    std::unordered_set<lsp::SymbolID> defIds;
    // Deal member param
    if (defineDecl->astKind == ASTKind::FUNC_PARAM || defineDecl->astKind == ASTKind::VAR_DECL) {
        DealMemberParam(curFilePath, syms, decls, defineDecl);
    }

    for (const auto &decl : decls) {
        if (decl->astKind == ASTKind::PRIMARY_CTOR_DECL) {
            continue;
        }
        auto id = GetSymbolId(*decl);
        if (id == lsp::INVALID_SYMBOL_ID) {
            continue;
        }
        (void)defIds.emplace(GetSymbolId(*decl));
    }

    // Get init funcs of struct/class and their own
    HandleInit(defineDecl, defIds);

    for (const auto &id : defIds) {
        RenameByIndex(id, documentChanges, newName);
    }

    return GetRealName(result, cb, documentChanges);
}

} // namespace ark
