// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#include "SQLiteAPI.h"

#include "sqlite3.h"

namespace sqlite {

const int OK = SQLITE_OK;
const int Error = SQLITE_ERROR;
const int Internal = SQLITE_INTERNAL;
const int Perm = SQLITE_PERM;
const int Abort = SQLITE_ABORT;
const int Busy = SQLITE_BUSY;
const int Locked = SQLITE_LOCKED;
const int NoMem = SQLITE_NOMEM;
const int ReadOnly = SQLITE_READONLY;
const int IOErr = SQLITE_IOERR;
const int Interrupt = SQLITE_INTERRUPT;
const int Corrupt = SQLITE_CORRUPT;
const int Full = SQLITE_FULL;
const int CantOpen = SQLITE_CANTOPEN;
const int Protocol = SQLITE_PROTOCOL;
const int Schema = SQLITE_SCHEMA;
const int TooBig = SQLITE_TOOBIG;
const int Constraint = SQLITE_CONSTRAINT;
const int Mismatch = SQLITE_MISMATCH;
const int Misuse = SQLITE_MISUSE;
const int NotADB = SQLITE_NOTADB;
const int Notice = SQLITE_NOTICE;
const int Warning = SQLITE_WARNING;

const int OpenReadOnly = SQLITE_OPEN_READONLY;
const int OpenReadWrite = SQLITE_OPEN_READWRITE;
const int OpenCreate = SQLITE_OPEN_CREATE;
const int OpenURI = SQLITE_OPEN_URI;

const int Integer = SQLITE_INTEGER;
const int Float = SQLITE_FLOAT;
const int Text = SQLITE_TEXT;
const int Blob = SQLITE_BLOB;
const int Null = SQLITE_NULL;

const int UTF8 = SQLITE_UTF8;

const int Deterministic = SQLITE_DETERMINISTIC;
const int DirectOnly = SQLITE_DIRECTONLY;
const int Innocuous = SQLITE_INNOCUOUS;

const sqlite3_destructor_type Static = SQLITE_STATIC;
const sqlite3_destructor_type Transient = SQLITE_TRANSIENT;

int value_type(sqlite3_value *V) { return sqlite3_value_type(V); }

long long value_int64(sqlite3_value *V) { return sqlite3_value_int64(V); }

double value_double(sqlite3_value *V) { return sqlite3_value_double(V); }

unsigned value_bytes(sqlite3_value *V) { return sqlite3_value_bytes(V); }

const void *value_blob(sqlite3_value *V) { return sqlite3_value_blob(V); }

const unsigned char *value_text(sqlite3_value *V) { return sqlite3_value_text(V); }

int column_count(sqlite3_stmt *S) { return sqlite3_data_count(S); }

sqlite3_value *column_value(sqlite3_stmt *S, int I) { return sqlite3_column_value(S, I); }

int bind_null(sqlite3_stmt *S, int I) { return sqlite3_bind_null(S, I); }

int bind_int64(sqlite3_stmt *S, int I, long long V) { return sqlite3_bind_int64(S, I, V); }

int bind_double(sqlite3_stmt *S, int I, double V) { return sqlite3_bind_double(S, I, V); }

int bind_text64(sqlite3_stmt *S, int I, const char *V, unsigned long long N, sqlite3_destructor_type D, unsigned char E)
{
    return sqlite3_bind_text64(S, I, V, N, D, E);
}

int bind_blob64(sqlite3_stmt *S, int I, const void *V, unsigned long long N, sqlite3_destructor_type D)
{
    return sqlite3_bind_blob64(S, I, V, N, D);
}

unsigned bind_parameter_index(sqlite3_stmt *S, const char *Name) { return sqlite3_bind_parameter_index(S, Name); }

void result_error(sqlite3_context *C, const char *V, int N) { sqlite3_result_error(C, V, N); }

void result_int64(sqlite3_context *C, long long V) { sqlite3_result_int64(C, V); }

void result_double(sqlite3_context *C, double V) { sqlite3_result_double(C, V); }

void result_text64(sqlite3_context *C, const char *V, unsigned long long N, sqlite3_destructor_type D, unsigned char E)
{
    sqlite3_result_text64(C, V, N, D, E);
}

void result_blob64(sqlite3_context *C, const void *V, unsigned long long N, sqlite3_destructor_type D)
{
    sqlite3_result_blob64(C, V, N, D);
}

void result_null(sqlite3_context *C) { sqlite3_result_null(C); }

int create_scalar(sqlite3 *DB,
    const char *Name,
    int ArgsNum,
    int Flags,
    void *AppData,
    void (*Function)(sqlite3_context *, int, sqlite3_value **),
    void (*Destroy)(void *))
{
    return sqlite3_create_function_v2(DB, Name, ArgsNum, Flags, AppData, Function, nullptr, nullptr, Destroy);
}

void *user_data(sqlite3_context *C) { return sqlite3_user_data(C); }

int create_aggregate(sqlite3 *DB,
    const char *Name,
    int ArgsNum,
    int Flags,
    void *AppData,
    void (*Step)(sqlite3_context *, int, sqlite3_value **),
    void (*Final)(sqlite3_context *),
    void (*Destroy)(void *))
{
    return sqlite3_create_function_v2(DB, Name, ArgsNum, Flags, AppData, nullptr, Step, Final, Destroy);
}

void *aggregate_context(sqlite3_context *C, int N) { return sqlite3_aggregate_context(C, N); }

namespace fts5 {
namespace {

int getApiFromDB(sqlite3 *DB, fts5_api **API)
{
    sqlite3_stmt *Stmt = nullptr;
    int RC = sqlite3_prepare(DB, "SELECT fts5(?)", -1, &Stmt, nullptr);
    if (RC == SQLITE_OK) {
        RC = sqlite3_bind_pointer(Stmt, 1, API, "fts5_api_ptr", nullptr);
    }
    if (RC == SQLITE_OK) {
        RC = sqlite3_step(Stmt);
    }
    if (RC == SQLITE_ROW) {
        RC = sqlite3_finalize(Stmt);
    }
    return RC;
}

int createTokenizer(void *AppData, const char **, int, Fts5Tokenizer **Out)
{
    *Out = static_cast<Fts5Tokenizer *>(AppData);
    return SQLITE_OK;
}

void destroyTokenizer(Fts5Tokenizer *) {}

} // namespace

int create_tokenizer(sqlite3 *DB,
    const char *Name,
    void *AppData,
    int (*Tokenize)(
        Fts5Tokenizer *, void *, int, const char *, int, int (*Callback)(void *, int, const char *, int, int, int)),
    void (*Destroy)(void *))
{
    fts5_api *API = nullptr;
    int RC = getApiFromDB(DB, &API);
    if (RC != SQLITE_OK) {
        return RC;
    }
    fts5_tokenizer Tok;
    Tok.xCreate = createTokenizer;
    Tok.xDelete = destroyTokenizer;
    Tok.xTokenize = Tokenize;
    return API->xCreateTokenizer(API, Name, AppData, &Tok, Destroy);
}

} // namespace fts5
} // namespace sqlite