#include <iostream>
#include <sqlparser/parser.hpp>
#include <sqlparser/generator.hpp>

int main() {
    std::wstring sql = L"select S.SCD, CAST(FUK_K_X AS NVARCHAR(15)) from KOT_ROS S, KOT_ROS_B B WHERE (S.LCD = 20011212) AND (S.ROS_N = B.ROS_N) AND (S.YEAR = B.YEAR) order by S.SCD";
    sqlparser::ast::SelectStatement ast;

    if (sqlparser::parser::parse(sql, ast)) {
        std::wcout << L"Parse Success!" << std::endl;
        std::wcout << L"Generated: " << sqlparser::generate(ast) << std::endl;
    } else {
        std::wcerr << L"Parse Failed" << std::endl;
    }
    return 0;
}
