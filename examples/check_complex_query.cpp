#include <iostream>
#include <sqlparser/parser.hpp>
#include <sqlparser/generator.hpp>

int main() {
    std::wstring sql = L"SELECT A.SID, TRIM(S.CNAME) FROM LCD_M.HT_AZA A LEFT OUTER JOIN LRG.HC_SCD S ON (A.CCD = S.CCD) AND (A.BCD = S.BCD) AND (A.SCD = S.SCD) ORDER BY A.SID";
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
