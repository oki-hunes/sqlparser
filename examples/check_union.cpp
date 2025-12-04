#include <iostream>
#include <sqlparser/parser.hpp>

void check_is_union(const std::wstring& sql) {
    sqlparser::ast::SelectStatement ast;
    if (sqlparser::parser::parse(sql, ast)) {
        std::wcout << L"SQL: " << sql << std::endl;
        
        // UNION かどうかの判定
        if (!ast.unions.empty()) {
            std::wcout << L"  [RESULT] This is a UNION query." << std::endl;
            std::wcout << L"  [INFO]   Combined queries count: " << ast.unions.size() << std::endl;
            
            for (size_t i = 0; i < ast.unions.size(); ++i) {
                const auto& u = ast.unions[i];
                std::wcout << L"  [INFO]   Union #" << (i + 1) << L": " 
                           << (u.type == sqlparser::ast::SetOperationType::UnionAll ? L"UNION ALL" : L"UNION") 
                           << std::endl;
            }
        } else {
            std::wcout << L"  [RESULT] This is a normal SELECT query (No UNION)." << std::endl;
        }
    } else {
        std::wcerr << L"Parse Failed: " << sql << std::endl;
    }
    std::wcout << L"----------------------------------------" << std::endl;
}

int main() {
    // ケース1: 通常の SELECT 文
    check_is_union(L"SELECT * FROM users WHERE id = 1");

    // ケース2: UNION 文
    check_is_union(L"SELECT id FROM table1 UNION SELECT id FROM table2");

    // ケース3: 複数の UNION (UNION ALL 含む)
    check_is_union(L"SELECT id FROM table1 UNION ALL SELECT id FROM table2 UNION SELECT id FROM table3");

    return 0;
}
