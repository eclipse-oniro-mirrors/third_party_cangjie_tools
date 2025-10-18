// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "StructuralRuleP03.h"
#include "common/CommonFunc.h"

namespace Cangjie::CodeCheck {
using namespace Cangjie;
using namespace AST;
using namespace Meta;

constexpr const char* JSONPATH = "/config/structural_rule_P_03.json";

void StructuralRuleP03::MemberAccessProcessor(Ptr<Node> node)
{
    if (node->astKind == AST::ASTKind::MEMBER_ACCESS) {
        auto pMemberAccess = As<ASTKind::MEMBER_ACCESS>(node);
        if (!pMemberAccess->target || pMemberAccess->target->astKind != ASTKind::FUNC_DECL) {
            return;
        }
        auto pFuncDecl = As<ASTKind::FUNC_DECL>(pMemberAccess->target);
        if (!pFuncDecl->TestAttr(Attribute::OPEN) || !pFuncDecl->outerDecl ||
            !pFuncDecl->outerDecl->TestAttr(Attribute::OPEN)) {
            return;
        }
        if (!pMemberAccess->baseExpr || pMemberAccess->baseExpr->astKind != ASTKind::REF_EXPR) {
            return;
        }

        auto ref = As<ASTKind::REF_EXPR>(pMemberAccess->baseExpr);
        if (ref->ref.target && ref->ref.target->astKind == ASTKind::FUNC_PARAM) {
            Diagnose(pMemberAccess->field.Begin(), pMemberAccess->field.End(),
                CodeCheckDiagKind::P_03_func_could_be_overridden, pFuncDecl->outerDecl->identifier.GetRawText(),
                pFuncDecl->identifier.GetRawText());
        }
    }
}

void StructuralRuleP03::RefExprProcessor(Ptr<Cangjie::AST::Node> node)
{
    if (node->astKind != ASTKind::REF_EXPR) {
        return;
    }
    auto pRefExpr = As<ASTKind::REF_EXPR>(node);
    if (!pRefExpr->ref.target || pRefExpr->ref.target->astKind != ASTKind::VAR_DECL) {
        return;
    }
    auto pVarDecl = As<ASTKind::VAR_DECL>(pRefExpr->ref.target);
    if (!pVarDecl->initializer || pVarDecl->initializer->astKind != ASTKind::CALL_EXPR) {
        return;
    }
    auto pCallExpr = As<ASTKind::CALL_EXPR>(pVarDecl->initializer);
    if (pCallExpr->baseFunc) {
        MemberAccessProcessor(pCallExpr->baseFunc);
    }
}

void StructuralRuleP03::FindFuncDecl(Ptr<FuncArg> funcArg)
{
    if (!funcArg || !funcArg->expr) {
        return;
    }

    Walker walker(funcArg->expr.get(), [this](Ptr<Node> node) -> VisitAction {
        RefExprProcessor(node);
        MemberAccessProcessor(node);
        return VisitAction::WALK_CHILDREN;
    });
    walker.Walk();
}

bool StructuralRuleP03::IsSecurityCheckCallExprHelper(Ptr<Cangjie::AST::Decl> target)
{
    if (!target || target->astKind != ASTKind::FUNC_DECL) {
        return false;
    }
    auto pFuncDecl = As<ASTKind::FUNC_DECL>(target);

    if (!jsonInfo.contains("SecurityCheckFunctionInfo")) {
        return false;
    }

    for (auto& item : jsonInfo["SecurityCheckFunctionInfo"]) {
        if (!item.contains("funcName") || !item.contains("fullPackageName")) {
            return false;
        }
        std::string funcName = item["funcName"].get<std::string>();
        std::string fullPackageName = item["fullPackageName"].get<std::string>();
        if (pFuncDecl->identifier.GetRawText() == funcName && pFuncDecl->fullPackageName == fullPackageName) {
            return true;
        }
    }
    return false;
}

bool StructuralRuleP03::IsSecurityCheckCallExpr(Ptr<Cangjie::AST::CallExpr> pCallExpr)
{
    if (pCallExpr->baseFunc && pCallExpr->baseFunc->astKind == ASTKind::REF_EXPR) {
        auto pRefExpr = As<ASTKind::REF_EXPR>(pCallExpr->baseFunc);
        return IsSecurityCheckCallExprHelper(pRefExpr->ref.target);
    } else if (pCallExpr->baseFunc && pCallExpr->baseFunc->astKind == ASTKind::MEMBER_ACCESS) {
        auto pMemberAccess = As<ASTKind::MEMBER_ACCESS>(pCallExpr->baseFunc);
        return IsSecurityCheckCallExprHelper(pMemberAccess->target);
    }
    return false;
}

void StructuralRuleP03::FindCallExpr(Ptr<Cangjie::AST::Node> node)
{
    if (!node) {
        return;
    }
    Walker walker(node, [this](Ptr<Node> node) -> VisitAction {
        if (node->astKind == ASTKind::CALL_EXPR) {
            auto pCallExpr = As<ASTKind::CALL_EXPR>(node.get());
            auto isSecurityCheckCallExpr = IsSecurityCheckCallExpr(pCallExpr);
            if (isSecurityCheckCallExpr) {
                for (auto& funcArg : pCallExpr->args) {
                    FindFuncDecl(funcArg);
                }
            }
        }
        return VisitAction::WALK_CHILDREN;
    });
    walker.Walk();
}

void StructuralRuleP03::MatchPattern(ASTContext& ctx, Ptr<Node> node)
{
    (void)ctx;
    if (CommonFunc::ReadJsonFileToJsonInfo(JSONPATH, ConfigContext::GetInstance(), jsonInfo) == ERR) {
        return;
    }
    FindCallExpr(node);
}
} // namespace Cangjie::CodeCheck
