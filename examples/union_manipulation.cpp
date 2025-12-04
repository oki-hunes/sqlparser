#include <iostream>
#include <sqlparser/parser.hpp>
#include <sqlparser/generator.hpp>

// ヘルパー関数: UNIONチェーンの最後の SELECT 文を取得する
// ORDER BY は通常、最後の SELECT 文に付与するため
sqlparser::ast::SelectStatement& get_last_select(sqlparser::ast::SelectStatement& ast) {
    if (ast.unions.empty()) {
        return ast;
    }
    // 再帰的に最後の要素を探索
    return get_last_select(ast.unions.back().select.get());
}

// ヘルパー関数: すべての SELECT 文に WHERE 条件を追加する
// UNION の各パートでフィルタリングを行いたい場合に有効
void add_where_to_all(sqlparser::ast::SelectStatement& ast, const sqlparser::ast::Expression& expr) {
    // 現在の SELECT 文に条件追加
    if (ast.where) {
        sqlparser::ast::BinaryOp and_op;
        and_op.op = sqlparser::ast::OpType::AND;
        and_op.left = *ast.where;
        and_op.right = expr;
        ast.where = and_op;
    } else {
        ast.where = expr;
    }

    // UNION されている後続の SELECT 文にも再帰的に適用
    for (auto& u : ast.unions) {
        add_where_to_all(u.select.get(), expr);
    }
}

int main() {
    // 元の UNION クエリ
    std::wstring sql = L"SELECT id, name FROM users_2023 UNION ALL SELECT id, name FROM users_2024";
    sqlparser::ast::SelectStatement ast;

    // パース実行
    if (sqlparser::parser::parse(sql, ast)) {
        std::wcout << L"Original SQL: " << sql << std::endl;

        // ---------------------------------------------------------
        // 1. カラムエイリアスの変更
        // UNION の場合、結果セットのカラム名は最初の SELECT 文で決まるため、
        // 先頭の AST のカラムを変更する。
        // ---------------------------------------------------------
        if (!ast.columns.empty()) {
            ast.columns[0].alias = L"user_id";
        }
        std::wcout << L"[Step 1] Changed alias of the first column to 'user_id'." << std::endl;

        // ---------------------------------------------------------
        // 2. ORDER BY の追加
        // UNION クエリ全体のソートを行うには、最後の SELECT 文に ORDER BY を追加する。
        // ---------------------------------------------------------
        sqlparser::ast::SelectStatement& last_stmt = get_last_select(ast);
        
        sqlparser::ast::OrderByElement order;
        order.column = L"user_id"; // エイリアス名を使用（DBによっては元のカラム名が必要な場合もある）
        order.direction = sqlparser::ast::OrderDirection::ASC;
        last_stmt.orderBy.push_back(order);
        
        std::wcout << L"[Step 2] Added ORDER BY 'user_id' to the end of the query." << std::endl;

        // ---------------------------------------------------------
        // 3. WHERE 句への条件追加 (IN 句)
        // UNION の各パートに対して条件を適用する（例: id IN (100, 200)）
        // ---------------------------------------------------------
        sqlparser::ast::In in_expr;
        in_expr.expr = std::wstring(L"id");
        in_expr.not_in = false;
        in_expr.values.push_back(sqlparser::ast::IntLiteral(100));
        in_expr.values.push_back(sqlparser::ast::IntLiteral(200));

        add_where_to_all(ast, in_expr);
        std::wcout << L"[Step 3] Added WHERE id IN (100, 200) to all SELECT parts." << std::endl;

        // SQL の再生成
        std::wstring generated_sql = sqlparser::generate(ast);
        std::wcout << L"Generated SQL: " << generated_sql << std::endl;

    } else {
        std::wcerr << L"Parse Failed" << std::endl;
        return 1;
    }

    return 0;
}
