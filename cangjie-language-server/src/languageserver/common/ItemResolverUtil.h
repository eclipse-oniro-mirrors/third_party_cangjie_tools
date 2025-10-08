// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#ifndef LSPSERVER_ITEMRESOLVERUTIL_H
#define LSPSERVER_ITEMRESOLVERUTIL_H

#include "cangjie/AST/ASTContext.h"
#include "cangjie/AST/Types.h"
#include "../../json-rpc/CompletionType.h"
#include "../../json-rpc/Protocol.h"
#include "cangjie/AST/Node.h"
#include "cangjie/Basic/SourceManager.h"

namespace ark {
class ItemResolverUtil {
public:
    static std::string ResolveNameByNode(Cangjie::AST::Node &node);

    static CompletionItemKind ResolveKindByNode(Cangjie::AST::Node &node);

    static CompletionItemKind ResolveKindByASTKind(Cangjie::AST::ASTKind &astKind);

    static std::string ResolveDetailByNode(Cangjie::AST::Node &node, Cangjie::SourceManager *sourceManager = nullptr);

    static std::string ResolveSourceByNode(Ptr<Cangjie::AST::Decl> decl, std::string path);

    static std::string ResolveSignatureByNode(const Cangjie::AST::Node &node,
                                              Cangjie::SourceManager *sourceManager = nullptr,
                                              bool isCompletionInsert = false,
                                              bool isAfterAT = false);

    static std::string ResolveInsertByNode(const Cangjie::AST::Node &primaryCtorDecl,
                                           Cangjie::SourceManager *sourceManager = nullptr, bool isAfterAT = false);

    static Ptr<Cangjie::AST::Decl> GetDeclByTy(Cangjie::AST::Ty *enumTy);

    static std::string GetGenericInsertByDecl(Ptr<Cangjie::AST::Generic> genericDecl);

    static void ResolveFuncParams(std::string &detail,
                                  const std::vector<OwnedPtr<Cangjie::AST::FuncParamList>> &paramLists,
                                  bool isEnumConstruct = false, Cangjie::SourceManager *sourceManager = nullptr,
                                  const std::string &filePath = "");

    static void ResolveMacroParams(std::string &detail,
        const std::vector<OwnedPtr<Cangjie::AST::FuncParamList>> &paramLists);

private:
    static std::string FetchTypeString(const Cangjie::AST::Type &type);

    template <typename T>
    static std::string GetGenericString(const T &t);

    static void GetInitializerInfo(std::string &detail, const Cangjie::AST::VarDecl &decl,
                                   Cangjie::SourceManager *sourceManager, bool hasType);

    static void GetDetailByTy(const Cangjie::AST::Ty *ty, std::string &detail, bool isLambda = false);

    static void ResolveVarDeclDetail(std::string &detail, const Cangjie::AST::VarDecl &decl,
                                     Cangjie::SourceManager *sourceManager = nullptr);
    
    static void ResolveFuncDeclQuickLook(std::string &detail, const Cangjie::AST::FuncDecl &decl,
                                         Cangjie::SourceManager *sourceManager = nullptr);

    static void ResolveFuncDeclDetail(std::string &detail, const Cangjie::AST::FuncDecl &decl,
                                      Cangjie::SourceManager *sourceManager = nullptr);

    static void ResolvePrimaryCtorDeclDetail(std::string &detail, const Cangjie::AST::PrimaryCtorDecl &decl,
                                             Cangjie::SourceManager *sourceManager = nullptr);

    static void ResolvePrimaryCtorDeclSignature(std::string &detail, const Cangjie::AST::PrimaryCtorDecl &decl,
                                                Cangjie::SourceManager *sourceManager = nullptr,
                                                bool isAfterAT = false);

    static void ResolvePatternSignature(std::string &signature, Ptr<Cangjie::AST::Pattern> pattern,
                                        Cangjie::SourceManager *sourceManager = nullptr);

    static void ResolvePatternDetail(std::string &detail, Ptr<Cangjie::AST::Pattern> pattern,
                                     Cangjie::SourceManager *sourceManager = nullptr);

    static void ResolveMacroDeclDetail(std::string &detail, const Cangjie::AST::MacroDecl &decl,
                                       Cangjie::SourceManager *sourceManager = nullptr);

    static void ResolveMacroDeclSignature(std::string &detail, const Cangjie::AST::MacroDecl &decl,
                                          Cangjie::SourceManager *sourceManager = nullptr);

    static void ResolveClassDeclDetail(std::string &detail, Cangjie::AST::ClassDecl &decl,
                                       Cangjie::SourceManager *sourceManager = nullptr);

    static void ResolveInterfaceDeclDetail(std::string &detail, Cangjie::AST::InterfaceDecl &decl,
                                           Cangjie::SourceManager *sourceManager = nullptr);

    static void ResolveEnumDeclDetail(std::string &detail, const Cangjie::AST::EnumDecl &decl,
                                      Cangjie::SourceManager *sourceManager = nullptr);

    static std::string GetGenericParamByDecl(Ptr<Cangjie::AST::Generic> genericDecl);

    static int AddGenericInsertByDecl(std::string &detail, Ptr<Cangjie::AST::Generic> genericDecl);

    static void ResolveFuncDeclSignature(std::string &detail, const Cangjie::AST::FuncDecl &decl,
                                         Cangjie::SourceManager *sourceManager = nullptr,
                                         bool isCompletionInsert = false,
                                         bool isAfterAT = false);

    template<typename T>
    static void ResolveFuncLikeDeclInsert(std::string &detail,
                                          const T &decl,
                                          Cangjie::SourceManager *sourceManager = nullptr,
                                          bool isAfterAT = false);

    static void ResolveStructDeclDetail(std::string &detail, const Cangjie::AST::StructDecl &decl);

    static void ResolveGenericParamDeclDetail(std::string &detail, const Cangjie::AST::GenericParamDecl &decl);

    static void ResolveTypeAliasDetail(std::string &detail, const Cangjie::AST::TypeAliasDecl &decl,
                                       Cangjie::SourceManager *sourceManager = nullptr);

    static void ResolveBuiltInDeclDetail(std::string &detail, const Cangjie::AST::BuiltInDecl &decl);

    static void DealTypeDetail(std::string &detail, Ptr<Cangjie::AST::Type> type, const std::string &filePath,
                               Cangjie::SourceManager *sourceManager = nullptr);
 
    static void ResolveMacroParamsInsert(std::string &detail,
                                         const std::vector<OwnedPtr<Cangjie::AST::FuncParamList>> &paramLists);

    static bool IsCustomAnnotation(const Cangjie::AST::Decl &decl);

    static const int detailMaxLen = 256;
};
} // namespace ark

#endif // LSPSERVER_ITEMRESOLVERUTIL_H
