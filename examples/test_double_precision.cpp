#include <iostream>
#include <sqlparser/parser.hpp>
#include <sqlparser/generator.hpp>

int main() {
    std::wstring sql = L"SELECT 123::DOUBLE PRECISION";
    sqlparser::ast::SelectStatement ast;

    if (sqlparser::parser::parse(sql, ast)) {
        std::wcout << L"Parse Success!" << std::endl;
        std::wcout << L"Generated: " << sqlparser::generate(ast) << std::endl;
    } else {
        std::wcerr << L"Parse Failed" << std::endl;
        return 1;
    }
    return 0;
}
