#include <iostream>
#include <vector>
#include <string>
#include <sqlparser/parser.hpp>
#include <sqlparser/generator.hpp>

int main() {
    std::vector<std::wstring> types = {
        L"BIGINT", L"INT8", L"BIGSERIAL", L"SERIAL8", L"BIT", L"BIT(3)", L"VARBIT", L"BIT VARYING",
        L"BOOLEAN", L"BOOL", L"BYTEA", L"CHARACTER(10)", L"CHAR(10)", L"CHARACTER VARYING(255)",
        L"VARCHAR(255)", L"CIDR", L"DATE", L"DOUBLE PRECISION", L"FLOAT8", L"INET",
        L"INTEGER", L"INT", L"INT4", L"INTERVAL", L"INTERVAL YEAR", L"INTERVAL DAY TO SECOND",
        L"JSON", L"JSONB", L"MACADDR", L"MONEY", L"NUMERIC", L"NUMERIC(10,2)",
        L"DECIMAL", L"PG_LSN", L"REAL", L"FLOAT4",
        L"SMALLINT", L"INT2", L"SMALLSERIAL", L"SERIAL2", L"SERIAL", L"SERIAL4",
        L"TEXT", L"TIME", L"TIME(3)", L"TIME WITH TIME ZONE", L"TIMETZ",
        L"TIMESTAMP", L"TIMESTAMP(3)", L"TIMESTAMP WITH TIME ZONE", L"TIMESTAMPTZ",
        L"TSQUERY", L"TSVECTOR", L"TXID_SNAPSHOT", L"UUID", L"XML"
    };

    int failed = 0;
    for (const auto& t : types) {
        std::wstring sql = L"SELECT CAST(col AS " + t + L")";
        sqlparser::ast::SelectStatement ast;
        if (sqlparser::parser::parse(sql, ast)) {
            // std::wcout << L"Success: " << t << std::endl;
        } else {
            std::wcerr << L"Failed: " << t << std::endl;
            failed++;
        }
    }

    if (failed == 0) {
        std::wcout << L"All types parsed successfully!" << std::endl;
    } else {
        std::wcerr << L"Total failures: " << failed << std::endl;
        return 1;
    }

    return 0;
}
