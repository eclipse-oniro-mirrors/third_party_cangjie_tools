// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "NormalCompleterByParse.h"
#include "OverrideCompleter.h"
#include "../../common/Utils.h"

using namespace Cangjie;
using namespace Cangjie::AST;
using namespace Cangjie::FileUtil;
using namespace Cangjie::Utils;

namespace {
auto CollectAliasMap(const File &file)
{
    std::unordered_map<std::string, std::string> fileAliasMap;
    for (const auto &import : file.imports) {
        if (!import->IsReExport() || import->IsImportSingle() || import->IsImportAll()) {
            continue;
        }

        if (import->IsImportAlias() && import->IsReExport()) {
            (void)fileAliasMap.insert_or_assign(import->content.identifier, import->content.aliasName);
        }

        // ImportMulti maybe include ImportAlias
        for (const auto &item : import->content.items) {
            if (item.kind == ImportKind::IMPORT_ALIAS) {
                (void)fileAliasMap.insert_or_assign(item.identifier, item.aliasName);
            }
        }
    }
    return fileAliasMap;
}

void SetAfterAT(const ark::ArkAST &input, ark::CompletionEnv& env, int curTokenIndex)
{
    if (curTokenIndex > 0 && curTokenIndex < input.tokens.size()) {
        auto preToken = input.tokens[static_cast<int>(curTokenIndex - 1)];
        if (preToken == "@") {
            env.isAfterAT = true;
        }
    }
}

std::unordered_set<TokenKind> overrideFlag = {TokenKind::RCURL, TokenKind::IDENTIFIER, TokenKind::INTEGER_LITERAL};
}

