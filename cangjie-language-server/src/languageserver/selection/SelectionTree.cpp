// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "SelectionTree.h"

#include <cangjie/AST/Walker.h>

#include "../ArkAST.h"
#include "../logger/Logger.h"

namespace ark {
bool SelectionTree::FindSelectNode(Decl *decl, Position start, Position end)
{
    if (!decl) {
        return false;
    }
    auto treeNode = std::make_unique<SelectionTreeNode>();

    Walker(decl, [&](auto node) {
        if (!node) {
            return VisitAction::STOP_NOW;
        }
        if (node->begin > end || node->end < start) {
            return VisitAction::SKIP_CHILDREN;
        }
        if (node->isInMacroCall) {
            return VisitAction::SKIP_CHILDREN;
        }
        if (node->begin > node->end) {
            return VisitAction::SKIP_CHILDREN;
        }
        if (node->begin <= start && node->end >= end) {
            if (!outerInterpExpr && node->astKind == ASTKind::INTERPOLATION_EXPR) {
                outerInterpExpr = node;
            }
            treeNode->node = node;
            treeNode->Parent = nullptr;
            Root = std::move(treeNode);
            MatchSelectedScope(node, start, end);
            FindTopDecl(*node);
            treeNode = std::make_unique<SelectionTreeNode>();
            if (node->begin == start && node->end == end) {
                return VisitAction::STOP_NOW;
            }
            return VisitAction::WALK_CHILDREN;
        }

        return VisitAction::SKIP_CHILDREN;
    }).Walk();
    if (!Root) {
        return false;
    }
    BuildTreeNode(Root.get(), start, end);
    return true;
}

void SelectionTree::BuildTreeNode(SelectionTreeNode *rootTreeNode, Position start, Position end)
{
    const Ptr<Node> parent = rootTreeNode->node.get();
    if (parent->begin > end || parent->end < start) {
        rootTreeNode->selected = Selection::Unselected;
    } else if (parent->begin >= start && parent->end <= end) {
        rootTreeNode->selected = Selection::Complete;
    } else {
        rootTreeNode->selected = Selection::Partial;
    }
    Walker(parent, [&](auto node) {
        if (node.get() == parent.get()) {
            return VisitAction::WALK_CHILDREN;
        }
        if (node->begin > node->end) {
            return VisitAction::SKIP_CHILDREN;
        }
        const Ptr<SelectionTreeNode> treeNode = new SelectionTreeNode();  // maybe cause memory leak, add destructure
        treeNode->node = node.get(); // maybe cause memory leak
        treeNode->Parent = parent.get();
        if (node->begin > end || node->end < start) {
            treeNode->selected = Selection::Unselected;
        } else if (node->begin >= start && node->end <= end) {
            treeNode->selected = Selection::Complete;
        } else {
            treeNode->selected = Selection::Partial;
        }
        rootTreeNode->Children.push_back(treeNode);
        BuildTreeNode(treeNode.get(), start, end);
        return VisitAction::SKIP_CHILDREN;
    }).Walk();
}

const SelectionTree::SelectionTreeNode *SelectionTree::CommonAncestor() const
{
    const SelectionTreeNode *Ancestor = Root.get();
    while (Ancestor->Children.size() == 1 && Ancestor->selected != Selection::Unselected) {
        Ancestor = Ancestor->Children.front().get();
    }
    // Returning nullptr here is a bit unprincipled, but it makes the API safer:
    // the TranslationUnitDecl contains all of the preamble, so traversing it is a
    // performance cliff. Callers can check for null and use root() if they want.
    return Ancestor != Root.get() ? Ancestor : nullptr;
}

bool SelectionTree::CreateEach(const ArkAST &arkAST,
    const std::string &fileName,
    Position Begin,
    Position End,
    std::function<bool(SelectionTree)> Func)
{
    if (Begin != End) {
        return Func(SelectionTree(arkAST, fileName, Begin, End));
    }
    return false;
}

SelectionTree::SelectionTree(const ArkAST &arkAST, const std::string &fileName, Position start, Position end)
{
    if (!arkAST.file || !arkAST.file->curPackage) {
        return;
    }

    for (auto &file : arkAST.file->curPackage->files) {
        if (!file || file->filePath != fileName) {
            continue;
        }
        for (auto &decl : file->decls) {
            if (!decl) {
                continue;
            }
            if (FindSelectNode(decl.get(), start, end)) {
                break;
            }
        }
    }
}

void SelectionTree::Walk(const SelectionTreeNode *node, ark::SelectionTree::WalkCallBack callBack)
{
    if (node) {
        WalkImpl(node, callBack);
    }
}

SelectionTree::WalkAction SelectionTree::WalkImpl(const ark::SelectionTree::SelectionTreeNode *node,
    ark::SelectionTree::WalkCallBack callBack)
{
    if (!node) {
        return WalkAction::STOP_NOW;
    }

    WalkAction action = callBack(node);
    if (action == WalkAction::STOP_NOW || action == WalkAction::SKIP_CHILDREN) {
        return action;
    }

    for (const auto &child : node->Children) {
        WalkAction childAction = WalkImpl(child.get(), callBack);
        if (childAction == WalkAction::STOP_NOW) {
            return WalkAction::STOP_NOW;
        }
    }

    return WalkAction::WALK_CHILDREN;
}

void SelectionTree::printSelection(const SelectionTreeNode* node, int level) const
{
    if (node == nullptr) {
        node = Root.get();
        if (node == nullptr) {
            Trace::Log("selection not in a top decl.");
            return;
        }
        Trace::Log("Root begin: line -> " + std::to_string(node->node->begin.line)
                   + " column -> " + std::to_string(node->node->begin.column));
        Trace::Log("Root end: line -> " + std::to_string(node->node->end.line)
                   + " column -> " + std::to_string(node->node->end.column));
        Trace::Log("Root selection -> " + std::to_string(static_cast<unsigned char>(node->selected)));
    }
    std::string levelStr = ">>";
    for (int i = 0; i < level; i++) {
        levelStr += ">>";
    }
    for (const auto child: node->Children) {
        Trace::Log(levelStr + "Node begin: line -> " + std::to_string(child->node->begin.line)
                   + " column -> " + std::to_string(child->node->begin.column));
        Trace::Log(levelStr + "Node end: line -> " + std::to_string(child->node->end.line)
                   + " column -> " + std::to_string(child->node->end.column));
        Trace::Log(levelStr + "Node selection -> "
                   + std::to_string(static_cast<unsigned char>(child->selected)));
        printSelection(child.get(), level + 1);
    }
}

void SelectionTree::MatchSelectedScope(Ptr<Node> node, Position start, Position end)
{
    if (scope != Scope::UNKNOWN) {
        return;
    }

    switch (node->astKind) {
        case ASTKind::VAR_DECL: {
            if (node->TestAttr(Attribute::GLOBAL)) {
                scope = Scope::GLOBAL_VAR;
                targetDecl = node;
                return;
            }
            if (node->TestAnyAttr(Attribute::IN_CLASSLIKE, Attribute::IN_STRUCT, Attribute::IN_ENUM)) {
                scope = Scope::MEMBER_VAR;
                targetDecl = node;
                return;
            }
            return;
        }
        case ASTKind::FUNC_DECL: {
            auto funcDecl = dynamic_cast<FuncDecl *>(node.get());
            if (!funcDecl || !funcDecl->funcBody || !funcDecl->funcBody->body) {
                return;
            }
            if (start < funcDecl->funcBody->body->begin || end > funcDecl->funcBody->body->end) {
                return;
            }
            scope = Scope::FUNC_BODY;
            targetDecl = node;
            return;
        }
        default:
            return;
    }
}

void SelectionTree::FindTopDecl(Cangjie::AST::Node &node)
{
    if (topDecl) {
        return;
    }
    Meta::match(node)(
        [&](Cangjie::AST::InterfaceDecl &interfaceDecl) {
            topDecl = &interfaceDecl;
            return;
        },
        [&](Cangjie::AST::ClassDecl &classDecl) {
            topDecl = &classDecl;
            return;
        },
        [&](Cangjie::AST::StructDecl &structDecl) {
            topDecl = &structDecl;
            return;
        },
        [&](Cangjie::AST::EnumDecl &enumDecl) {
            topDecl = &enumDecl;
            return;
        },
        [&](Cangjie::AST::ExtendDecl &extendDecl) {
            topDecl = &extendDecl;
            return;
        },
        [&](Cangjie::AST::FuncDecl &funcDecl) {
            if (funcDecl.TestAttr(Attribute::GLOBAL)) {
                topDecl = &funcDecl;
            }
            return;
        },
        [&](Cangjie::AST::VarDecl &varDecl) {
            if (varDecl.TestAttr(Attribute::GLOBAL)) {
                topDecl = &varDecl;
            }
            return;
        },
        [&]() {
            return;
        });
}

} // namespace ark