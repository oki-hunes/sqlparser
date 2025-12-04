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
        EQ, NE, GT, LT, GE, LE, AND, OR
    };

    // ソート順
    enum class OrderDirection {
        ASC, DESC
    };

    // 前方宣言
    struct BinaryOp;
    struct Cast;
    struct FunctionCall;
    struct Case;

    // 式を表すバリアント
    // int: 数値
    // String: 識別子 (カラム名) または 文字列リテラル
    // BinaryOp: 二項演算 (再帰的)
    // Cast: 型変換
    // FunctionCall: 関数呼び出し
    // Case: CASE式
    using Expression = boost::variant<
        int,
        String, 
        boost::recursive_wrapper<BinaryOp>,
        boost::recursive_wrapper<Cast>,
        boost::recursive_wrapper<FunctionCall>,
        boost::recursive_wrapper<Case>
    >;

    // 二項演算構造体
    struct BinaryOp {
        OpType op;
        Expression left;
        Expression right;
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

    // テーブル参照 (テーブル名 または サブクエリ)
    using TableReference = boost::variant<
        String,
        boost::recursive_wrapper<SelectStatement>
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
    };
}

// Boost.Fusion で構造体をアダプト
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::BinaryOp, op, left, right)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::Cast, expr, type_name)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::FunctionCall, name, args)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::WhenClause, when, then)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::Case, arg, when_clauses, else_result)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::ResultColumn, expr, alias)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::OrderByElement, column, direction)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::Join, type, table, on)
BOOST_FUSION_ADAPT_STRUCT(sqlparser::ast::SelectStatement, quantifier, columns, table, joins, where, groupBy, having, orderBy, limit, offset)

