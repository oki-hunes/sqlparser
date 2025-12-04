#pragma once
#include <string>
#include <vector>
#include <boost/variant.hpp>
#include <boost/optional.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <sqlparser/config.hpp>

namespace sqlparser::ast {
    
    // 演算子の種類
    enum class OpType {
        EQ, NE, GT, LT, GE, LE, // 比較: =, !=, >, <, >=, <=
        AND, OR,                // 論理: AND, OR
        ADD, SUB, MUL, DIV, MOD, // 算術: +, -, *, /, %
        CONCAT,                 // 文字列連結: ||
        LIKE,                   // パターンマッチ: LIKE
        NOT,                    // 単項: NOT, !
        IS_NULL, IS_NOT_NULL,   // NULL判定: IS NULL, IS NOT NULL
        BIT_AND, BIT_OR, BIT_XOR, BIT_NOT, // ビット演算: &, |, ^, ~
        BIT_LSHIFT, BIT_RSHIFT  // ビットシフト: <<, >>
    };

    // ソート順
    enum class OrderDirection {
        ASC, DESC
    };

    // 前方宣言
    struct BinaryOp;
    struct UnaryOp;
    struct Cast;
    struct FunctionCall;
    struct Case;
    struct StringLiteral;
    struct IntLiteral;
    struct Between;
    struct In; // Added

    // 式を表すバリアント
    // IntLiteral: 数値
    // String: 識別子 (カラム名)
    // StringLiteral: 文字列リテラル
    // BinaryOp: 二項演算 (再帰的)
    // UnaryOp: 単項演算 (再帰的)
    // Cast: 型変換
    // FunctionCall: 関数呼び出し
    // Case: CASE式
    // Between: BETWEEN式
    // In: IN式
    using Expression = boost::variant<
        IntLiteral,
        String, 
        boost::recursive_wrapper<StringLiteral>,
        boost::recursive_wrapper<BinaryOp>,
        boost::recursive_wrapper<UnaryOp>,
        boost::recursive_wrapper<Cast>,
        boost::recursive_wrapper<FunctionCall>,
        boost::recursive_wrapper<Case>,
        boost::recursive_wrapper<Between>,
        boost::recursive_wrapper<In> // Added
    >;

    // 数値リテラル構造体
    struct IntLiteral {
        int value;
        IntLiteral(int v = 0) : value(v) {}
    };

    // 文字列リテラル構造体
    struct StringLiteral {
        String value;
    };

    // 二項演算構造体
    struct BinaryOp {
        OpType op;
        Expression left;
        Expression right;
    };

    // 単項演算構造体
    struct UnaryOp {
        OpType op;
        Expression expr;
    };

    // CAST式構造体
    struct Cast {
        Expression expr;
        String type_name;
    };

    // 関数呼び出し構造体
    struct FunctionCall {
        String name;
        std::vector<Expression> args;
    };

    // CASE式のWHEN句
    struct WhenClause {
        Expression when;
        Expression then;
    };

    // CASE式構造体
    struct Case {
        boost::optional<Expression> arg;
        std::vector<WhenClause> when_clauses;
        boost::optional<Expression> else_result;
    };

    // BETWEEN式構造体
    struct Between {
        Expression expr;
        Expression lower;
        Expression upper;
        bool not_between; // true if NOT BETWEEN
    };

    // IN式構造体
    struct In {
        Expression expr;
        std::vector<Expression> values;
        bool not_in; // true if NOT IN
    };

    // 選択リストの要素 (式 + オプションのエイリアス)
    struct ResultColumn {
        Expression expr;
        boost::optional<String> alias;
    };

    // ORDER BY 要素
    struct OrderByElement {
        String column;
        OrderDirection direction;
    };

    struct SelectStatement;

    // テーブル (名前 + エイリアス)
    struct Table {
        String name;
        boost::optional<String> alias;
    };

    // サブクエリ (SELECT文 + エイリアス)
    struct Subquery {
        boost::recursive_wrapper<SelectStatement> select;
        boost::optional<String> alias;
    };

    // テーブル参照 (テーブル または サブクエリ)
    using TableReference = boost::variant<
        Table,
        Subquery
    >;

    // JOINの種類
    enum class JoinType {
        INNER, LEFT, RIGHT, FULL
    };

    // JOIN句
    struct Join {
        JoinType type;
        TableReference table;
        Expression on;
    };

    // SELECT修飾子
    enum class SelectQuantifier {
        Default,
        All,
        Distinct,
        DistinctRow
    };

    // 集合演算の種類
    enum class SetOperationType {
        Union,
        UnionAll
    };

    struct SelectStatement;

    // UNION句
    struct UnionClause {
        SetOperationType type;
        boost::recursive_wrapper<SelectStatement> select;
    };

    // TOP句
    // SELECT文を表す構造体
    struct SelectStatement {
        SelectQuantifier quantifier = SelectQuantifier::Default;
        std::vector<ResultColumn> columns; // 取得するカラムのリスト
        TableReference table;        // テーブル参照
        std::vector<Join> joins;     // JOIN句のリスト
        boost::optional<Expression> where; // WHERE句 (省略可能)
        std::vector<Expression> groupBy; // GROUP BY句 (空なら指定なし)
        boost::optional<Expression> having; // HAVING句 (省略可能)
        std::vector<OrderByElement> orderBy; // ORDER BY句 (空なら指定なし)
        boost::optional<Expression> limit; // LIMIT句 (省略可能)
        boost::optional<Expression> offset; // OFFSET句 (省略可能)
        std::vector<UnionClause> unions; // UNION句のリスト
    };
}

// Boost.Fusion で構造体をアダプト
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::Table, name, alias)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::Subquery, select, alias)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::StringLiteral, value)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::BinaryOp, op, left, right)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::UnaryOp, op, expr)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::Cast, expr, type_name)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::FunctionCall, name, args)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::WhenClause, when, then)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::Case, arg, when_clauses, else_result)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::Between, expr, lower, upper, not_between)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::In, expr, values, not_in)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::ResultColumn, expr, alias)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::OrderByElement, column, direction)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::Join, type, table, on)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::UnionClause, type, select)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::SelectStatement, quantifier, columns, table, joins, where, groupBy, having, orderBy, limit, offset, unions)

