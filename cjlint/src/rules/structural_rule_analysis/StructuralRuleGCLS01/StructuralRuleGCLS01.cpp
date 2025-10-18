// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "StructuralRuleGCLS01.h"

namespace Cangjie::CodeCheck {
using namespace Cangjie;
using namespace Cangjie::AST;
using namespace Meta;

constexpr int PRIVATE = 1;
constexpr int DEFAULT = 2;
constexpr int PROTECTED = 3;
constexpr int PUBLIC = 4;
std::set<TokenKind> FUNC_ACCESS{ TokenKind::PRIVATE, TokenKind::PUBLIC, TokenKind::PROTECTED };
std::map<std::string, int> COMP_ACCESS{
    { "public", PUBLIC },
    { "protected", PROTECTED },
    { "default", DEFAULT },
    { "private", PRIVATE },
};

void StructuralRuleGCLS01::CheckClassDecl(const ClassDecl &classDecl)
{
    if (classDecl.subDecls.empty()) {
        return;
    }
    if (classDecl.body == nullptr || classDecl.body->decls.empty()) {
        return;
    }
    std::map<std::pair<std::string, std::vector<std::string>>, std::string> funcAccessibility;
    CheckParentClassFuncAccess(classDecl, funcAccessibility);
    std::set<Ptr<Cangjie::AST::Decl>> visitedClassDecl{};
    CheckSubClassFuncAccess(classDecl, funcAccessibility, classDecl.identifier, visitedClassDecl);
}

std::string StructuralRuleGCLS01::CheckModifier(const std::set<Cangjie::AST::Modifier> &modifiers)
{
    for (auto &modifier : modifiers) {
        if (FUNC_ACCESS.find(modifier.modifier) != FUNC_ACCESS.end()) {
            return modifier.ToString();
        }
    }
    return "";
}

void StructuralRuleGCLS01::CheckParentClassFuncAccess(const ClassDecl &classDecl,
    std::map<std::pair<std::string, std::vector<std::string>>, std::string> &funcAccessibility)
{
    for (auto &decl : classDecl.body->decls) {
        if (decl->astKind != ASTKind::FUNC_DECL) {
            continue;
        }
        auto funcDecl = As<ASTKind::FUNC_DECL>(decl.get());
        if (!funcDecl) {
            continue;
        }
        auto result = std::any_of(funcDecl->modifiers.begin(), funcDecl->modifiers.end(),
            [](const Cangjie::AST::Modifier &modifier) { return modifier.modifier == TokenKind::OPEN; });
        if (!result) {
            continue;
        }
        auto access = CheckModifier(funcDecl->modifiers);
        if (access.empty()) {
            continue;
        }

        funcAccessibility[GetFuncDecl(funcDecl)] = access;
    }
}

void StructuralRuleGCLS01::CheckSubClassFuncAccess(const Cangjie::AST::ClassDecl &classDecl,
    std::map<std::pair<std::string, std::vector<std::string>>, std::string> &funcAccessibility,
    const std::string &parentClassName, std::set<Ptr<Cangjie::AST::Decl>> &visitedClassDecl)
{
    for (auto &decl : classDecl.subDecls) {
        if (visitedClassDecl.count(decl) > 0) {
            continue;
        }
        visitedClassDecl.insert(decl);
        auto childClassDecl = As<ASTKind::CLASS_DECL>(decl);
        if (!childClassDecl) {
            continue;
        }
        if (childClassDecl->body == nullptr || childClassDecl->body->decls.empty()) {
            continue;
        }
        CheckSubClassFuncAccessHelper(childClassDecl, funcAccessibility, parentClassName, visitedClassDecl);
        if (!childClassDecl->subDecls.empty()) {
            CheckSubClassFuncAccess(*childClassDecl, funcAccessibility, classDecl.identifier, visitedClassDecl);
        }
    }
}

bool StructuralRuleGCLS01::CheckFuncCall(Ptr<FuncDecl> funcDecl,
    std::map<std::pair<std::string, std::vector<std::string>>, std::string> &funcAccessibility,
    const std::string &parentClassName, const std::string &subClassName, const std::string &access)
{
    if (!funcDecl->funcBody || !funcDecl->funcBody->body) {
        return false;
    }
    if (funcDecl->funcBody->body->body.size() > 1) {
        return false;
    }

    for (auto &node : funcDecl->funcBody->body->body) {
        if (node->astKind != ASTKind::CALL_EXPR) {
            continue;
        }
        auto callExpr = As<ASTKind::CALL_EXPR>(node.get());
        if (!callExpr || callExpr->baseFunc == nullptr) {
            continue;
        }
        if (callExpr->baseFunc->astKind == ASTKind::MEMBER_ACCESS) {
            auto ma = As<ASTKind::MEMBER_ACCESS>(callExpr->baseFunc.get());
            if (ma == nullptr || ma->target == nullptr || ma->target->astKind != ASTKind::FUNC_DECL) {
                continue;
            }
            auto func = As<ASTKind::FUNC_DECL>(ma->target);
            if (func == nullptr) {
                continue;
            }
            return CheckFuncCallHelper(func, funcAccessibility, parentClassName, subClassName, access);
        }
    }
    return false;
}

bool StructuralRuleGCLS01::CheckFuncCallHelper(Ptr<FuncDecl> funcDecl,
    std::map<std::pair<std::string, std::vector<std::string>>, std::string> &funcAccessibility,
    const std::string &parentClassName, const std::string &subClassName, const std::string &access)
{
    if (funcDecl == nullptr || funcDecl->outerDecl == nullptr) {
        return false;
    }
    if (funcDecl->outerDecl->astKind == ASTKind::CLASS_DECL &&
        (funcDecl->outerDecl->identifier == parentClassName || funcDecl->outerDecl->identifier == subClassName) &&
        funcAccessibility.count(GetFuncDecl(funcDecl))) {
        if (COMP_ACCESS[funcAccessibility[GetFuncDecl(funcDecl)]] < COMP_ACCESS[access]) {
            return true;
        }
    }
    return false;
}

std::pair<std::string, std::vector<std::string>> StructuralRuleGCLS01::GetFuncDecl(Ptr<FuncDecl> funcDecl)
{
    std::vector<std::string> params;
    for (auto &param : funcDecl->funcBody->paramLists[0]->params) {
        params.emplace_back(param->type->ToString());
    }
    return make_pair(funcDecl->identifier, params);
}

void StructuralRuleGCLS01::FindClassDecl(Ptr<Node> node)
{
    if (node == nullptr) {
        return;
    }

    Walker walker(node, [this](Node *node) -> VisitAction {
        match (*node)([this](const ClassDecl &classDecl) { CheckClassDecl(classDecl); });
        return VisitAction::WALK_CHILDREN;
    });
    walker.Walk();
}

void StructuralRuleGCLS01::MatchPattern(ASTContext &ctx, Ptr<Node> node)
{
    (void)ctx;
    FindClassDecl(node);
}
void StructuralRuleGCLS01::CheckSubClassFuncAccessHelper(Ptr<Cangjie::AST::ClassDecl> classDecl,
    std::map<std::pair<std::string, std::vector<std::string>>, std::string> &funcAccessibility,
    const std::string &parentClassName, std::set<Ptr<Cangjie::AST::Decl>> &visitedClassDecl)
{
    for (auto &childdecl : classDecl->body->decls) {
        if (visitedClassDecl.count(childdecl) > 0) {
            continue;
        }
        visitedClassDecl.insert(childdecl);
        if (childdecl->astKind != ASTKind::FUNC_DECL) {
            continue;
        }
        auto funcDecl = As<ASTKind::FUNC_DECL>(childdecl.get());
        if (!funcDecl) {
            continue;
        }
        auto access = CheckModifier(funcDecl->modifiers);
        if (access.empty()) {
            continue;
        }
        if (!funcAccessibility.count(GetFuncDecl(funcDecl))) {
            if (CheckFuncCall(funcDecl, funcAccessibility, parentClassName, classDecl->identifier, access)) {
                Diagnose(funcDecl->identifier.Begin(), funcDecl->identifier.End(),
                    CodeCheckDiagKind::G_CLS_01_increase_the_accessibility_of_the_function_02);
            }
            continue;
        }
        if (COMP_ACCESS[funcAccessibility[GetFuncDecl(funcDecl)]] < COMP_ACCESS[access]) {
            Diagnose(funcDecl->identifier.Begin(), funcDecl->identifier.End(),
                CodeCheckDiagKind::G_CLS_01_increase_the_accessibility_of_the_function_01);
        }
    }
}
}