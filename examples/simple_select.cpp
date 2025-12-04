#include <iostream>
#include <string>
#include <boost/variant.hpp> // Explicitly include boost variant
#include <sqlparser/config.hpp>
#include <sqlparser/ast.hpp>
#include <sqlparser/parser.hpp>
#include <sqlparser/generator.hpp>

// Boost.Spirit X3 のヘッダ
#include <boost/spirit/home/x3.hpp>
// unicode サポートのために必要なヘッダを追加
#include <boost/spirit/home/x3/char/unicode.hpp>

namespace x3 = boost::spirit::x3;

int main() {
    // テスト用のSQL (CASE式、算術演算、LIKE、IS NULL、BETWEEN、IN、文字列連結、ビット演算を含む)
    std::wstring sql = L"SELECT name || ' (' || CAST(age AS VARCHAR) || ')' AS full_desc, age + 1 AS next_age, (age & 1) AS is_odd, (age | 2) AS bit_or, (age ^ 255) AS bit_xor, ~age AS bit_not, age << 1 AS shift_left, age >> 1 AS shift_right, CASE WHEN age < 20 THEN 0 ELSE 1 END AS is_adult FROM users WHERE name LIKE 'A%' AND (age * 2) > 30 AND NOT (age = 100) AND ! (age = 0) AND name IS NOT NULL AND age BETWEEN 20 AND 50 AND age IN (20, 30, 40) AND name IN ('Alice', 'Bob')";
    
    std::wcout << L"Testing SQL: " << sql << std::endl;

    auto iter = sql.cbegin();
    auto end = sql.cend();
    
    // パース結果を格納するAST
    sqlparser::ast::SelectStatement ast;

    try {
        // 新しい構造解析ロジックを使用
        bool r = sqlparser::parser::parse(sql, ast);

        std::wcout << L"Result: " << std::boolalpha << r << std::endl;

        if (r) {
            std::wcout << L"Parsing succeeded!" << std::endl;
            
            if (ast.quantifier == sqlparser::ast::SelectQuantifier::Distinct) {
                std::wcout << L"Has DISTINCT clause" << std::endl;
            }

            std::wcout << L"Columns: " << ast.columns.size() << std::endl;
            // カラムの詳細は複雑なので省略、再構築で確認

            if (!ast.joins.empty()) {
                std::wcout << L"Has JOIN clauses (" << ast.joins.size() << L" elements)" << std::endl;
            }

            if (ast.where) {
                std::wcout << L"Has WHERE clause" << std::endl;
            }

            if (!ast.groupBy.empty()) {
                std::wcout << L"Has GROUP BY clause (" << ast.groupBy.size() << L" elements)" << std::endl;
            }

            if (ast.having) {
                std::wcout << L"Has HAVING clause" << std::endl;
            }

            if (!ast.orderBy.empty()) {
                std::wcout << L"Has ORDER BY clause (" << ast.orderBy.size() << L" elements)" << std::endl;
            }

            if (ast.limit) {
                std::wcout << L"Has LIMIT clause" << std::endl;
            }

            if (ast.offset) {
                std::wcout << L"Has OFFSET clause" << std::endl;
            }

            // --- Phase 3: 再構築機能のデモ ---
            std::wcout << L"\n--- Regenerating SQL from parsed AST ---" << std::endl;
            std::wstring initial_generated_sql = sqlparser::generate(ast);
            std::wcout << L"Regenerated SQL: " << initial_generated_sql << std::endl;

            std::wcout << L"\n--- Modifying AST and Regenerating SQL ---" << std::endl;
            
            // ASTを変更する
            // カラムを追加: 1 AS one
            sqlparser::ast::ResultColumn new_col;
            new_col.expr = sqlparser::ast::IntLiteral{1};
            new_col.alias = L"one";
            ast.columns.push_back(new_col);

            // SQLを再構築
            std::wstring new_sql = sqlparser::generate(ast);
            std::wcout << L"Generated SQL: " << new_sql << std::endl;

        } else {
            std::wcout << L"Parsing failed" << std::endl;
        }
    } catch (...) {}

    return 0;
}
