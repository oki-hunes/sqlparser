# SQLParser

C++20 と Boost.Spirit X3 を使用して実装された、軽量な SQL パーサーライブラリです。
主に `SELECT` 文の解析と、AST (抽象構文木) からの SQL 再構築をサポートしています。

## 特徴

- **構文解析**: SQL 文字列を解析し、扱いやすい AST 構造体に変換します。
- **SQL 生成**: AST から正規化された SQL 文字列を再生成できます。
- **Unicode 対応**: `std::wstring` (wchar_t) をベースにしており、多言語に対応しています。
- **柔軟性**: テーブル名やカラム名の変更、条件の追加など、AST を操作することでクエリの書き換えが容易です。

## サポートしている構文

現在は `SELECT` 文を中心に以下の機能をサポートしています。

### クエリ構造
- `SELECT` (ALL, DISTINCT, DISTINCTROW)
- `FROM` (テーブル, サブクエリ)
- `JOIN` (INNER, LEFT, RIGHT, FULL) ... ON
- `WHERE`
- `GROUP BY`
- `HAVING`
- `ORDER BY` (ASC, DESC)
- `LIMIT` / `OFFSET`
- `UNION` / `UNION ALL`

### 式・演算子
- **算術演算**: `+`, `-`, `*`, `/`, `%`
- **比較演算**: `=`, `!=`, `<>`, `>`, `<`, `>=`, `<=`, `LIKE`
- **論理演算**: `AND`, `OR`, `NOT`
- **ビット演算**: `&`, `|`, `^`, `<<`, `>>`, `~`
- **文字列演算**: `||` (連結)
- **述語**: `IS NULL`, `IS NOT NULL`, `BETWEEN`, `IN`, `EXISTS`, `NOT EXISTS`
- **その他**:
    - 関数呼び出し: `COUNT(id)`, `MAX(value)` など
    - `CAST(expr AS type)`
    - `CASE` 式
    - `EXISTS (subquery)` / `NOT EXISTS (subquery)`
    - エイリアス (AS)

## 使い方

### 必要要件
- C++20 対応コンパイラ (MSVC 14.50+ 推奨)
- Boost Libraries (1.80.0+ 推奨)
- CMake

### サンプルコード

```cpp
#include <iostream>
#include <sqlparser/parser.hpp>
#include <sqlparser/generator.hpp>

int main() {
    std::wstring sql = L"SELECT id, name FROM users WHERE age > 20 ORDER BY id DESC";
    sqlparser::ast::SelectStatement ast;

    // パース実行
    if (sqlparser::parser::parse(sql, ast)) {
        std::wcout << L"Parse Success!" << std::endl;

        // AST の操作例 (LIMIT を追加)
        ast.limit = sqlparser::ast::IntLiteral(10);

        // SQL の再生成
        std::wstring generated_sql = sqlparser::generate(ast);
        std::wcout << L"Generated: " << generated_sql << std::endl;
    } else {
        std::wcerr << L"Parse Failed" << std::endl;
    }
    return 0;
}
```

## AST 操作の例

パースした AST をプログラムで操作し、SQL を書き換える例です。

### 1. カラムエイリアスの変更

先頭のカラムのエイリアスを `cust_id` に変更します。

```cpp
if (!ast.columns.empty()) {
    ast.columns[0].alias = L"cust_id";
}
```

### 2. ORDER BY の追加

`ORDER BY cust_id` を追加します。

```cpp
sqlparser::ast::OrderByElement order;
order.column = L"cust_id";
order.direction = sqlparser::ast::OrderDirection::ASC;
ast.orderBy.push_back(order);
```

### 3. WHERE 句への条件追加 (IN 句)

既存の WHERE 句を壊さずに `AND cust_id IN (1, 2, 3)` を追加します。

```cpp
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
} else {
    // ない場合はそのまま設定
    ast.where = in_expr;
}
```

### 4. NOT EXISTS によるサブクエリ条件の追加

`AND NOT EXISTS (SELECT * FROM excluded WHERE excluded.id = t.id)` を追加します。

```cpp
// サブクエリ "SELECT * FROM excluded WHERE (excluded.id = t.id)" をパース
std::wstring sub_sql = L"SELECT * FROM excluded WHERE (excluded.id = t.id)";
sqlparser::ast::SelectStatement sub_ast;
sqlparser::parser::parse(sub_sql, sub_ast);

// EXISTS 式の作成
sqlparser::ast::Exists exists_expr;
exists_expr.subquery = sub_ast;

// NOT EXISTS = UnaryOp(NOT, Exists)
sqlparser::ast::UnaryOp not_exists;
not_exists.op = sqlparser::ast::OpType::NOT;
not_exists.expr = exists_expr;

if (ast.where) {
    sqlparser::ast::BinaryOp and_op;
    and_op.op = sqlparser::ast::OpType::AND;
    and_op.left = *ast.where;
    and_op.right = not_exists;
    ast.where = and_op;
} else {
    ast.where = not_exists;
}
```

## ビルド方法

CMake を使用してビルドします。詳細は `about_build.md` を参照してください。

```powershell
mkdir build
cd build
cmake ..
nmake
```

