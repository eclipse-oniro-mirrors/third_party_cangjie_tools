// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "StructuralRuleGITF01.h"

namespace Cangjie::CodeCheck {
using namespace Cangjie;
using namespace AST;
using namespace Meta;

void StructuralRuleGITF01::HasModifiedVar(
    Ptr<AST::AssignExpr> pAssignExpr, std::unordered_set<Ptr<AST::Decl>>& varDecls, const Ptr<AST::Decl> parentFuncDecl)
{
    Ptr<const Expr> baseExpr = pAssignExpr->leftValue;
    while (baseExpr != nullptr) {
        auto target = baseExpr->GetTarget();
        if (Is<const RefExpr*>(baseExpr)) {
            if (varDecls.find(target) != varDecls.cend()) {
                Diagnose(parentFuncDecl->begin, parentFuncDecl->end,
                    CodeCheckDiagKind::G_ITF_01_should_use_mut_modifier, parentFuncDecl->identifier.Val());
            }
            break;
        } else if (auto ma = DynamicCast<const MemberAccess*>(baseExpr); ma) {
            if (ma->baseExpr && ma->baseExpr->ty->IsClassLike()) {
                // don't check member access of field of class like type, unless it is a member access of this
                // e.g. let T be a class type with field v
                // this.a = T() // this is a mutation to this
                // this.a.v = T() // this is not a mutation, because a is of class type
                break;
            }
            if (auto re = DynamicCast<RefExpr*>(ma->baseExpr.get());
                re && re->isThis && varDecls.find(target) != varDecls.cend()) {
                Diagnose(parentFuncDecl->begin, parentFuncDecl->end,
                    CodeCheckDiagKind::G_ITF_01_should_use_mut_modifier, parentFuncDecl->identifier.Val());
                break;
            }
            baseExpr = ma->baseExpr.get();
        } else {
            break;
        }
    }
}

void StructuralRuleGITF01::AnalysisFuncDecl(
    Ptr<AST::FuncDecl> pFuncDecl, std::unordered_set<Ptr<AST::Decl>>& varDecls, const Ptr<AST::Decl> parentFuncDecl)
{
    if (!pFuncDecl->funcBody || !pFuncDecl->funcBody->body) {
        return;
    }

    for (auto& node : pFuncDecl->funcBody->body->body) {
        if (auto pAssignExpr = DynamicCast<AssignExpr*>(node.get()); pAssignExpr) {
            HasModifiedVar(pAssignExpr, varDecls, parentFuncDecl);
        }

        if (auto pIncOrDecExpr = DynamicCast<IncOrDecExpr*>(node.get()); pIncOrDecExpr) {
            if (auto pAssignExpr = DynamicCast<AssignExpr*>(pIncOrDecExpr->desugarExpr.get()); pAssignExpr) {
                HasModifiedVar(pAssignExpr, varDecls, parentFuncDecl);
            }
        }
    }
}

void StructuralRuleGITF01::CoverDeclToFuncDecl(TypeManager* typeManager, Ptr<AST::Decl> pDecl,
    std::unordered_set<Ptr<AST::Decl>>& varDecls, std::unordered_set<Ptr<AST::Decl>>& funcDecls)
{
    if (auto childfd = DynamicCast<FuncDecl*>(pDecl.get()); childfd) {
        for (auto& parentFuncDecl : funcDecls) {
            if (typeManager->PairIsOverrideOrImpl(*childfd, *parentFuncDecl)) {
                AnalysisFuncDecl(childfd, varDecls, parentFuncDecl);
            }
        }
    }
}

void StructuralRuleGITF01::AnalysisFuncDeclsOfExtendDecl(TypeManager* typeManager, Ptr<AST::ExtendDecl> pExtendDecl,
    std::unordered_set<Ptr<AST::Decl>>& varDecls, std::unordered_set<Ptr<AST::Decl>>& funcDecls)
{
    for (auto& member : pExtendDecl->members) {
        CoverDeclToFuncDecl(typeManager, member.get(), varDecls, funcDecls);
    }
}

void StructuralRuleGITF01::AnalysisFuncDeclsOfStructDecl(TypeManager* typeManager, Ptr<AST::StructDecl> pStructDecl,
    std::unordered_set<Ptr<AST::Decl>>& varDecls, std::unordered_set<Ptr<AST::Decl>>& funcDecls)
{
    if (!pStructDecl->body) {
        return;
    }
    for (auto& sdDecl : pStructDecl->body->decls) {
        CoverDeclToFuncDecl(typeManager, sdDecl.get(), varDecls, funcDecls);
    }
}

void StructuralRuleGITF01::CollectVarDeclsOfStructDecl(
    Ptr<AST::StructDecl> pStructDecl, std::unordered_set<Ptr<AST::Decl>>& varDecls)
{
    if (!pStructDecl->body) {
        return;
    }

    for (auto& sdDecl : pStructDecl->body->decls) {
        if (auto vd = DynamicCast<VarDecl*>(sdDecl.get());
            vd && !vd->TestAttr(Attribute::STATIC) && Ty::IsTyCorrect(vd->ty) && !vd->ty->IsArray()) {
            varDecls.emplace(vd);
        }
    }
}

void StructuralRuleGITF01::AnalysisInterfaceDecl(const InterfaceDecl& interfaceDecl, TypeManager* typeManager)
{
    if (!interfaceDecl.body) {
        return;
    }

    std::unordered_set<Ptr<Decl>> funcDecls;
    for (auto& decl : interfaceDecl.body->decls) {
        if (auto fd = DynamicCast<FuncDecl*>(decl.get()); fd && !fd->TestAttr(Attribute::MUT)) {
            funcDecls.emplace(fd);
        }
    }

    for (auto& decl : interfaceDecl.subDecls) {
        if (auto sd = DynamicCast<StructDecl*>(decl.get()); sd && sd->body) {
            std::unordered_set<Ptr<Decl>> varDecls;
            CollectVarDeclsOfStructDecl(sd, varDecls);
            AnalysisFuncDeclsOfStructDecl(typeManager, sd, varDecls, funcDecls);
            auto extends = typeManager->GetDeclExtends(*sd);
            for (auto& extend : extends) {
                AnalysisFuncDeclsOfExtendDecl(typeManager, extend, varDecls, funcDecls);
            }
        }
    }
}

void StructuralRuleGITF01::FindInterfaceDecl(Ptr<Cangjie::AST::Node> node, TypeManager* typeManager)
{
    if (node == nullptr) {
        return;
    }
    Walker walker(node, [this, typeManager](Ptr<Node> node) -> VisitAction {
        return match(*node)(
            [this, typeManager](const InterfaceDecl& interfaceDecl) {
                AnalysisInterfaceDecl(interfaceDecl, typeManager);
                return VisitAction::WALK_CHILDREN;
            },
            []() { return VisitAction::WALK_CHILDREN; });
    });
    walker.Walk();
}

void StructuralRuleGITF01::MatchPattern(ASTContext& ctx, Ptr<Node> node) {}

void StructuralRuleGITF01::MatchPattern(Ptr<Node> node, TypeManager* typeManager)
{
    FindInterfaceDecl(node, typeManager);
}

void StructuralRuleGITF01::DoAnalysis(CJLintCompilerInstance* instance)
{
    auto packageList = instance->GetSourcePackages();

    for (auto package : packageList) {
        MatchPattern(package, instance->typeManager);
    }
}
} // namespace Cangjie::CodeCheck
