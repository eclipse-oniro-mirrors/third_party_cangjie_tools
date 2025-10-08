// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "Protocol.h"

namespace ark {
using namespace Cangjie;

std::string MessageHeaderEndOfLine::eol = "\r\n\r\n";
bool MessageHeaderEndOfLine::isDeveco = false;

bool FromJSON(const nlohmann::json &params, DidOpenTextDocumentParams &reply)
{
    nlohmann::json textDocument = params["textDocument"];
    if (!textDocument.is_object()) {
        return false;
    }
    if (textDocument["uri"].is_null() || textDocument["languageId"].is_null() ||
        textDocument["version"].is_null() || textDocument["text"].is_null()) {
        return false;
    }
    if (textDocument["languageId"] != "Cangjie") {
        return false;
    }
    reply.textDocument.uri.file = textDocument.value("uri", "");
    reply.textDocument.languageId = textDocument.value("languageId", "");
    reply.textDocument.version = textDocument.value("version", -1);
    reply.textDocument.text = textDocument.value("text", "");
    return true;
}

bool FromJSON(const nlohmann::json &params, TextDocumentPositionParams &reply)
{
    if (!params["textDocument"].is_object() || !params["position"].is_object()) {
        return false;
    }

    nlohmann::json textDocument = params["textDocument"];
    if (textDocument["uri"].is_null()) {
        return false;
    }
    reply.textDocument.uri.file = textDocument.value("uri", "");

    nlohmann::json position = params["position"];
    if (!position.is_object() || position["line"].is_null() || position["character"].is_null()) {
        return false;
    }
    reply.position.line = position.value("line", -1);
    reply.position.column = position.value("character", -1);

    return true;
}

bool FromJSON(const nlohmann::json &params, SignatureHelpContext &reply)
{
    if (!params.contains("triggerKind") || params["triggerKind"].is_null()) {
        return false;
    }
    int value = params.value("triggerKind", -1);
    if (value < 0 || value >= static_cast<int>(SignatureHelpTriggerKind::END)) {
        return false;
    }
    if (params.contains("triggerCharacter") && !params["triggerCharacter"].is_null()) {
        reply.triggerCharacter = params.value("triggerCharacter", "");
    }
    if (params.contains("isRetrigger") && !params["isRetrigger"].is_null()) {
        reply.isRetrigger = params.value("isRetrigger", false);
    }
    if (params.contains("activeSignatureHelp") && !params["activeSignatureHelp"].is_null()) {
        if (params["activeSignatureHelp"].contains("activeParameter") &&
            !params["activeSignatureHelp"]["activeParameter"].is_null()) {
            reply.activeSignatureHelp.activeParameter =
                params["activeSignatureHelp"].value("activeParameter", -1);
        }
        if (params.contains("activeSignatureHelp")
            && params["activeSignatureHelp"].contains("activeSignature")
            && !params["activeSignatureHelp"]["activeSignature"].is_null()) {
            reply.activeSignatureHelp.activeSignature =
                params["activeSignatureHelp"].value("activeSignature", -1);
        }
        for (size_t count = 0; count < params["activeSignatureHelp"]["signatures"].size(); count++) {
            Signatures signatures = {};
            signatures.label = params["activeSignatureHelp"]["signatures"][count].value("label", "");
            for (size_t j = 0;
                 j < params["activeSignatureHelp"]["signatures"][count]["parameters"].size(); j++) {
                    signatures.parameters.push_back(
                        params["activeSignatureHelp"]["signatures"][count]["parameters"][j].value("label", "")
                    );
            }
            reply.activeSignatureHelp.signatures.push_back(signatures);
        }
    }
    return true;
}

// need by signaturehelp
bool FromJSON(const nlohmann::json &params, SignatureHelpParams &reply)
{
    if (!FromJSON(params, static_cast<TextDocumentPositionParams &>(reply))) {
        return false;
    }
    if (params.contains("context") && !params["context"].is_null()) {
        return FromJSON(params["context"], reply.context);
    }
    return true;
}

bool FromJSON(const nlohmann::json &params, InitializeParams &reply)
{
    if (params["rootUri"].is_null() || params["capabilities"].is_null()) {
        return false;
    }
    reply.rootUri.file = params.value("rootUri", "");

    nlohmann::json capabilities = params["capabilities"];
    if (!capabilities.is_object()) {
        return false;
    }
    nlohmann::json textDocument = capabilities["textDocument"];
    if (!textDocument.is_object()) {
        return true;
    }
    if (params.contains("initializationOptions") && !params["initializationOptions"].empty()) {
        reply.initializationOptions = params["initializationOptions"];
        // Adapting to the DevEco IDE
        if (reply.initializationOptions.contains("cangjieRootUri") &&
            !reply.initializationOptions["cangjieRootUri"].empty()) {
            reply.rootUri.file = reply.initializationOptions.value("cangjieRootUri", "");
            MessageHeaderEndOfLine::SetIsDeveco(true);
        }
    } else {
        reply.initializationOptions = nullptr;
    }
    return FetchTextDocument(textDocument, reply);
}

bool FetchTextDocument(const nlohmann::json &textDocument, InitializeParams &reply)
{
    if (textDocument.contains("documentHighlight") && textDocument["documentHighlight"].is_object()) {
        reply.capabilities.textDocumentClientCapabilities.documentHighlightClientCapabilities = true;
    }
    if (textDocument.contains("typeHierarchy") && textDocument["typeHierarchy"].is_object()) {
        reply.capabilities.textDocumentClientCapabilities.typeHierarchyCapabilities = true;
    }
    if (textDocument.contains("publishDiagnostics") && textDocument["publishDiagnostics"].is_object()) {
        nlohmann::json publishDiagnostics = textDocument["publishDiagnostics"];
        if (!publishDiagnostics["versionSupport"].is_null()) {
            reply.capabilities.textDocumentClientCapabilities.diagnosticVersionSupport =
                publishDiagnostics.value("versionSupport", false);
        }
    }
    if (textDocument.contains("completion") && textDocument["completion"].is_object()) {
        if (textDocument["completion"]["completionItem"].is_null()) { return false; }
    }
    if (textDocument.contains("hover") && textDocument["hover"].is_object()) {
            // Hover feature is not tested. Therefore, set this parameter to false to disable it.
            reply.capabilities.textDocumentClientCapabilities.hoverClientCapabilities = true;
    }
    if (textDocument.contains("documentLink") && textDocument["documentLink"].is_object()) {
        reply.capabilities.textDocumentClientCapabilities.documentLinkClientCapabilities = true;
    }
    return true;
}

bool FromJSON(const nlohmann::json &params, DidCloseTextDocumentParams &reply)
{
    nlohmann::json textDocument = params["textDocument"];
    if (!textDocument.is_object() || textDocument["uri"].is_null()) {
        return false;
    }
    reply.textDocument.uri.file = textDocument.value("uri", "");
    return true;
}

bool FromJSON(const nlohmann::json &params, TrackCompletionParams &reply)
{
    if (!params.contains("label")) {
        return false;
    }
    reply.label = params.value("label", "");
    return true;
}

bool FromJSON(const nlohmann::json &params, CompletionContext &reply)
{
    if (!params.contains("triggerKind") || params["triggerKind"].is_null()) {
        return false;
    }
    reply.triggerKind = static_cast<CompletionTriggerKind>(params.value("triggerKind", -1));
    if (reply.triggerKind == CompletionTriggerKind::TRIGGER_CHAR && params.contains("triggerCharacter") &&
        !params["triggerCharacter"].is_null()) {
        reply.triggerCharacter = params.value("triggerCharacter", "");
    }
    return true;
}

bool FromJSON(const nlohmann::json &params, CompletionParams &reply)
{
    if (!FromJSON(params, static_cast<TextDocumentPositionParams &>(reply))) {
        return false;
    }
    if (params.contains("context") && !params["context"].is_null()) {
        return FromJSON(params["context"], reply.context);
    }
    return true;
}

// For SemanticTokens Message Extraction
bool FromJSON(const nlohmann::json &params, SemanticTokensParams &reply)
{
    nlohmann::json textDocument = params["textDocument"];
    if (!textDocument.is_object() || textDocument["uri"].is_null()) {
        return false;
    }
    reply.textDocument.uri.file = textDocument.value("uri", "");
    return true;
}

bool FromJSON(const nlohmann::json &params, DidChangeTextDocumentParams &reply)
{
    nlohmann::json textDocument = params["textDocument"];
    if (!params["textDocument"].is_object()) {
        return false;
    }
    if (textDocument["uri"].is_null() || textDocument["version"].is_null()) {
        return false;
    }
    reply.textDocument.uri.file = textDocument.value("uri", "");
    reply.textDocument.version = textDocument.value("version", -1);

    nlohmann::json contentChanges = params["contentChanges"];
    if (contentChanges.empty()) {
        return false;
    }
    for (nlohmann::json change : contentChanges) {
        if (!change.is_object() || change["text"].is_null()) {
            continue;
        }
        TextDocumentContentChangeEvent changeEvent;
        changeEvent.text = change.value("text", "");
        if (change["range"].is_object()) {
            if (change["range"]["start"].is_null() || change["range"]["end"].is_null()) {
                continue;
            }
            if (!change["range"]["start"].is_object() || !change["range"]["end"].is_object()) {
                continue;
            }
            if (change["range"]["start"]["line"].is_null() || change["range"]["start"]["character"].is_null()
                || change["range"]["end"]["line"].is_null() || change["range"]["end"]["character"].is_null()) {
                continue;
            }
            Range range{};
            Position start;
            start.line = change["range"]["start"].value("line", -1);
            start.column = change["range"]["start"].value("character", -1);
            range.start = start;
            Position end;
            end.line = change["range"]["end"].value("line", -1);
            end.column = change["range"]["end"].value("character", -1);
            range.end = end;
            changeEvent.range = std::move(range);
            if (!change["rangeLength"].is_null()) {
                changeEvent.rangeLength = change.value("rangeLength", -1);
            }
        }

        reply.contentChanges.push_back(std::move(changeEvent));
    }

    return !reply.contentChanges.empty();
}

bool FromJSON(const nlohmann::json &params, RenameParams &reply)
{
    if (params["textDocument"].is_null()) {
        return false;
    }
    nlohmann::json textDocument = params["textDocument"];
    if (textDocument["uri"].is_null() || params["newName"].is_null()) {
        return false;
    }
    reply.textDocument.uri.file = textDocument.value("uri", "");
    nlohmann::json position = params["position"];
    if (!position.is_object() || position["line"].is_null() || position["character"].is_null()) {
        return false;
    }
    reply.newName = params.value("newName", "");
    reply.position.line = position.value("line", -1);
    reply.position.column = position.value("character", -1);
    return true;
}

bool FromJSON(const nlohmann::json &params, TextDocumentIdentifier &reply)
{
    if (params["uri"].is_null()) {
        return false;
    }
    reply.uri.file = params.value("uri", "");
    return true;
}

bool FromJSON(const nlohmann::json &params, TextDocumentParams &tdReply)
{
    if (params["textDocument"].is_null()) {
        return false;
    }
    nlohmann::json textDocument = params["textDocument"];
    if (textDocument["uri"].is_null()) {
        return false;
    }
    tdReply.textDocument.uri.file = textDocument.value("uri", "");
    return true;
}

bool ToJSON(const BreakpointLocation &params, nlohmann::json &reply)
{
    reply["uri"] = params.uri;
    reply["range"]["start"]["line"] = params.range.start.line;
    reply["range"]["start"]["character"] = params.range.start.column;
    reply["range"]["end"]["line"] = params.range.end.line;
    reply["range"]["end"]["character"] = params.range.end.column;
    return true;
}

bool ToJSON(const ExecutableRange &params, nlohmann::json &reply)
{
    reply["uri"] = params.uri;
    reply["projectName"] = params.projectName;
    reply["packageName"] = params.packageName;
    reply["className"] = params.className;
    reply["functionName"] = params.functionName;
    reply["range"]["start"]["line"] = params.range.start.line;
    reply["range"]["start"]["character"] = params.range.start.column;
    reply["range"]["end"]["line"] = params.range.end.line;
    reply["range"]["end"]["character"] = params.range.end.column;
    return true;
}

bool ToJSON(const CodeLens &params, nlohmann::json &reply)
{
    reply["range"]["start"]["line"] = params.range.start.line;
    reply["range"]["start"]["character"] = params.range.start.column;
    reply["range"]["end"]["line"] = params.range.end.line;
    reply["range"]["end"]["character"] = params.range.end.column;

    reply["command"]["title"] = params.command.title;
    reply["command"]["command"] = params.command.command;

    nlohmann::json jsonValue;
    for (auto &iter : params.command.arguments) {
        nlohmann::json temp;
        (void) ToJSON(iter, temp);
        jsonValue.push_back(temp);
    }
    reply["command"]["arguments"] = jsonValue;
    return true;
}

bool FromJSON(const nlohmann::json &params, TypeHierarchyItem &replyTH)
{
    if (!params["item"].is_object()) {
        return false;
    }
    replyTH.name = params["item"].value("name", "");
    replyTH.kind = static_cast<SymbolKind>(params["item"].value("kind", -1));
    replyTH.uri.file = params["item"].value("uri", "");
    replyTH.range.start.line = params["item"]["range"]["start"].value("line", -1);
    replyTH.range.start.column = params["item"]["range"]["start"].value("character", -1);
    replyTH.range.end.line = params["item"]["range"]["end"].value("line", -1);
    replyTH.range.end.column = params["item"]["range"]["end"].value("character", -1);
    replyTH.selectionRange.start.line = params["item"]["selectionRange"]["start"].value("line", -1);
    replyTH.selectionRange.start.column = params["item"]["selectionRange"]["start"].value("character", -1);
    replyTH.selectionRange.end.line = params["item"]["selectionRange"]["end"].value("line", -1);
    replyTH.selectionRange.end.column = params["item"]["selectionRange"]["end"].value("character", -1);
    replyTH.isKernel = params["item"]["data"].value("isKernel", false);
    replyTH.isChildOrSuper = params["item"]["data"].value("isChildOrSuper", false);
    auto symbolId = params["item"]["data"].value("symbolId", "");
    if (!symbolId.empty()) {
        replyTH.symbolId = std::stoull(symbolId);
    }
    return true;
}

bool ToJSON(const TypeHierarchyItem &iter, nlohmann::json &replyTH)
{
    replyTH["name"] = iter.name;
    replyTH["kind"] = static_cast<int>(iter.kind);
    replyTH["uri"] = iter.uri.file;
    replyTH["range"]["start"]["line"] = iter.range.start.line;
    replyTH["range"]["start"]["character"] = iter.range.start.column;
    replyTH["range"]["end"]["line"] = iter.range.end.line;
    replyTH["range"]["end"]["character"] = iter.range.end.column;
    replyTH["selectionRange"]["start"]["line"] = iter.selectionRange.start.line;
    replyTH["selectionRange"]["start"]["character"] = iter.selectionRange.start.column;
    replyTH["selectionRange"]["end"]["line"] = iter.selectionRange.end.line;
    replyTH["selectionRange"]["end"]["character"] = iter.selectionRange.end.column;
    replyTH["data"]["isKernel"] = iter.isKernel;
    replyTH["data"]["isChildOrSuper"] = iter.isChildOrSuper;
    replyTH["data"]["symbolId"] = std::to_string(iter.symbolId);
    return true;
}

bool FromJSON(const nlohmann::json &params, CallHierarchyItem &replyCH)
{
    if (!params["item"].is_object()) {
        return false;
    }
    replyCH.name = params["item"].value("name", "");
    replyCH.kind = static_cast<SymbolKind>(params["item"].value("kind", -1));
    replyCH.uri.file = params["item"].value("uri", "");
    replyCH.range.start.line = params["item"]["range"]["start"].value("line", -1);
    replyCH.range.start.column = params["item"]["range"]["start"].value("character", -1);
    replyCH.range.end.line = params["item"]["range"]["end"].value("line", -1);
    replyCH.range.end.column = params["item"]["range"]["end"].value("character", -1);
    replyCH.selectionRange.start.line = params["item"]["selectionRange"]["start"].value("line", -1);
    replyCH.selectionRange.start.column = params["item"]["selectionRange"]["start"].value("character", -1);
    replyCH.selectionRange.end.line = params["item"]["selectionRange"]["end"].value("line", -1);
    replyCH.selectionRange.end.column = params["item"]["selectionRange"]["end"].value("character", -1);
    replyCH.detail = params["item"].value("detail", "");
    replyCH.selectionRange.start.fileID = 1;
    replyCH.isKernel = params["item"]["data"].value("isKernel", false);
    auto symbolId = params["item"]["data"].value("symbolId", "");
    if (!symbolId.empty()) {
        replyCH.symbolId = std::stoull(symbolId);
    }
    return true;
}

bool ToJSON(const CallHierarchyItem &iter, nlohmann::json &replyCH)
{
    replyCH["name"] = iter.name;
    replyCH["kind"] = static_cast<int>(iter.kind);
    replyCH["uri"] = iter.uri.file;
    replyCH["range"]["start"]["line"] = iter.range.start.line;
    replyCH["range"]["start"]["character"] = iter.range.start.column;
    replyCH["range"]["end"]["line"] = iter.range.end.line;
    replyCH["range"]["end"]["character"] = iter.range.end.column;
    replyCH["selectionRange"]["start"]["line"] = iter.selectionRange.start.line;
    replyCH["selectionRange"]["start"]["character"] = iter.selectionRange.start.column;
    replyCH["selectionRange"]["end"]["line"] = iter.selectionRange.end.line;
    replyCH["selectionRange"]["end"]["character"] = iter.selectionRange.end.column;
    replyCH["detail"] = iter.detail;
    replyCH["data"]["isKernel"] = iter.isKernel;
    replyCH["data"]["symbolId"] = std::to_string(iter.symbolId);
    return true;
}

bool FromJSON(const nlohmann::json &params, DocumentLinkParams &reply)
{
    if (!FromJSON(params["textDocument"], reply.textDocument)) {
        return false;
    }
    return true;
}

bool FromJSON(const nlohmann::json &params, DidChangeWatchedFilesParam &reply)
{
    for (int i = 0; i < static_cast<int>(params["changes"].size()); i++) {
        FileWatchedEvent event;
        event.textDocument.uri.file = params["changes"][i].value("uri", "");
        event.type = static_cast<FileChangeType>(params["changes"][i].value("type", -1));
        reply.changes.push_back(event);
    }
    return true;
}

bool ToJSON(const CompletionItem &iter, nlohmann::json &reply)
{
    reply["label"] = iter.label;
    reply["kind"] = static_cast<int>(iter.kind);
    reply["detail"] = iter.detail;
    reply["documentation"] = iter.documentation;
    reply["sortText"] = iter.sortText;
    reply["filterText"] = iter.filterText;
    reply["insertText"] = iter.insertText;
    reply["insertTextFormat"] = static_cast<int>(iter.insertTextFormat);
    reply["deprecated"] = iter.deprecated;
    if (iter.additionalTextEdits.has_value()) {
        nlohmann::json editItems;
        for (auto &textEdit : iter.additionalTextEdits.value()) {
            nlohmann::json temp;
            temp["range"]["start"]["line"] = textEdit.range.start.line;
            temp["range"]["start"]["character"] = textEdit.range.start.column;
            temp["range"]["end"]["line"] = textEdit.range.end.line;
            temp["range"]["end"]["character"] = textEdit.range.end.column;
            temp["newText"] = textEdit.newText;
            editItems.push_back(temp);
        }
        reply["additionalTextEdits"] = editItems;
    }
    return true;
}

bool ToJSON(const DiagnosticToken &iter, nlohmann::json &reply)
{
    reply["range"]["start"]["line"] = iter.range.start.line;
    reply["range"]["start"]["character"] = iter.range.start.column;
    reply["range"]["end"]["line"] = iter.range.end.line;
    reply["range"]["end"]["character"] = iter.range.end.column;
    reply["severity"] = iter.severity;
    reply["code"] = iter.code;
    reply["source"] = iter.source;
    reply["message"] = iter.message;
    if (iter.category.has_value()) {
        reply["category"] = iter.category.value();
    }
    if (iter.relatedInformation.has_value()) {
        for (auto &info : iter.relatedInformation.value()) {
            nlohmann::json jsonInfo;
            if (!ToJSON(info, jsonInfo)) {
                continue;
            }
            reply["relatedInformation"].push_back(jsonInfo);
        }
    }
    if (iter.codeActions.has_value()) {
        nlohmann::json actions;
        for (const auto &action : iter.codeActions.value()) {
            nlohmann::json temp;
            temp["title"] = action.title;
            temp["kind"] = action.kind;
            if (action.diagnostics.has_value()) {
                nlohmann::json diagVec;
                for (const auto &diag : action.diagnostics.value()) {
                    nlohmann::json diagJson;
                    if (ToJSON(diag, diagJson)) {
                        diagVec.push_back(diagJson);
                    }
                }
                temp["diagnostics"] = diagVec;
            }
            if (action.edit.has_value()) {
                nlohmann::json editJson;
                if (ToJSON(action.edit.value(), editJson)) {
                    temp["edit"] = editJson;
                }
            }
            actions.push_back(temp);
        }
        reply["codeActions"] = actions;
    }
    return true;
}

bool ToJSON(const DiagnosticRelatedInformation &info, nlohmann::json &reply)
{
    reply["message"] = info.message;
    reply["location"]["uri"] = info.location.uri.file;
    reply["location"]["range"]["start"]["line"] = info.location.range.start.line;
    reply["location"]["range"]["start"]["character"] = info.location.range.start.column;
    reply["location"]["range"]["end"]["line"] = info.location.range.end.line;
    reply["location"]["range"]["end"]["character"] = info.location.range.end.column;
    return true;
}

bool ToJSON(const PublishDiagnosticsParams &params, nlohmann::json &reply)
{
    nlohmann::json jsonDiagnostics;
    for (auto &iter : params.diagnostics) {
        nlohmann::json value;
        if (!ToJSON(iter, value)) {
            continue;
        }
        (void)jsonDiagnostics.push_back(value);
    }
    reply["uri"] = params.uri.file;
    if (params.diagnostics.empty()) {
        reply["diagnostics"] = nlohmann::json::array(); // to make "diagnostics":[] instead of null
    } else {
        reply["diagnostics"] = jsonDiagnostics;
    }
    if (params.version.has_value()) {
        reply["version"] = params.version.value();
    }
    return true;
}

bool ToJSON(const WorkspaceEdit &params, nlohmann::json &reply)
{
    nlohmann::json changesJson;
    for (const auto &change : params.changes) {
        const std::string &uri = change.first;
        const std::vector<TextEdit> &edits = change.second;
        nlohmann::json editItems;
        for (const auto &edit : edits) {
            nlohmann::json temp;
            temp["range"]["start"]["line"] = edit.range.start.line;
            temp["range"]["start"]["character"] = edit.range.start.column;
            temp["range"]["end"]["line"] = edit.range.end.line;
            temp["range"]["end"]["character"] = edit.range.end.column;
            temp["newText"] = edit.newText;
            editItems.push_back(temp);
        }
        changesJson[uri] = editItems;
    }
    reply["changes"] = changesJson;
    return true;
}

bool ToJSON(const TextDocumentEdit &params, nlohmann::json &reply)
{
    reply["textDocument"]["uri"] = params.textDocument.uri.file;
    reply["textDocument"]["version"] = params.textDocument.version;
    nlohmann::json editItems;
    for (auto &iter : params.textEdits) {
        nlohmann::json temp;
        temp["range"]["start"]["line"] = iter.range.start.line;
        temp["range"]["start"]["character"] = iter.range.start.column;
        temp["range"]["end"]["line"] = iter.range.end.line;
        temp["range"]["end"]["character"] = iter.range.end.column;
        temp["newText"] = iter.newText;
        (void) editItems.push_back(temp);
    }
    reply["edits"] = editItems;
    return true;
}

bool FromJSON(const nlohmann::json &params, DocumentSymbolParams &dsReply)
{
    if (params["textDocument"].is_null()) {
        return false;
    }
    nlohmann::json textDocument = params["textDocument"];
    if (textDocument["uri"].is_null()) {
        return false;
    }
    dsReply.textDocument.uri.file = textDocument.value("uri", "");
    return true;
}

bool ToJSON(const DocumentSymbol &item, nlohmann::json &result)
{
    result["name"] = item.name;
    result["kind"] = static_cast<int>(item.kind);
    result["detail"] = item.detail;
    result["range"]["start"]["line"] = item.range.start.line;
    result["range"]["start"]["character"] = item.range.start.column;
    result["range"]["end"]["line"] = item.range.end.line;
    result["range"]["end"]["character"] = item.range.end.column;
    result["selectionRange"]["start"]["line"] = item.selectionRange.start.line;
    result["selectionRange"]["start"]["character"] = item.selectionRange.start.column;
    result["selectionRange"]["end"]["line"] = item.selectionRange.end.line;
    result["selectionRange"]["end"]["character"] = item.selectionRange.end.column;
    nlohmann::json children;
    for (const auto &child: item.children) {
        if (!ToJSON(child, children)) {
            continue;
        }
        result["children"].push_back(children);
    }
    return true;
}

bool ToJSON(const CallHierarchyOutgoingCall &iter, nlohmann::json &reply)
{
    (void) ToJSON(iter.to, reply["to"]);
    for (const auto &item: iter.fromRanges) {
        nlohmann::json range;
        range["start"]["line"] = item.start.line;
        range["start"]["character"] = item.start.column;
        range["end"]["line"] = item.end.line;
        range["end"]["character"] = item.end.column;
        reply["fromRanges"].push_back(range);
    }
    return true;
}

bool ToJSON(const CallHierarchyIncomingCall &iter, nlohmann::json &reply)
{
    (void) ToJSON(iter.from, reply["from"]);
    for (const auto &item: iter.fromRanges) {
        nlohmann::json range;
        range["start"]["fileID"] = item.start.fileID;
        range["start"]["line"] = item.start.line;
        range["start"]["character"] = item.start.column;
        range["end"]["fileID"] = item.end.fileID;
        range["end"]["line"] = item.end.line;
        range["end"]["character"] = item.end.column;
        reply["fromRanges"].push_back(range);
    }
    return true;
}
} // namespace ark
