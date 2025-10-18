// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#pragma once

#include "Connection.h"

namespace sqldb {
/**
 * RAII-style wrapper object to deal with transactions.
 */
class Transaction {
public:
    /**
     * Create transaction object and start a new transaction.
     */
    explicit Transaction(Connection &DB);

    /**
     * Rollback current transaction if it was not already
     * committed or rolled back.
     */
    ~Transaction() noexcept;

    /**
     * Transaction objects are move-constructible.
     */
    Transaction(Transaction &&Other) noexcept;

    /**
     * Transaction objects are move-assignable.
     */
    Transaction &operator=(Transaction &&Other) noexcept;

    /**
     * Transaction objects are not copy-constructible.
     */
    Transaction(const Transaction &) = delete;

    /**
     * Transaction objects are not copy-assignable.
     */
    Transaction &operator=(const Transaction &) = delete;

    /**
     * Exchange the connection handle and current state
     * of the transaction with those of Other.
     */
    void swap(Transaction &Other) noexcept;

    /**
     * Commit current transaction. Does nothing if the transaction
     * was already committed or rolled back.
     */
    void commit();

    /**
     * Rollback current transaction. Does nothing if the transaction
     * was already committed or rolled back.
     */
    void rollback();

private:
    Connection *DB = nullptr;
};

} // namespace sqldb