namespace ark {
bool NormalCompleterByParse::Complete(const ArkAST &input, const Position pos)
{
    CompletionEnv env;
    if (!input.file || !input.file->curPackage) {
        return true;
    }
    auto curModule = SplitFullPackage(input.file->curPackage->fullPackageName).first;
    env.SetSyscap(curModule);
    env.prefix = prefix;
    env.curPkgName = input.file->curPackage->fullPackageName;
    env.parserAst = &input;
    env.cache = input.semaCache;

    int curTokenIndex = input.GetCurTokenByPos(pos, 0, static_cast<int>(input.tokens.size()) - 1);
    SetAfterAT(input, env, curTokenIndex);

    // Complete all imported and accessible top-level decl.
    pkgAliasMap.merge(::CollectAliasMap(*input.file));
    for (auto const &declSet : importManager->GetImportedDecls(*input.file)) {
        // The first element is real imported name
        for (const auto &decl : declSet.second) {
            result.importDeclsSymID.insert(CompletionEnv::GetDeclSymbolID(*decl));
            if (decl->identifier == declSet.first) {
                env.CompleteNode(decl.get());
            } else {
                env.CompleteAliasItem(decl.get(), declSet.first);
            }
        }
    }

    // Complete all top-decls in files of current package, which not includes import spec.
    auto decls = CompleteCurrentPackages(input, pos, env);
    Ptr<Decl> topLevelDecl = decls.first;
    // No code completion needed in these declaration position
    bool isMinDecl = false;
    bool isInPrimaryCtor = false;
    std::unordered_set<ASTKind> trustList = {ASTKind::CLASS_DECL, ASTKind::ENUM_DECL, ASTKind::VAR_DECL,
        ASTKind::STRUCT_DECL, ASTKind::INTERFACE_DECL, ASTKind::FUNC_DECL, ASTKind::MACRO_DECL, ASTKind::FUNC_PARAM,
        ASTKind::TYPE_ALIAS_DECL};

    Walker(
        topLevelDecl,
        [&](auto node) {
            auto decl = DynamicCast<Decl>(node);
            if (node->astKind == ASTKind::PRIMARY_CTOR_DECL) {
                isInPrimaryCtor = true;
            }
            if (decl && decl->identifier.Begin() <= pos && pos <= decl->identifier.End()) {
                bool isParamFuncInPrimary = node->astKind == ASTKind::FUNC_PARAM && isInPrimaryCtor;
                if (trustList.find(node->astKind) != trustList.end() && !isParamFuncInPrimary) {
                    isMinDecl = true;
                }
            }
            return VisitAction::WALK_CHILDREN;
        },
        [&](auto node) {
            if (node->astKind == ASTKind::PRIMARY_CTOR_DECL) {
                isInPrimaryCtor = false;
            }
            return VisitAction::WALK_CHILDREN;
        })
        .Walk();

    auto kind = FindPreFirstValidTokenKind(input, curTokenIndex);
    Ptr<Decl> decl = nullptr;
    bool isInclass = CheckIfOverrideComplete(topLevelDecl, decl, pos, kind);
    Ptr<Decl> semaCacheDecl = decls.second;
    if (isInclass && semaCacheDecl) {
        OverrideCompleter overrideCompleter(semaCacheDecl, env.prefix);
        if (overrideCompleter.SetCompletionConfig(decl, pos)) {
            overrideCompleter.FindOvrrideFunction();
            auto items = overrideCompleter.ExportItems();
            for (auto& item : items) {
                result.completions.push_back(item);
            }
        }
    }

    if (isMinDecl) {
        return false;
    }

    if (!isInclass) {
        // Complete own scope decl.
        env.visitedNodes.clear();
        env.DeepComplete(topLevelDecl, pos);
        env.visitedNodes.clear();
        env.SemaCacheComplete(semaCacheDecl, pos);
    }
    AddImportPkgDecl(input, env);
    env.OutputResult(result);
    return true;
}

void NormalCompleterByParse::AddImportPkgDecl(const ArkAST &input, CompletionEnv &env)
{
    std::vector<Ptr<PackageDecl>> curImportedPackages = {};
    bool isValid = input.semaCache && input.semaCache->packageInstance &&
                   input.semaCache->packageInstance->ctx &&
                   input.semaCache->packageInstance->ctx->curPackage;
    if (isValid) {
        curImportedPackages = importManager->GetCurImportedPackages(env.curPkgName);
    }
    // Complete imported packageName for fully qualified name reference.
    // Include cjo import and source import.
    auto curModule = SplitFullPackage(env.curPkgName).first;
    for (auto &im : input.file->imports) {
        if (im->IsImportSingle()) {
            auto fullPackageName = im->content.ToString();
            if (!CompilerCangjieProject::GetInstance()->Denoising(fullPackageName).empty() ||
                CompilerCangjieProject::GetInstance()->IsCurModuleCjoDep(curModule, fullPackageName)) {
                env.AccessibleByString(im->content.identifier, "packageName");
            }
        } else if (im->IsImportAlias()) {
            std::string candidate;
            for (auto &prefix : im->content.prefixPaths) {
                candidate += prefix + CONSTANTS::DOT;
            }
            candidate += im->content.identifier;
            if (!CompilerCangjieProject::GetInstance()->Denoising(candidate).empty() ||
                CompilerCangjieProject::GetInstance()->IsCurModuleCjoDep(curModule, candidate)) {
                env.AccessibleByString(im->content.aliasName, "packageName");
            }
        }
    }
    for (const auto &it : curImportedPackages) {
        if (!it || it->identifier.Empty()) {
            continue;
        }
        if (isValid) {
            FillingDeclsInPackage(it->identifier, env, it);
        }
    }
}

std::pair<Ptr<Decl>, Ptr<Decl>> NormalCompleterByParse::CompleteCurrentPackages(const ArkAST &input, const Position pos,
                                                                                CompletionEnv &env)
{
    // parse info fullPackageName is "", so we need a flag to complete init Func
    env.isInPackage = true;
    Ptr<Decl> innerDecl = nullptr;
    Ptr<Decl> semaCacheDecl = nullptr;
    bool invalid = !input.file || !input.file->curPackage;
    if (invalid) {
        return {innerDecl, semaCacheDecl};
    }
    for (auto &file : input.file->curPackage->files) {
        if (!file) {
            continue;
        }
        bool sameFile = file->fileHash == input.file->fileHash;
        for (auto &decl : file->decls) {
            if (!sameFile && decl->TestAttr(Attribute::PRIVATE)) {
                continue;
            }
            if (!DealDeclInCurrentPackage(decl.get(), env)) {
                continue;
            }
            if (sameFile && decl->GetBegin() <= pos &&
                pos <= decl->GetEnd()) {
                innerDecl = decl.get();
            }
        }
    }
    semaCacheDecl = CompleteCurrentPackagesOnSema(input, pos, env);
    env.isInPackage = false;
    return {innerDecl, semaCacheDecl};
}

Ptr<Decl> NormalCompleterByParse::CompleteCurrentPackagesOnSema(const ArkAST &input, const Position pos,
                                                                CompletionEnv &env)
{
    Ptr<Decl> semaCacheDecl = nullptr;
    if (!input.semaCache || !input.semaCache->file || !input.semaCache->file->curPackage ||
        input.semaCache->file->curPackage->files.empty()) {
        env.isInPackage = false;
        return semaCacheDecl;
    }
    // add Sema topLevel completeItem
    for (auto &file : input.semaCache->file->curPackage->files) {
        if (!file || !file.get() || file->decls.empty()) {
            continue;
        }
        bool sameFile = file->fileHash == input.file->fileHash;
        for (auto &decl : file->decls) {
            if (decl && decl->astKind != Cangjie::AST::ASTKind::EXTEND_DECL &&
                !(!sameFile && decl->TestAttr(Attribute::PRIVATE))) {
                env.CompleteNode(decl.get(), false, false, true);
                if (sameFile && decl->GetBegin() <= pos &&
                    pos <= decl->GetEnd()) {
                    semaCacheDecl = decl.get();
                }
            }
        }
    }
    return semaCacheDecl;
}

bool NormalCompleterByParse::CheckCompletionInParse(Ptr<Decl> decl)
{
    if (!decl) {
        return false;
    }
    if (decl->astKind == Cangjie::AST::ASTKind::EXTEND_DECL) {
        return false;
    }
    if (decl->astKind == ASTKind::MACRO_EXPAND_DECL && decl->identifier == "APILevel") {
        return false;
    }
    return true;
}

bool NormalCompleterByParse::CheckIfOverrideComplete(Ptr<Decl> topLevelDecl, Ptr<Decl>& decl,
                                                    const Position& pos, TokenKind kind)
{
    if (!Options::GetInstance().IsOptionSet("test") && !MessageHeaderEndOfLine::GetIsDeveco()) {
        return false;
    }
    auto classLikeDecl = DynamicCast<ClassLikeDecl>(topLevelDecl);
    auto structDecl = DynamicCast<StructDecl>(topLevelDecl);
    auto enumDecl = DynamicCast<EnumDecl>(topLevelDecl);
    if (!(classLikeDecl || structDecl || enumDecl)) {
        return false;
    }
    if (topLevelDecl->GetMemberDecls().empty() || (topLevelDecl->GetMemberDecls().front() &&
        topLevelDecl->GetMemberDecls().front()->GetBegin() > pos)) {
        return enumDecl ? false : kind == TokenKind::LCURL;
    }
    for (auto& memberDecl : topLevelDecl->GetMemberDecls()) {
        if (memberDecl == nullptr) {
            continue;
        }
        if (memberDecl->GetBegin() <= pos && pos <= memberDecl->GetEnd()) {
            decl = memberDecl.get();
            if (enumDecl) {
                return (kind == TokenKind::FUNC && memberDecl->astKind == ASTKind::FUNC_DECL) ||
                       (memberDecl->astKind == ASTKind::VAR_DECL);
            }
            return (kind == TokenKind::FUNC && memberDecl->astKind == ASTKind::FUNC_DECL);
        }
    }
    if (decl == nullptr) {
        return overrideFlag.find(kind) != overrideFlag.end();
    }
    return true;
}

bool NormalCompleterByParse::DealDeclInCurrentPackage(Ptr<Decl> decl, CompletionEnv &env)
{
    if (!decl) {
        return false;
    }
    if (CheckCompletionInParse(decl)) {
        env.CompleteNode(decl);
    }
    auto declName = decl->identifier;
    if (decl->astKind == Cangjie::AST::ASTKind::EXTEND_DECL) {
        auto *extendDecl = dynamic_cast<ExtendDecl *>(decl.get());
        if (!extendDecl) {
            return false;
        }
        declName = extendDecl->extendedType->ToString();
        auto posOfLeft = declName.Val().find_first_of('<');
        if (posOfLeft != std::string::npos) {
            declName = declName.Val().substr(0, posOfLeft);
        }
    }
    // update idMap && importedExternalDeclMap
    if (env.idMap.find(declName) != env.idMap.end()) {
        env.idMap[declName].push_back(decl);
        env.importedExternalDeclMap[declName] = std::make_pair(decl, false);
    } else {
        std::vector<Ptr<Decl>> temp = {decl};
        env.idMap[declName] = temp;
        env.importedExternalDeclMap[declName] = std::make_pair(decl, false);
    }
    return true;
}

void NormalCompleterByParse::FillingDeclsInPackage(const std::string &packageName,
                                                   CompletionEnv &env,
                                                   Ptr<const PackageDecl> pkgDecl)
{
    // deal external import pkgDecl
    if (pkgDecl == nullptr) {
        pkgDecl = importManager->GetPackageDecl(packageName);
        if (!pkgDecl) {
            env.OutputResult(result);
            return;
        }
    }
    if (pkgDecl == nullptr || pkgDecl->srcPackage == nullptr ||
        pkgDecl->srcPackage->files.empty()) {
        return;
    }
}

void NormalCompleterByParse::CompleteModuleName(const std::string &curModule)
{
    CompletionEnv env;
    for (const auto &item : Cangjie::LSPCompilerInstance::cjoLibraryMap) {
        env.AccessibleByString(item.first, "moduleName");
    }
    for (const auto &item : CompilerCangjieProject::GetInstance()->GetOneModuleDeps(curModule)) {
        if (!IsFullPackageName(item)) {
            continue;
        }
        env.AccessibleByString(SplitFullPackage(item).first, "moduleName");
    }
    env.OutputResult(result);
}

void NormalCompleterByParse::CompletePackageSpec(const ArkAST &input)
{
    if (!input.file || input.file->filePath.empty()) {
        return;
    }
    CompletionEnv env;
    std::string path = Normalize(input.file->filePath);
    std::string fullPkgName = CompilerCangjieProject::GetInstance()->GetFullPkgName(path);
    if (IsFromCIMapNotInSrc(fullPkgName)) {
        auto workspace = CompilerCangjieProject::GetInstance()->GetWorkSpace();
        std::string relativeDirName = fullPkgName;
        size_t found = fullPkgName.find(workspace);
        if (found != std::string::npos) {
            relativeDirName = relativeDirName.substr(found + workspace.length());
            // Remove leading separator if present
            if (!relativeDirName.empty() && (relativeDirName[0] == '/' || relativeDirName[0] == '\\')) {
                relativeDirName = relativeDirName.substr(1);
            }
        }
#ifdef _WIN32
        std::replace(relativeDirName.begin(), relativeDirName.end(), '\\', '.');
#else
        std::replace(relativeDirName.begin(), relativeDirName.end(), '/', '.');
#endif
        fullPkgName = relativeDirName;
    }
    env.AccessibleByString(fullPkgName, "packageName");
    env.OutputResult(result);
}

} // namespace ark
