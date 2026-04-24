#include <iostream>
#include <string>
#include <sqlparser/parser.hpp>
#include <sqlparser/generator.hpp>

// Helper: appends a "field = nIndex" condition to the WHERE clause of a SELECT
// using the first column name (or its alias). Works on each part of a UNION.
bool add_where_to_select(sqlparser::ast::SelectStatement& ast, long nIndex) {
    if (ast.columns.empty()) return false;

    // Use the first column expression directly as the filter condition.
    // (Aliases cannot be referenced in WHERE since it is evaluated before SELECT.)
    sqlparser::ast::BinaryOp eq_op;
    eq_op.op    = sqlparser::ast::OpType::EQ;
    eq_op.left  = ast.columns[0].expr;
    eq_op.right = sqlparser::ast::IntLiteral(static_cast<int>(nIndex));

    // AND the new condition into an existing WHERE, or set it directly.
    if (ast.where) {
        sqlparser::ast::BinaryOp and_op;
        and_op.op    = sqlparser::ast::OpType::AND;
        and_op.left  = *ast.where;
        and_op.right = eq_op;
        ast.where = and_op;
    } else {
        ast.where = eq_op;
    }
    return true;
}

// Rewrites every SELECT part of src (including UNION branches) by appending
// a "first_field = nIndex" filter, then returns the regenerated SQL.
bool build_id_filter_sql(const std::wstring& src, long nIndex, std::wstring& out_sql) {
    sqlparser::ast::SelectStatement ast;
    if (!sqlparser::parser::parse(src, ast)) return false;

    // Add condition to the main SELECT.
    if (!add_where_to_select(ast, nIndex)) return false;

    // Add the same condition to each UNION branch.
    for (auto& u : ast.unions) {
        if (!add_where_to_select(u.select.get(), nIndex)) return false;
    }

    out_sql = sqlparser::generate(ast);
    return true;
}

// -------------------------------------------------
// Demo helper: prints the input SQL and its rewritten form.
// -------------------------------------------------
void demo(const std::wstring& label, const std::wstring& src, long nIndex) {
    std::wcout << L"=== " << label << L" ===" << std::endl;
    std::wcout << L"Input  SQL : " << src << std::endl;
    std::wcout << L"Filter ID  : " << nIndex << std::endl;

    std::wstring result;
    if (build_id_filter_sql(src, nIndex, result)) {
        std::wcout << L"Output SQL : " << result << std::endl;
    } else {
        std::wcerr << L"Failed to process SQL." << std::endl;
    }
    std::wcout << std::endl;
}

int main() {
    // --- Case 1: no WHERE clause ---
    demo(
        L"No WHERE clause",
        L"SELECT shape_id, name, area FROM shapes",
        42
    );

    // --- Case 2: existing WHERE clause ---
    demo(
        L"Existing WHERE clause",
        L"SELECT shape_id, name FROM shapes WHERE area > 100",
        7
    );

    // --- Case 3: first column has an alias (alias is used as the filter field) ---
    demo(
        L"First column with alias",
        L"SELECT s.id AS shape_id, s.name FROM shapes s",
        15
    );

    // --- Case 4: SQL with UNION ---
    demo(
        L"UNION",
        L"SELECT shape_id, name FROM shapes_a UNION SELECT shape_id, name FROM shapes_b",
        3
    );

    // --- Case 5: UNION with existing WHERE ---
    demo(
        L"UNION + existing WHERE",
        L"SELECT shape_id, name FROM shapes_a WHERE area > 50 UNION SELECT shape_id, name FROM shapes_b WHERE area > 50",
        8
    );

    return 0;
}