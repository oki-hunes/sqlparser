#include <iostream>
#include <sqlparser/parser.hpp>
#include <sqlparser/generator.hpp>

int main() {
    // 1. ベースとなる2つのクエリ
    std::wstring sql1 = L"SELECT id, name FROM users_2023 WHERE age > 20";
    std::wstring sql2 = L"SELECT id, name FROM users_2024 WHERE age > 20";

    std::wcout << L"Query 1: " << sql1 << std::endl;
    std::wcout << L"Query 2: " << sql2 << std::endl;

    sqlparser::ast::SelectStatement ast1;
    sqlparser::ast::SelectStatement ast2;

    // 2. パース実行
    if (!sqlparser::parser::parse(sql1, ast1) || !sqlparser::parser::parse(sql2, ast2)) {
        std::wcerr << L"Parse Failed" << std::endl;
        return 1;
    }

    // 3. UNION ALL で結合
    // ast1 に ast2 を UNION ALL として追加する
    sqlparser::ast::UnionClause union_clause;
    union_clause.type = sqlparser::ast::SetOperationType::UnionAll;
    union_clause.select = ast2; // ast2 をセット (コピーされます)
    
    ast1.unions.push_back(union_clause);
    std::wcout << L"[Mod] Combined Query 2 into Query 1 using UNION ALL" << std::endl;

    // 4. 全体に対する ORDER BY の追加
    // UNION されたクエリ全体のソート順を指定する場合、通常は最後の SELECT 文に ORDER BY を記述します。
    // ast1.unions の最後の要素の select (recursive_wrapper) にアクセスして変更します。
    
    sqlparser::ast::OrderByElement order;
    order.column = L"id";
    order.direction = sqlparser::ast::OrderDirection::ASC;
    
    // ast1.unions.back().select は boost::recursive_wrapper<SelectStatement>
    // get() で参照を取得して操作します
    ast1.unions.back().select.get().orderBy.push_back(order);
    
    std::wcout << L"[Mod] Added ORDER BY id to the last query" << std::endl;

    // 5. SQL 再生成
    std::wstring generated_sql = sqlparser::generate(ast1);
    std::wcout << L"Generated SQL: " << generated_sql << std::endl;

    return 0;
}
