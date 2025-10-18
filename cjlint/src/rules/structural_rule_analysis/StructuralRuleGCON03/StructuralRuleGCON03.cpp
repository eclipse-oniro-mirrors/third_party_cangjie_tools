// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "StructuralRuleGCON03.h"
namespace Cangjie::CodeCheck {
using namespace Cangjie;
using namespace Cangjie::AST;
using namespace Meta;

void StructuralRuleGCON03::OverrideFuncFinder(Ptr<Cangjie::AST::Node> node)
{
    if (node == nullptr) {
        return;
    }

    Walker walker(node, [this](Ptr<Node> node) -> VisitAction {
        return match(*node)(
            [this](const FuncDecl& funcDecl) {
                if (IsOverrideModifier(funcDecl.modifiers) && funcDecl.funcBody->body != nullptr &&
                    !IsThreadSafe(funcDecl.funcBody->body->body)) {
                    auto classDecl = DynamicCast<ClassDecl*>(funcDecl.outerDecl.get());
                    if (!classDecl) {
                        return VisitAction::SKIP_CHILDREN;
                    }
                    std::pair<std::pair<std::string, std::string>, PositionPair> pair;
                    if (classDecl->GetSuperClassDecl() != nullptr) {
                        std::pair<std::string, std::string> funcInfo(
                            classDecl->GetSuperClassDecl()->identifier, funcDecl.identifier);
                        pair.first = funcInfo;
                        pair.second = std::make_pair(funcDecl.begin, funcDecl.end);
                        (void)nonThreadSafeOverrideFuncSet.insert(pair);
                    }
                }
                return VisitAction::WALK_CHILDREN;
            },
            []() { return VisitAction::WALK_CHILDREN; }
        );
    });
    walker.Walk();
}

bool StructuralRuleGCON03::IsOverrideModifier(const std::set<Cangjie::AST::Modifier> &modifiers)
{
    auto result = std::any_of(modifiers.begin(), modifiers.end(), [](const Cangjie::AST::Modifier &modifier) {
        return modifier.modifier == TokenKind::OVERRIDE || modifier.modifier == TokenKind::REDEF;
    });
    return result;
}

bool StructuralRuleGCON03::IsThreadSafe(std::vector<OwnedPtr<Cangjie::AST::Node>> &nodes)
{
    bool inLock = false;
    for (auto &node : nodes) {
        switch (node->astKind) {
            case ASTKind::SYNCHRONIZED_EXPR: {
                break;
            }
            case ASTKind::INC_OR_DEC_EXPR: {
                auto incOrDecExpr = As<ASTKind::INC_OR_DEC_EXPR>(node.get());
                if (!IsIncOrDecSafe(incOrDecExpr) && !inLock) {
                    return false;
                }
                break;
            }
            case ASTKind::ASSIGN_EXPR: {
                auto assignExpr = As<ASTKind::ASSIGN_EXPR>(node.get());
                if (!IsAssignSafe(assignExpr) && !inLock) {
                    return false;
                }
                break;
            }
            case ASTKind::CALL_EXPR: {
                auto callExpr = As<ASTKind::CALL_EXPR>(node.get());
                if (safeFuncSet.find(callExpr->begin) != safeFuncSet.end() ||
                    checkedFuncSet.find(callExpr->begin) != checkedFuncSet.end()) {
                    break;
                }
                (void)checkedFuncSet.insert(callExpr->begin);
                if (IsReentrantMutex(callExpr) == MutexState::MUTEX_LOCK) {
                    inLock = true;
                } else if (IsReentrantMutex(callExpr) == MutexState::MUTEX_UNLOCK) {
                    inLock = false;
                }
                if (!IsFuncSafe(callExpr) && !inLock) {
                    return false;
                }
                (void)safeFuncSet.insert(callExpr->begin);
                break;
            }
            default: {
            }
        }
    }
    return true;
}

bool StructuralRuleGCON03::IsAssignSafe(Ptr<Cangjie::AST::AssignExpr> assignExpr)
{
    if (assignExpr->leftValue == nullptr) {
        return true;
    }
    if (assignExpr->leftValue->astKind == ASTKind::REF_EXPR) {
        auto refExpr = As<ASTKind::REF_EXPR>(assignExpr->leftValue.get());
        if (refExpr == nullptr) {
            return true;
        }
        return refExpr->ref.target != nullptr && !refExpr->ref.target->IsStaticOrGlobal();
    }
    return true;
}

bool StructuralRuleGCON03::IsIncOrDecSafe(Ptr<Cangjie::AST::IncOrDecExpr> incOrDecExpr)
{
    auto assignExpr = As<ASTKind::ASSIGN_EXPR>(incOrDecExpr->desugarExpr.get());
    if (assignExpr == nullptr) {
        return true;
    }
    return IsAssignSafe(assignExpr);
}

bool StructuralRuleGCON03::CoverDeclToFuncDecl(Ptr<Cangjie::AST::Decl> decl)
{
    if (decl && decl->astKind == ASTKind::FUNC_DECL) {
        auto funcDecl = As<ASTKind::FUNC_DECL>(decl);
        if (funcDecl->funcBody->body != nullptr && !IsThreadSafe(funcDecl->funcBody->body->body)) {
            return false;
        }
    }
    return true;
}

bool StructuralRuleGCON03::IsFuncSafe(Ptr<Cangjie::AST::CallExpr> callExpr)
{
    if (callExpr->baseFunc->astKind == ASTKind::REF_EXPR) {
        auto refExpr = As<ASTKind::REF_EXPR>(callExpr->baseFunc.get());
        if (!CoverDeclToFuncDecl(refExpr->ref.target)) {
            return false;
        }

        for (auto &target : refExpr->ref.targets) {
            if (!CoverDeclToFuncDecl(target)) {
                return false;
            }
        }
    }
    return true;
}

StructuralRuleGCON03::MutexState StructuralRuleGCON03::IsReentrantMutex(Ptr<Cangjie::AST::CallExpr> callExpr)
{
    if (callExpr->baseFunc->astKind != AST::ASTKind::MEMBER_ACCESS) {
        return MutexState::NOT_MUTEX;
    }
    auto memberAccess = As<ASTKind::MEMBER_ACCESS>(callExpr->baseFunc.get());
    if (memberAccess == nullptr || memberAccess->baseExpr == nullptr ||
        memberAccess->baseExpr->astKind != AST::ASTKind::REF_EXPR) {
        return MutexState::NOT_MUTEX;
    }
    auto ref = As<ASTKind::REF_EXPR>(memberAccess->baseExpr.get());
    if (ref == nullptr || ref->ref.target == nullptr || ref->ref.target->ty->name != "ReentrantMutex") {
        return MutexState::NOT_MUTEX;
    }
    return memberAccess->field == "lock" ? MutexState::MUTEX_LOCK : MutexState::MUTEX_UNLOCK;
}

void StructuralRuleGCON03::BaseClassFinder(Ptr<Cangjie::AST::Node> node)
{
    if (node == nullptr) {
        return;
    }

    Walker walker(node, [this](Ptr<Node> node) -> VisitAction {
        match (*node)([this](const ClassDecl &classDecl) {
            auto result = std::any_of(nonThreadSafeOverrideFuncSet.begin(), nonThreadSafeOverrideFuncSet.end(),
                [&classDecl](const std::pair<std::pair<std::string, std::string>, PositionPair> &pair) {
                    return pair.first.first == classDecl.identifier;
                });
            if (result) {
                CheckBaseClassFuncSafe(classDecl.body->decls, classDecl.identifier);
            }
        });
        return VisitAction::WALK_CHILDREN;
    });
    walker.Walk();
}

void StructuralRuleGCON03::CheckBaseClassFuncSafe(std::vector<OwnedPtr<Cangjie::AST::Decl>> &decls,
    const std::string &className)
{
    for (auto &decl : decls) {
        switch (decl->astKind) {
            case ASTKind::FUNC_DECL: {
                auto funcDecl = As<ASTKind::FUNC_DECL>(decl.get());
                if (funcDecl != nullptr && funcDecl->funcBody != nullptr && funcDecl->funcBody->body != nullptr &&
                    IsThreadSafe(funcDecl->funcBody->body->body)) {
                    std::pair<std::string, std::string> pair(className, funcDecl->identifier);
                    (void)threadSafeBaseFuncSet.insert(pair);
                }
                break;
            }
            default: {
            }
        }
    }
}

void StructuralRuleGCON03::ResultCheck(
    const std::set<std::pair<std::pair<std::string, std::string>, PositionPair>> &unsafeThreadOverrideFuncSet,
    const std::set<std::pair<std::string, std::string>> &safeThreadBaseFuncSet)
{
    for (auto &item : unsafeThreadOverrideFuncSet) {
        auto result = std::any_of(safeThreadBaseFuncSet.begin(), safeThreadBaseFuncSet.end(),
            [&item](const std::pair<std::string, std::string> &pair) {
                return (pair.first == item.first.first && pair.second == item.first.second);
            });
        if (result) {
            Diagnose(item.second.first, item.second.second, CodeCheckDiagKind::G_CON_03_override_func_thread_safe,
                item.first.second);
        }
    }
}

void StructuralRuleGCON03::MatchPattern(ASTContext &ctx, Ptr<Cangjie::AST::Node> node)
{
    (void)ctx;
    OverrideFuncFinder(node);
    BaseClassFinder(node);
    ResultCheck(nonThreadSafeOverrideFuncSet, threadSafeBaseFuncSet);
}
}
