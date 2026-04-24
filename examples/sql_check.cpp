// sql_check.cpp
// Usage:
//   sql_check.exe "SELECT ..."                -- parse and regenerate
//   sql_check.exe "SELECT ..." --filter <id>  -- also apply add_existing_sql filter
//   echo "SELECT ..." | sql_check.exe         -- pipe SQL via stdin
//   sql_check.exe                             -- interactive mode (blank line to quit)
#include <iostream>
#include <string>
#include <locale>
#include <optional>
#if defined(_WIN32)
#include <io.h>    // _isatty, _fileno
#endif
#include <sqlparser/parser.hpp>
#include <sqlparser/generator.hpp>

// ---- filter logic (same as add_exsiting_sql) ----
static bool add_where_to_select(sqlparser::ast::SelectStatement& ast, long nIndex) {
    if (ast.columns.empty()) return false;
    // Use the first column expression directly as the filter condition.
    sqlparser::ast::BinaryOp eq_op;
    eq_op.op    = sqlparser::ast::OpType::EQ;
    eq_op.left  = ast.columns[0].expr;
    eq_op.right = sqlparser::ast::IntLiteral(static_cast<int>(nIndex));
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

static void check(const std::wstring& sql, std::optional<long> filter_id = std::nullopt) {
    std::wcout << L"Input : " << sql << std::endl;
    sqlparser::ast::SelectStatement ast;
    if (!sqlparser::parser::parse(sql, ast)) {
        std::wcout << L"Result: FAILED" << std::endl;
        std::wcout << std::endl;
        return;
    }
    std::wcout << L"Result: OK" << std::endl;
    std::wcout << L"Output: " << sqlparser::generate(ast) << std::endl;

    if (filter_id.has_value()) {
        // re-parse for filter (parse modifies the AST)
        sqlparser::ast::SelectStatement ast2;
        sqlparser::parser::parse(sql, ast2);
        if (!add_where_to_select(ast2, *filter_id)) {
            for (auto& u : ast2.unions) add_where_to_select(u.select.get(), *filter_id);
            std::wcout << L"Filter: FAILED (first column is not a plain identifier)" << std::endl;
        } else {
            for (auto& u : ast2.unions) add_where_to_select(u.select.get(), *filter_id);
            std::wcout << L"Filter: " << sqlparser::generate(ast2) << std::endl;
        }
    }
    std::wcout << std::endl;
}

int main(int argc, char* argv[]) {
    // Use the system locale so wcout works correctly on Windows.
    std::locale::global(std::locale(""));
    std::wcout.imbue(std::locale());
    std::wcin.imbue(std::locale());

    if (argc >= 2) {
        // SQL from command-line argument (narrow string -> wstring)
        std::string narrow(argv[1]);
        std::wstring sql(narrow.begin(), narrow.end());

        // optional: --filter <id>
        std::optional<long> filter_id;
        for (int i = 2; i < argc - 1; ++i) {
            if (std::string(argv[i]) == "--filter") {
                filter_id = std::stol(argv[i + 1]);
                break;
            }
        }
        check(sql, filter_id);
        return 0;
    }

    // Read from stdin (pipe or interactive)
    bool interactive = false;
#if defined(_WIN32)
    // Check if stdin is a terminal (not piped)
    if (_isatty(_fileno(stdin))) {
        interactive = true;
        std::wcout << L"sql_check -- enter SQL and press Enter (blank line to quit)" << std::endl;
    }
#endif

    std::wstring line;
    while (std::getline(std::wcin, line)) {
        if (line.empty()) {
            if (interactive) break;
            continue;
        }
        check(line);
    }

    return 0;
}