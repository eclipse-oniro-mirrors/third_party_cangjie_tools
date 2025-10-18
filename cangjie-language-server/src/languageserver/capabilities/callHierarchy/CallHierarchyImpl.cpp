// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "CallHierarchyImpl.h"

using namespace std;
using namespace Cangjie;
using namespace AST;

namespace ark {
CallHierarchyItem DeclToCallHierarchyItem(Ptr<const Decl> decl)
{
    CallHierarchyItem result;
    if (!decl) { return result; }
    string path = decl->curFile->filePath;
    CompilerCangjieProject::GetInstance()->GetRealPath(path);
    if (FileUtil::FileExist(path)) {
        result.uri.file = URI::URIFromAbsolutePath(path).ToString();
    } else {
        result.uri.file = URI::URIFromAbsolutePath(CallHierarchyImpl::curFilePath).ToString();
        result.isKernel = true;
    }
    result.detail = decl->fullPackageName + "/" + decl->curFile->fileName;
    size_t pos;
    while ((pos = result.detail.find('/')) != std::string::npos) { (void) result.detail.replace(pos, 1, "."); }
    result.kind = SymbolKind::FUNCTION;
    Ptr<const FuncDecl> funcDecl = dynamic_cast<const FuncDecl*>(decl.get());
    Ptr<const VarDecl> varDecl = dynamic_cast<const VarDecl*>(decl.get());
    result.name = decl->identifier;
    if (funcDecl) {
        if (funcDecl->TestAttr(Attribute::PRIMARY_CONSTRUCTOR) ||
            funcDecl->TestAttr(Attribute::CONSTRUCTOR)) {
            result.name = GetConstructorIdentifier(*funcDecl, false);
        }
        result.name += "(";
        // append params
        const std::vector<std::string> funcParamsTypeName = GetFuncParamsTypeName(funcDecl);
        for (decltype(funcParamsTypeName.size()) i = 0; i < funcParamsTypeName.size(); ++i) {
            if (i != 0) {
                result.name += ", ";
            }
            result.name += funcParamsTypeName[i];
        }
        result.name += ")";
        bool hasRetType = funcDecl->funcBody && funcDecl->funcBody->retType && funcDecl->funcBody->retType->ty;
        result.name += hasRetType ? " : " + GetString(*funcDecl->funcBody->retType->ty) : "";
    }
    if (varDecl) {
        result.name += GetVarDeclType(varDecl);
    }
    ArkAST *arkAst = CompilerCangjieProject::GetInstance()->GetArkAST(path);
    result.range.start = decl->begin;
    result.range.end = decl->end;
    Range range = GetDeclRange(*decl, static_cast<int>(decl->identifier.Val().size()));
    if (decl->TestAttr(Attribute::PRIMARY_CONSTRUCTOR) ||
        decl->TestAttr(Attribute::CONSTRUCTOR)) {
        range = GetConstructorRange(*decl, GetConstructorIdentifier(*decl));
    }
    if (arkAst != nullptr) {
        UpdateRange(arkAst->tokens, range, *decl);
        UpdateRange(arkAst->tokens, result.range, *decl);
    }
    result.range = TransformFromChar2IDE(result.range);
    result.selectionRange = TransformFromChar2IDE(range);
    // find reference by memIndex
    result.symbolId = GetSymbolId(*decl);
    return result;
}

void DealAnonymousConstructorRange(Range& range, const lsp::Symbol&containerSym)
{
    if (!containerSym.location.IsZeroLoc() || containerSym.name != "init") {
        return;
    }
 
    auto index = CompilerCangjieProject::GetInstance()->GetIndex();
    if (!index) {
        return;
    }
    
    lsp::SymbolID outerId = 0;
    index->Relations({containerSym.id, lsp::RelationKind::CONTAINED_BY},
                    [&outerId](const lsp::Relation& rel) {
                        outerId = rel.object;
                    });
    if (outerId) {
        lsp::Symbol outer;
        index->Lookup({{outerId}}, [&outer](const lsp::Symbol& sym) {
            outer = sym;
        });
        range = {outer.location.begin, outer.location.end};
    }
}

CallHierarchyItem DeclToCallHierarchyItem(const lsp::Symbol&containerSym)
{
    CallHierarchyItem result;
    string path = containerSym.location.fileUri;
    if (FileUtil::FileExist(path)) {
        result.uri.file = URI::URIFromAbsolutePath(path).ToString();
    } else {
        result.uri.file = URI::URIFromAbsolutePath(CallHierarchyImpl::curFilePath).ToString();
        result.isKernel = true;
    }
    auto pkgName = CompilerCangjieProject::GetInstance()->GetFullPkgName(path);
    auto fileName = FileUtil::GetFileName(path);
    result.detail = pkgName + "/" + fileName;
    size_t pos;
    while ((pos = result.detail.find('/')) != std::string::npos) { (void) result.detail.replace(pos, 1, "."); }
    result.kind = SymbolKind::FUNCTION;
    std::string symName = containerSym.signature;
    size_t keyWordInitLength = 4;
    if (symName.length() > keyWordInitLength && containerSym.signature.substr(0, keyWordInitLength + 1) == "init(") {
        result.name = symName.replace(0, keyWordInitLength, containerSym.returnType);
    } else {
        result.name = containerSym.signature + ":" + containerSym.returnType;
    }
    Range range = {containerSym.location.begin, containerSym.location.end};
    DealAnonymousConstructorRange(range, containerSym);
    result.range = TransformFromChar2IDE(range);
    result.selectionRange = TransformFromChar2IDE(range);
    result.symbolId = containerSym.id;
    return result;
}

void DealCallee(const std::pair<lsp::SymbolID, std::vector<lsp::Ref>>& callee, lsp::SymbolIndex *index,
    vector<CallHierarchyOutgoingCall>& result)
{
    std::unordered_set<lsp::SymbolID> declSymIds;
    (void)declSymIds.insert(callee.first);
    lsp::LookupRequest lookUpReq{declSymIds};
    lsp::Symbol declSym;
    // find callee symbol by id
    index->Lookup(lookUpReq, [&declSym](const lsp::Symbol &sym) {
        if (sym.location.IsZeroLoc() && sym.name != "init") {
            return;
        }
        declSym = sym;
    });
    if (declSym.signature.empty() || (declSym.kind != ASTKind::FUNC_DECL && declSym.kind != ASTKind::PRIMARY_CTOR_DECL
                && declSym.kind != ASTKind::LAMBDA_EXPR)) {
        return;
    }
    const CallHierarchyItem item = DeclToCallHierarchyItem(declSym);
    CallHierarchyOutgoingCall callHierarchyOutgoingCall;
    callHierarchyOutgoingCall.to = item;
    // deal ref incoming call
    bool rangeIsFromCjo = declSym.isCjoSym;
    std::set<Range> rangesSet;
    for (const auto &ref: callee.second) {
        Range range = TransformFromChar2IDE({ref.location.begin, ref.location.end});
        if (rangeIsFromCjo) {
            callHierarchyOutgoingCall.to.range = range;
            callHierarchyOutgoingCall.to.selectionRange = range;
            rangeIsFromCjo = false;
        }
        rangesSet.insert(range);
    }
    callHierarchyOutgoingCall.fromRanges.assign(rangesSet.begin(), rangesSet.end());
    (void) result.emplace_back(callHierarchyOutgoingCall);
}

void DealCaller(const std::pair<lsp::SymbolID, std::vector<lsp::Ref>>& caller, lsp::SymbolIndex *index,
                vector<CallHierarchyIncomingCall>& result)
{
    std::unordered_set<lsp::SymbolID> containerIds;
    (void)containerIds.insert(caller.first);
    lsp::LookupRequest lookUpReq{containerIds};
    lsp::Symbol containerSym;
    // find caller symbol by id
    index->Lookup(lookUpReq, [&containerSym](const lsp::Symbol &sym) {
        if (sym.location.IsZeroLoc() && sym.name != "init") {
            return;
        }
        containerSym = sym;
    });
    if (containerSym.signature.empty()) {
        return;
    }
    const CallHierarchyItem item = DeclToCallHierarchyItem(containerSym);
    CallHierarchyIncomingCall callHierarchyIncomingCall;
    callHierarchyIncomingCall.from = item;
    // deal ref incoming call
    std::set<Range> rangesSet;
    for (const auto &ref: caller.second) {
        Range range = TransformFromChar2IDE({ref.location.begin, ref.location.end});
        rangesSet.insert(range);
    }
    callHierarchyIncomingCall.fromRanges.assign(rangesSet.begin(), rangesSet.end());
    (void) result.emplace_back(callHierarchyIncomingCall);
}

void FindFuncDeclCaller(lsp::SymbolID id, lsp::SymbolIndex *index, vector <CallHierarchyIncomingCall> &result)
{
    if (id == lsp::INVALID_SYMBOL_ID) {
        return;
    }
    std::unordered_set<lsp::SymbolID> ids;
    lsp::SymbolID topId = id;
    index->FindRiddenUp(id, ids, topId);
    index->FindRiddenDown(topId, ids);
    (void)ids.insert(id);

    std::map<lsp::SymbolID, std::vector<lsp::Ref>> callers;
    const lsp::RefsRequest req{ids, lsp::RefKind::REFERENCE};
    // search refs
    index->Refs(req, [&callers, id](const lsp::Ref &ref) {
        if (ref.location.IsZeroLoc()) {
            return;
        }
        auto containerId = ref.container;
        if (containerId == id || containerId == lsp::INVALID_SYMBOL_ID) {
            return;
        }
        if (callers.find(containerId) != callers.end()) {
            // called more than twice
            callers[containerId].emplace_back(ref);
        } else {
            // first called
            callers.emplace(containerId, std::vector<lsp::Ref>{ref});
        }
    });

    for (auto &caller: callers) {
        DealCaller(caller, index, result);
    }
}

std::string CallHierarchyImpl::curFilePath;

void CallHierarchyImpl::FindCallHierarchyImpl(const ArkAST &ast, CallHierarchyItem &result,
                                                   Position pos)
{
    curFilePath = ast.file ? ast.file->filePath : "";
    Logger &logger = Logger::Instance();
    logger.LogMessage(MessageType::MSG_LOG, "CallHierarchyImpl::FindCallHierarchyImpl in.");
    // update pos fileID
    pos.fileID = ast.fileID;
    if (ast.file == nullptr) { return; }
    // adjust position from IDE to AST
    pos = PosFromIDE2Char(pos);
    PositionIDEToUTF8(ast.tokens, pos, *ast.file);
    std::vector<Symbol *> syms;
    std::vector<Ptr<Decl> > decls;
    const Ptr<Decl> decl = ast.GetDeclByPosition(pos, syms, decls, {true, false});
    if (!decl) { return; }
    result = DeclToCallHierarchyItem(dynamic_cast<FuncDecl*>(decl.get()));
    if (result.isKernel) {
        int index = ast.GetCurTokenByPos(pos, 0, static_cast<int>(ast.tokens.size()) - 1);
        if (index != -1) {
            const auto token = ast.tokens[index];
            const Range range = {token.Begin(),
                {token.End().line, token.Begin().column + CountUnicodeCharacters(token.Value())}};
            result.selectionRange = TransformFromChar2IDE(range);
            result.range = result.selectionRange;
        }
    }
}

void CallHierarchyImpl::FindOnOutgoingCallsImpl(vector <CallHierarchyOutgoingCall> &results,
                                                const CallHierarchyItem &callHierarchyItem)
{
    const auto declSymId = callHierarchyItem.symbolId;
    if (declSymId == lsp::INVALID_SYMBOL_ID) {
        return;
    }
    auto index = ark::CompilerCangjieProject::GetInstance()->GetIndex();
    if (!index) {
        return;
    }
    std::unordered_set<lsp::SymbolID> containerIds;
    (void)containerIds.insert(callHierarchyItem.symbolId);
    lsp::LookupRequest lookUpReq{containerIds};
    lsp::Symbol declSym;
    // find decl symbol by id
    index->Lookup(lookUpReq, [&declSym](const lsp::Symbol &sym) {
        if (sym.location.IsZeroLoc() && sym.name != "init" && !sym.isCjoSym) {
            return;
        }
        declSym = sym;
    });
    auto pkgName = declSym.scope;
    size_t pos = declSym.scope.find_last_of(':');
    if (pos != string::npos) {
        pkgName = declSym.scope.substr(0, pos);
    }

    // search callees
    std::map<lsp::SymbolID, std::vector<lsp::Ref>> callees;
    index->Callees(pkgName, declSymId, [&callees](const lsp::SymbolID &refDeclSymId, const lsp::Ref &ref) {
        if (ref.location.IsZeroLoc() || ref.location.fileUri.empty()) {
            return;
        }
        if (callees.find(refDeclSymId) != callees.end()) {
            // more than twice
            callees[refDeclSymId].emplace_back(ref);
        } else {
            // first time
            callees.emplace(refDeclSymId, std::vector<lsp::Ref>{ref});
        }
    });

    for (auto &callee: callees) {
        DealCallee(callee, index, results);
    }
}

void CallHierarchyImpl::FindOnIncomingCallsImpl(vector <CallHierarchyIncomingCall> &results,
                                                const CallHierarchyItem &callHierarchyItem)
{
    if (callHierarchyItem.symbolId == lsp::INVALID_SYMBOL_ID) {
        return;
    }
    auto index = CompilerCangjieProject::GetInstance()->GetIndex();
    if (!index) {
        return;
    }
    FindFuncDeclCaller(callHierarchyItem.symbolId, index, results);
}
}
