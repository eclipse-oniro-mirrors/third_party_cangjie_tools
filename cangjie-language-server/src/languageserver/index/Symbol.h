// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#ifndef LSPSERVER_INDEX_SYMBOL_H
#define LSPSERVER_INDEX_SYMBOL_H

#include <string>
#include <unordered_map>
#include <vector>
#include "cangjie/AST/Node.h"

namespace ark {
namespace lsp {

using namespace Cangjie;
using SymbolID = uint64_t;
constexpr SymbolID INVALID_SYMBOL_ID = 0;
using IDArray = std::vector<uint8_t>;

struct SymbolLocation {
    Position begin;
    Position end;
    std::string fileUri;

    bool IsZeroLoc() const
    {
        return begin.IsZero() && end.IsZero();
    }
};

enum class Modifier : uint8_t {
    UNDEFINED,
    PRIVATE,
    INTERNAL,
    PROTECTED,
    PUBLIC
};

struct CompletionItem {
    std::string label;
    std::string insertText;
};

enum class SymbolKind : uint8_t {
    UNKNOWN,

    MODULE,
    NAMESPACE,
    NAMESPACE_ALIAS,
    MACRO,

    ENUM,
    STRUCT,
    CLASS,
    PROTOCOL,
    EXTENSION,
    UNION,
    TYPE_ALIAS,

    FUNCTION,
    VARIABLE,
    FIELD,
    ENUM_CONSTANT,

    INSTANCE_METHOD,
    CLASS_METHOD,
    STATIC_METHOD,
    INSTANCE_PROPERTY,
    CLASS_PROPERTY,
    STATIC_PROPERTY,

    CONSTRUCTOR,
    DESTRUCTOR,
    CONVERSION_FUNCTION,

    PARAMETER,
    USING,
    TEMPLATE_TYPE_PARM,
    TEMPLATE_TEMPLATE_PARM,
    NON_TYPE_TEMPLATE_PARM,

    CONCEPT, /// C++20 concept.
};

enum class SymbolLanguage : uint8_t {
    C,
    OBJC,
    CXX,
    SWIFT,
    CANGJIE,
};

/// Language specific sub-kinds.
enum class SymbolSubKind : uint8_t {
    NONE,
    CXX_COPY_CONSTRUCTOR,
    CXX_MOVE_CONSTRUCTOR,
    ACCESSOR_GETTER,
    ACCESSOR_SETTER,
    USING_TYPENAME,
    USING_VALUE,
    USING_ENUM,
};

using SymbolPropertySet = uint16_t;
/// Set of properties that provide additional info about a symbol.
enum class SymbolProperty : SymbolPropertySet {
    GENERIC                         = 1 << 0,
    TEMPLATE_PARTIAL_SPECIALIZATION = 1 << 1,
    TEMPLATE_SPECIALIZATION         = 1 << 2,
    UNIT_TEST                       = 1 << 3,
    IB_ANNOTATED                    = 1 << 4,
    IB_OUTLETCOLLECTION             = 1 << 5,
    GK_INSPECTABLE                  = 1 << 6,
    LOCAL                           = 1 << 7,
    /// Symbol is part of a protocol interface.
    PROTOCOL_INTERFACE              = 1 << 8,
};

struct SymbolInfo {
    SymbolKind kind;
    SymbolSubKind subKind;
    SymbolLanguage lang = SymbolLanguage::CANGJIE;
    SymbolPropertySet properties;
};

struct Symbol {
public:
    SymbolID id;
    IDArray idArray;
    SymbolInfo symInfo = SymbolInfo();  // to ref kind
    std::string name;
    std::string scope;
    SymbolLocation location;
    SymbolLocation declaration;
    SymbolLocation canonicalDeclaration;

    AST::ASTKind kind;
    std::string signature;
    std::string returnType;
    std::string templateSpecializationArgs;
    std::string completionSnippetSuffix;
    std::string documentation;
    std::string type;
    uint32_t references = 0;

    bool isMemberParam{false};
    Modifier modifier{Modifier::UNDEFINED};
    bool isCjoSym{false};
    bool isDeprecated{false};
    std::string curModule;
    SymbolLocation curMacroCall;
    // used in completion.
    std::vector<CompletionItem> completionItems;
    // store syscap for auto import completion
    std::string syscap;

    AST::CommentGroups comments;

    enum SymbolFlag : uint8_t {
        NONE = 0,
        /// Whether or not this symbol is meant to be used for the code completion.
        /// See also isIndexedForCodeCompletion().
        /// Note that we don't store completion information (signature, snippet,
        /// type, includes) if the symbol is not indexed for code completion.
        INDEXED_FOR_CODE_COMPLETION = 1 << 0,
        /// Indicates if the symbol is deprecated.
        DEPRECATED = 1 << 1,
        /// Symbol is an implementation detail.
        IMPLEMENTATION_DETAIL = 1 << 2,
        /// Symbol is visible to other files (not e.g. a static helper function).
        VISIBLE_OUTSIDE_FILE = 1 << 3,
    };

    SymbolFlag flags = SymbolFlag::NONE;
    // Rank from pattern find
    double rank;

    bool IsInvalidSym()
    {
        return id == INVALID_SYMBOL_ID;
    }
};

using SymbolSlab = std::vector<Symbol>;

struct Comment {
public:
    SymbolID id;
    AST::CommentStyle style;
    AST::CommentKind kind;
    std::string commentStr;
};

struct ExtendItem {
public:
    SymbolID id;
    Modifier modifier;
    std::string interfaceName;
};

using ExtendSlab = std::map<SymbolID, std::vector<ExtendItem>>;

enum class CrossType: uint8_t {
    UNDEFINED,
    ARK_TS_WITH_INTEROP,
    ARK_TS_WITH_REGISTER
};

struct CrossSymbol: public Symbol {
public:
    CrossType crossType{CrossType::UNDEFINED};
    SymbolID container{};
    std::string containerName;
};

using CrossSymbolSlab = std::vector<CrossSymbol>;

} // namespace lsp
} // namespace ark
#endif // LSPSERVER_INDEX_SYMBOL_H