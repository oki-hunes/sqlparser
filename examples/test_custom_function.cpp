#include <iostream>
#include <sqlparser/parser.hpp>
#include <sqlparser/generator.hpp>

int main() {
    // ユーザー定義関数 Foo(name) と Bar(1, 2) を含むクエリ
    std::wstring sql = L"SELECT Foo(name), Bar(1, 2) FROM users";
    sqlparser::ast::SelectStatement ast;

    if (sqlparser::parser::parse(sql, ast)) {
        std::wcout << L"Parse Success!" << std::endl;
        std::wcout << L"Original: " << sql << std::endl;
        std::wcout << L"Generated: " << sqlparser::generate(ast) << std::endl;
        
        // ASTの中身を確認
        if (ast.columns.size() >= 1) {
            auto& col = ast.columns[0];
            // col.expr が FunctionCall かどうか確認
            if (auto* func = boost::get<sqlparser::ast::FunctionCall>(&col.expr)) {
                std::wcout << L"First column is a function call: " << func->name << std::endl;
            }
        }
    } else {
        std::wcerr << L"Parse Failed" << std::endl;
        return 1;
    }
    return 0;
}
