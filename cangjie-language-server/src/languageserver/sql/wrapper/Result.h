// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#pragma once

#include "Column.h"
#include "Invoke.h"
#include "Store.h"

#include <iterator>
#include <stdexcept>
#include <tuple>

namespace sqldb {
/**
 * Non-owning view for a row of data extracted by SQL query.
 */
class Result {
public:
    /**
     * Create a result view object for SQLite native handle. The result view
     * does not own the underlying SQLite native handle object.
     */
    explicit Result(sqlite3_stmt *Stmt) noexcept;

    /**
     * Create a Result object with invalid state.
     */
    Result() noexcept = default;

    /**
     * Result view objects are copy-constructible.
     */
    Result(const Result &) noexcept = default;

    /**
     * Result objects are copy-assignable.
     */
    Result &operator=(const Result &) noexcept = default;

    /**
     * Return true if this result object represents a row of data.
     */
    [[nodiscard]] explicit operator bool() const noexcept;

    /**
     * Get number of result columns returned by query.
     */
    [[nodiscard]] int getColumnCount() const noexcept;

    /**
     * Get Column result value. Index is zero-based.
     */
    [[nodiscard]] Column getColumn(int Index) const noexcept;

    /**
     * Get Column result value. Index is zero-based.
     */
    [[nodiscard]] Column operator[](int Index) const noexcept;

    /**
     * An input iterator over Column list of the Result object.
     */
    struct Iterator {
        using value_type = Column;
        using reference = Column;

        using iterator_category = std::input_iterator_tag;

        Iterator(sqlite3_stmt *Stmt, int Count, int Index) noexcept;

        reference operator*() const noexcept;

        Iterator &operator++() noexcept;
        Iterator operator++(int) noexcept;

        friend bool operator==(const Iterator &LHS, const Iterator &RHS) noexcept;
        friend bool operator!=(const Iterator &LHS, const Iterator &RHS) noexcept;

    private:
        sqlite3_stmt *Stmt;
        const int Count;
        int Index;
    };

    /**
     * Return an iterator to the first Column in the result.
     */
    [[nodiscard]] Iterator begin() const noexcept;

    /**
     * Return an iterator referring to the past-the-end Column in the result.
     */
    [[nodiscard]] Iterator end() const noexcept;

    /**
     * Invoke the Callback passing fetched row of data as callback arguments.
     */
    template <typename Callable>
    bool invoke(Callable &&Callback) const;

    /**
     * Copy row of data extracted by SQL query into the provided variables.
     * Pass std::ignore to ignore certain columns in the query result.
     */
    template <typename... Variables>
    void store(Variables &&...Vars) const;

private:
    sqlite3_stmt *Stmt = nullptr;
};

template <typename Callable>
bool Result::invoke(Callable &&Callback) const
{
#ifndef NO_EXCEPTIONS
    if (static_cast<unsigned>(getColumnCount()) < traits::function<Callable>::arity) {
        throw std::runtime_error("Cannot invoke callback: too many arguments");
    }
#endif
    return impl::invoke(Stmt, std::forward<Callable>(Callback));
}

template <typename... Variables>
void Result::store(Variables &&...Vars) const
{
#ifndef NO_EXCEPTIONS
    if (static_cast<unsigned>(getColumnCount()) < sizeof...(Variables)) {
        throw std::runtime_error("Cannot store result: too many arguments");
    }
#endif
    tuple_for_each(std::tie(Vars...),
        [&, Index = 0](auto &&Var) mutable { impl::store(Stmt, Index++, std::forward<decltype(Var)>(Var)); });
}

} // namespace sqldb