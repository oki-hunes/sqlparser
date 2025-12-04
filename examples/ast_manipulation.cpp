#include <iostream>
#include <sqlparser/parser.hpp>
#include <sqlparser/generator.hpp>

int main() {
    // ベースとなる SQL
    std::wstring sql = L"SELECT id, name FROM users WHERE age > 20";
    std::wcout << L"Original SQL: " << sql << std::endl;

    sqlparser::ast::SelectStatement ast;

    // 1. パース実行
    if (!sqlparser::parser::parse(sql, ast)) {
        std::wcerr << L"Parse Failed" << std::endl;
        return 1;
    }
    std::wcout << L"Parse Success!" << std::endl;

    // 2. カラムエイリアスの変更
    // 先頭のカラム (id) のエイリアスを cust_id に変更
    if (!ast.columns.empty()) {
        ast.columns[0].alias = L"cust_id";
        std::wcout << L"[Mod] Changed first column alias to 'cust_id'" << std::endl;
    }

    // 3. ORDER BY の追加
    // ORDER BY cust_id を追加
    sqlparser::ast::OrderByElement order;
    order.column = L"cust_id";
    order.direction = sqlparser::ast::OrderDirection::ASC;
    ast.orderBy.push_back(order);
    std::wcout << L"[Mod] Added ORDER BY cust_id" << std::endl;

    // 4. WHERE 句への条件追加 (IN 句)
    // 既存の WHERE 句を壊さずに AND cust_id IN (1, 2, 3) を追加

    // IN 式の作成: cust_id IN (1, 2, 3)
    sqlparser::ast::In in_expr;
    in_expr.expr = std::wstring(L"cust_id"); // カラム名
    in_expr.not_in = false;
    in_expr.values.push_back(sqlparser::ast::IntLiteral(1));
    in_expr.values.push_back(sqlparser::ast::IntLiteral(2));
    in_expr.values.push_back(sqlparser::ast::IntLiteral(3));

    if (ast.where) {
        // 既存の WHERE がある場合は AND で結合
        sqlparser::ast::BinaryOp and_op;
        and_op.op = sqlparser::ast::OpType::AND;
        and_op.left = *ast.where;
        and_op.right = in_expr;
        ast.where = and_op;
        std::wcout << L"[Mod] Appended IN condition to WHERE clause" << std::endl;
    } else {
        // ない場合はそのまま設定
        ast.where = in_expr;
        std::wcout << L"[Mod] Set IN condition to WHERE clause" << std::endl;
    }

    // 5. SQL の再生成
    std::wstring generated_sql = sqlparser::generate(ast);
    std::wcout << L"Generated SQL: " << generated_sql << std::endl;

    return 0;
}
