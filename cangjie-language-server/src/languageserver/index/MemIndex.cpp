// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "MemIndex.h"
#include "../CompilerCangjieProject.h"

namespace ark {
namespace lsp {
PackageRelation GetPackageRelation(const std::string &srcFullPackageName,
                                   const std::string &targetFullPackageName)
{
    if (srcFullPackageName.length() < targetFullPackageName.length()
        && targetFullPackageName.find(srcFullPackageName) == 0
        && targetFullPackageName[srcFullPackageName.length()] == '.') {
        return PackageRelation::PARENT;
    }
    if (srcFullPackageName.length() > targetFullPackageName.length()
        && srcFullPackageName.find(targetFullPackageName) == 0
        && srcFullPackageName[targetFullPackageName.length()] == '.') {
        return PackageRelation::CHILD;
    }
    auto srcRoot = Cangjie::Utils::SplitQualifiedName(srcFullPackageName).front();
    auto targetRoot = Cangjie::Utils::SplitQualifiedName(targetFullPackageName).front();
    return srcRoot == targetRoot ? PackageRelation::SAME_MODULE : PackageRelation::NONE;
}

void MemIndex::FuzzyFind(const FuzzyFindRequest &req, std::function<void(const Symbol &)> callback) const
{
    for (const auto &pkgSyms : pkgSymsMap) {
        for (const auto &sym : pkgSyms.second) {
            if (!IsMatchingCompletion(req.query, sym.name)) {
                continue;
            }
            callback(sym);
        }
    }
}

void MemIndex::Lookup(const LookupRequest &req, std::function<void(const Symbol &)> callback) const
{
    for (const auto &id : req.ids) {
        for (const auto &pkgSyms : pkgSymsMap) {
            for (const auto &sym : pkgSyms.second) {
                if (id == sym.id) {
                    callback(sym);
                }
            }
        }
    }
}

void MemIndex::Refs(const RefsRequest &req, std::function<void(const Ref &)> callback) const
{
    for (const auto &id : req.ids) {
        for (const auto &pkgRefs : pkgRefsMap) {
            auto symRef = pkgRefs.second.find(id);
            if (symRef == pkgRefs.second.end()) {
                continue;
            }
            for (const auto &sym : symRef->second) {
                if (!static_cast<int>(req.filter & sym.kind)) {
                    continue;
                }
                callback(sym);
            }
        }
    }
}

void MemIndex::RefsFindReference(const RefsRequest &req,
    Ref &definition, std::function<void(const Ref &)> callback) const
{
    for (const auto &id : req.ids) {
        for (const auto &pkgRefs : pkgRefsMap) {
            auto symRef = pkgRefs.second.find(id);
            if (symRef == pkgRefs.second.end()) {
                continue;
            }
            for (const auto &sym : symRef->second) {
                if (sym.kind == RefKind::DEFINITION) {
                    definition = sym;
                }
                if (!static_cast<int>(req.filter & sym.kind)) {
                    continue;
                }
                callback(sym);
            }
        }
    }
}

void MemIndex::Callees(const std::string &pkgName, const SymbolID &declId,
    std::function<void(const SymbolID &, const Ref &)> callback) const
{
    auto pkgSymRefs = pkgRefsMap.find(pkgName);
    if (pkgSymRefs == pkgRefsMap.end()) {
        return;
    }
    for (const auto& symbolRef : pkgSymRefs->second) {
        auto declSymId = symbolRef.first;
        for (const auto& ref : symbolRef.second) {
            if (ref.container != declId) {
                continue;
            }
            callback(declSymId, ref);
        }
    }
}

void MemIndex::Relations(const RelationsRequest &req, std::function<void(const Relation &)> callback) const
{
    for (const auto &pkgRelations : pkgRelationsMap) {
        for (const auto &spo : pkgRelations.second) {
            if (spo.subject == req.id && spo.predicate == req.predicate) {
                callback(spo);
            }
            if (spo.object == req.id && spo.predicate == req.predicate) {
                callback(spo);
            }
        }
    }
}

Symbol MemIndex::GetAimSymbol(const Decl& decl)
{
    auto pkgName = decl.fullPackageName;
    auto symbolID = GetSymbolId(decl);
    if (pkgSymsMap.find(pkgName) == pkgSymsMap.end()) { return {}; }
    for (auto &sym: pkgSymsMap[pkgName]) {
        if (sym.id == symbolID) {
            return sym;
        }
    }
    return {};
}

void MemIndex::FindImportSymsOnCompletion(
    const std::pair<std::unordered_set<SymbolID>, std::unordered_set<SymbolID>>& filterSyms,
    const std::string &curPkgName, const std::string &curModule, const std::string &prefix,
    std::function<void(const std::string &, const Symbol &, const CompletionItem &)> callback)
{
    if (Options::GetInstance().IsOptionSet("test")) {
        return;
    }
    const auto &normalCompleteSyms  = filterSyms.first;
    const auto &importDeclSyms  = filterSyms.second;
    size_t normalCompleteCount = 0;
    size_t importDeclCount = 0;
    std::unordered_set<std::string> curModuleDeps =
        CompilerCangjieProject::GetInstance()->GetOneModuleDirectDeps(curModule);
    for (const auto &pkgSyms : pkgSymsMap) {
        // filter curPackage sym
        if (curPkgName == pkgSyms.first) {
            continue;
        }
        auto relation = GetPackageRelation(curPkgName, pkgSyms.first);
        for (const auto &sym : pkgSyms.second) {
            // filter symbols that not dependent by curModule
            if (!sym.isCjoSym && !curModuleDeps.count(sym.curModule)) {
                continue;
            }
            // filter symbols that are already in the NormalComplete
            if (sym.id != 0 && normalCompleteCount < normalCompleteSyms.size() && normalCompleteSyms.count(sym.id)) {
                normalCompleteCount++;
                continue;
            }
            // filter imported syms
            if (importDeclCount < importDeclSyms.size() && importDeclSyms.count(sym.id)) {
                importDeclCount++;
                continue;
            }
            // filter not top decl
            if (sym.scope.find(':') != std::string::npos) {
                continue;
            }
            // filter by modifier
            bool isAccessible =
                sym.modifier == ark::lsp::Modifier::PUBLIC
                || (relation == PackageRelation::CHILD && (sym.modifier == ark::lsp::Modifier::INTERNAL
                                                          || sym.modifier == ark::lsp::Modifier::PROTECTED))
                || (relation == PackageRelation::SAME_MODULE && sym.modifier == ark::lsp::Modifier::PROTECTED)
                || (relation == PackageRelation::PARENT && sym.modifier == ark::lsp::Modifier::PROTECTED);
            if (isAccessible) {
                for (const auto &completionItem : sym.completionItems) {
                    callback(pkgSyms.first, sym, completionItem);
                }
            }
        }
    }
}

void MemIndex::FindExtendSymsOnCompletion(const SymbolID &dotCompleteSym,
    const std::unordered_set<SymbolID> &visibleMembers,
    const std::string &curPkgName, const std::string &curModule,
    const std::function<void(const std::string &, const std::string &,
        const Symbol &, const CompletionItem &)>& callback)
{
    if (Options::GetInstance().IsOptionSet("test")) {
        return;
    }
    std::unordered_set<std::string> curModuleDeps =
        CompilerCangjieProject::GetInstance()->GetOneModuleDirectDeps(curModule);
    for (const auto &extendSyms : pkgExtendsMap) {
        // filter curPackage sym
        std::string pkgName = extendSyms.first;
        if (curPkgName == pkgName) {
            continue;
        }
        auto relation = GetPackageRelation(curPkgName, pkgName);
        auto syms = pkgSymsMap[pkgName];
        std::unordered_map<SymbolID, Symbol> symMap;
        for (const auto& sym : syms) {
            symMap.insert_or_assign(sym.id, sym);
        }
        auto checkAccessible = [&relation](const Modifier& modifier) -> bool {
            bool isAccessible =
                modifier == Modifier::PUBLIC
                || (relation == PackageRelation::CHILD && (modifier == Modifier::INTERNAL
                                                        || modifier == Modifier::PROTECTED))
                || (relation == PackageRelation::SAME_MODULE &&
                    modifier == Modifier::PROTECTED)
                || (relation == PackageRelation::PARENT && modifier == Modifier::PROTECTED);
            return isAccessible;
        };
        for (const auto &extendSlab : extendSyms.second) {
            if (extendSlab.first == dotCompleteSym) {
                for (const auto& symbol : extendSlab.second) {
                    if (visibleMembers.find(symbol.id) != visibleMembers.end()) {
                        continue;
                    }
                    auto sym = symMap[symbol.id];
                    // filter symbols that not dependent by curModule
                    if (!sym.isCjoSym && !curModuleDeps.count(sym.curModule)) {
                        continue;
                    }
                    // filter by modifier
                    if (checkAccessible(symbol.modifier) && checkAccessible(sym.modifier)) {
                        for (const auto &completionItem : sym.completionItems) {
                            callback(pkgName, symbol.interfaceName, sym, completionItem);
                        }
                    }
                }
                break;
            }
        }
    }
}

void MemIndex::FindImportSymsOnQuickFix(const std::string &curPkgName, const std::string &curModule,
    const std::unordered_set<SymbolID> &importDeclSyms, const std::string& identifier,
    const std::function<void(const std::string &, const Symbol &)>& callback)
{
    size_t importDeclCount = 0;
    std::unordered_set<std::string> curModuleDeps =
        CompilerCangjieProject::GetInstance()->GetOneModuleDirectDeps(curModule);
    for (const auto &pkgSyms : pkgSymsMap) {
        // filter curPackage sym
        if (curPkgName == pkgSyms.first) {
            continue;
        }
        auto relation = GetPackageRelation(curPkgName, pkgSyms.first);
        for (const auto &sym : pkgSyms.second) {
            // filter diff name
            if (sym.name != identifier) {
                continue;
            }
            // filter symbols that not dependent by curModule
            if (!sym.isCjoSym && !curModuleDeps.count(sym.curModule)) {
                continue;
            }
            // filter imported syms
            if (importDeclCount < importDeclSyms.size() && importDeclSyms.count(sym.id)) {
                importDeclCount++;
                continue;
            }
            // filter not top decl
            if (sym.scope.find(':') != std::string::npos) {
                continue;
            }
            // filter by modifier
            bool isAccessible =
                sym.modifier == ark::lsp::Modifier::PUBLIC
                || (relation == PackageRelation::CHILD && (sym.modifier == ark::lsp::Modifier::INTERNAL
                                                              || sym.modifier == ark::lsp::Modifier::PROTECTED))
                || (relation == PackageRelation::SAME_MODULE && sym.modifier == ark::lsp::Modifier::PROTECTED)
                || (relation == PackageRelation::PARENT && sym.modifier == ark::lsp::Modifier::PROTECTED);
            if (isAccessible) {
                callback(pkgSyms.first, sym);
            }
        }
    }
}

void MemIndex::FindComment(const Symbol &sym, std::vector<std::string> &comments)
{
    const auto [leadCommentGroup, innerCommentGroup, trailCommentGroup] = sym.comments;
    for (const auto &innerComment : innerCommentGroup) {
        for (const auto &comment : innerComment.cms) {
            comments.push_back(comment.info.Value());
        }
    }
    for (auto &leadComment : leadCommentGroup) {
        for (const auto &comment : leadComment.cms) {
            comments.push_back(comment.info.Value());
        }
    }
    for (auto &trailComment : trailCommentGroup) {
        for (const auto &comment : trailComment.cms) {
            comments.push_back(comment.info.Value());
        }
    }
}

void MemIndex::FindCrossSymbolByName(const std::string &packageName, const std::string &symName, bool isComebined,
    const std::function<void(const CrossSymbol &)> &callback)
{
    std::unordered_set<std::string> targetPackageSet;
    targetPackageSet.insert(packageName);
    if (isComebined) {
        const auto pkgMap = CompilerCangjieProject::GetInstance()->GetFullPkgNameToPathMap();
        for (auto &item : pkgMap) {
            std::string pkgName = item.first;
            if (pkgName.find(packageName) != std::string::npos) {
                targetPackageSet.insert(pkgName);
            }
        }
    }
    for (const auto &pkgName : targetPackageSet) {
        if (pkgCrossSymsMap.find(pkgName) == pkgCrossSymsMap.end()) {
            continue;
        }
        for (const auto &crs : pkgCrossSymsMap[pkgName]) {
            if (crs.name != symName) {
                continue;
            }
            callback(crs);
        }
    }
}
} // namespace lsp
} // namespace ark