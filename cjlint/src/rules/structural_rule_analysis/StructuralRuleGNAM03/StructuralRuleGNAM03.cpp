// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "StructuralRuleGNAM03.h"
#include <regex>
#include "common/CommonFunc.h"

using namespace Cangjie;
using namespace Cangjie::AST;
using namespace Meta;
using namespace CodeCheck;

/**
 * upper camel case rule
 * When a variable name or function name is a unique identifier consisting of one or more words linked together,
 * the first letter of each word is capitalized.
 * right eg:
 * CheckNameRule , FindAssignTypes ,
 * wrong eg:
 * checkNameRule , findAssignTypes , 34come
 */
static const std::string UPPER_CAMEL_CASE = "[A-Z][A-Z0-9a-z]*";

template <typename T> auto StructuralRuleGNAM03::CheckNameRule(T &decl)
{
    if (!regex_match(decl.identifier.Val(), std::regex(UPPER_CAMEL_CASE))) {
        Diagnose(
            decl.begin, decl.end, CodeCheckDiagKind::G_NAM_03_identifier_naming_information, decl.identifier.Val());
    }
    return VisitAction::SKIP_CHILDREN;
}

bool StructuralRuleGNAM03::IsExceptionSubclass(const ClassDecl &classDecl, std::set<Ptr<ClassDecl>> declSet)
{
    if (classDecl.inheritedTypes.empty()) {
        return false;
    }
    for (auto &it : classDecl.inheritedTypes) {
        if (it->ToString() == "Exception" || it->ToString() == "Error" || it->ToString() == "Throwable") {
            return true;
        }
        if (it->ty && it->ty->kind == TypeKind::TYPE_CLASS) {
            Ptr<ClassDecl> claDecl = StaticCast<ClassTy>(it->ty)->decl;
            if (declSet.count(claDecl) > 0) {
                continue;
            }
            declSet.insert(claDecl);
            if (IsExceptionSubclass(*claDecl, declSet)) {
                return true;
            }
        }
    }
    return false;
}

void StructuralRuleGNAM03::CheckExceptionRule(const ClassDecl &classDecl)
{
    if (!IsExceptionSubclass(classDecl)) {
        return;
    }
    std::string str = classDecl.identifier.Val();
    if (!CommonFunc::HasEnding(str, "Error") && !CommonFunc::HasEnding(str, "Exception")) {
        Diagnose(classDecl.begin, classDecl.end, CodeCheckDiagKind::G_NAM_03_exception_naming_information,
            classDecl.identifier.Val());
    }
}

void StructuralRuleGNAM03::PrintDiagnoseInfo(
    bool condition, Position start, Position end, const std::string& value)
{
    if (condition && !regex_match(value, std::regex(UPPER_CAMEL_CASE))) {
        Diagnose(start, end, CodeCheckDiagKind::G_NAM_03_identifier_naming_information, value);
    }
}

void StructuralRuleGNAM03::CheckQuoteExprToken(std::vector<Token> &tokens)
{
    for (size_t i = 0; i < tokens.size() - 1; ++i) {
        switch (tokens[i].kind) {
            case TokenKind::CLASS:
            case TokenKind::INTERFACE:
            case TokenKind::STRUCT:
            case TokenKind::TYPE: {
                ++i;
                PrintDiagnoseInfo(
                    tokens[i].kind == TokenKind::IDENTIFIER, tokens[i].Begin(), tokens[i].End(), tokens[i].Value());
                break;
            }
            case TokenKind::ENUM: {
                while (tokens[i].kind != TokenKind::RCURL && i < tokens.size() - 1) {
                    bool isNotFunc = tokens[i].kind != TokenKind::FUNC;
                    ++i;
                    bool isIdentifier = tokens[i].kind == TokenKind::IDENTIFIER;
                    PrintDiagnoseInfo(
                        (isNotFunc && isIdentifier), tokens[i].Begin(), tokens[i].End(), tokens[i].Value());
                }
                break;
            }
            default: {
            }
        }
    }
}

void StructuralRuleGNAM03::CheckQuoteExpr(const Cangjie::AST::QuoteExpr &quoteExpr)
{
    for (auto &expr : quoteExpr.exprs) {
        if (expr->astKind != ASTKind::TOKEN_PART) {
            continue;
        }
        auto tokenPart = static_cast<TokenPart*>(expr.get().get());
        CheckQuoteExprToken(tokenPart->tokens);
    }
}

void StructuralRuleGNAM03::FindAssignTypes(Ptr<Node> node)
{
    if (node == nullptr) {
        return;
    }

    Walker walker(node, [this](Ptr<Node> node) -> VisitAction {
        return match(*node)(
            [this](const ClassDecl &classDecl) {
                CheckExceptionRule(classDecl);
                return CheckNameRule(classDecl);
            },
            [this](const InterfaceDecl &interfaceDecl) { return CheckNameRule(interfaceDecl); },
            [this](const StructDecl &structDecl) { return CheckNameRule(structDecl); },
            [this](const TypeAliasDecl &typeAliasDecl) { return CheckNameRule(typeAliasDecl); },
            [this](const EnumDecl &enumDecl) {
                for (auto &it : enumDecl.constructors) {
                    if (!regex_match(it->identifier.Val(), std::regex(UPPER_CAMEL_CASE))) {
                        Diagnose(it->begin, it->end, CodeCheckDiagKind::G_NAM_03_identifier_naming_information,
                            it->identifier.Val());
                    }
                }
                return CheckNameRule(enumDecl);
            },
            [this](const QuoteExpr &quoteExpr) {
                CheckQuoteExpr(quoteExpr);
                return VisitAction::WALK_CHILDREN;
            },
            []() { return VisitAction::WALK_CHILDREN; });
    });
    walker.Walk();
}

void StructuralRuleGNAM03::MatchPattern(ASTContext &ctx, Ptr<Node> node)
{
    (void)ctx;
    FindAssignTypes(node);
}
