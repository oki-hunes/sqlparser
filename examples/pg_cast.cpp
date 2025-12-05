#include <iostream>
#include <sqlparser/parser.hpp>
#include <sqlparser/generator.hpp>

void test_parse(const std::wstring& sql) {
    sqlparser::ast::SelectStatement ast;
    if (sqlparser::parser::parse(sql, ast)) {
        std::wcout << L"Parse Success: " << sql << std::endl;
        std::wstring generated = sqlparser::generate(ast);
        std::wcout << L"Generated: " << generated << std::endl;
    } else {
        std::wcerr << L"Parse Failed: " << sql << std::endl;
    }
    std::wcout << L"----------------------------------------" << std::endl;
}

int main() {
    // 1. Simple cast
    test_parse(L"SELECT '123'::INT");

    // 2. Date cast
    test_parse(L"SELECT '2023-01-01'::DATE");

    // 3. Complex type cast (TIMESTAMP WITH TIME ZONE)
    test_parse(L"SELECT '2023-01-01 12:00:00'::TIMESTAMP WITH TIME ZONE");

    // 4. Parameterized type cast
    test_parse(L"SELECT '1010'::BIT VARYING(10)");

    // 5. Array type cast
    test_parse(L"SELECT '{1,2,3}'::INT[]");

    // 6. Standard CAST syntax
    test_parse(L"SELECT CAST('123' AS INT)");

    // 7. Standard CAST with complex type
    test_parse(L"SELECT CAST('123' AS DOUBLE PRECISION)");

    // 8. Nested cast
    test_parse(L"SELECT '123'::TEXT::INT");

    return 0;
}
