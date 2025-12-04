#pragma once
#include <sqlparser/ast.hpp>
#include <sstream>
#include <boost/variant/apply_visitor.hpp>

namespace sqlparser {
    
    // Expression を文字列化する Visitor
    struct ExpressionPrinter : boost::static_visitor<void> {
        std::wostream& os;
        ExpressionPrinter(std::wostream& os) : os(os) {}

        void operator()(const ast::IntLiteral& i) const {
            // os << i.value;
            os << i.value; // Reverted debug print idea to keep output clean, but I know it is IntLiteral
        }

        void operator()(const String& s) const {
            os << s;
        }

        void operator()(const ast::StringLiteral& s) const {
            os << L"'" << s.value << L"'";
        }

        void operator()(const ast::UnaryOp& op) const {
            os << L"(";
            switch (op.op) {
                case ast::OpType::NOT: os << L"NOT "; break;
                case ast::OpType::SUB: os << L"-"; break;
                case ast::OpType::BIT_NOT: os << L"~"; break;
                case ast::OpType::IS_NULL: 
                    boost::apply_visitor(*this, op.expr);
                    os << L" IS NULL";
                    os << L")";
                    return;
                case ast::OpType::IS_NOT_NULL:
                    boost::apply_visitor(*this, op.expr);
                    os << L" IS NOT NULL";
                    os << L")";
                    return;
                default: break;
            }
            boost::apply_visitor(*this, op.expr);
            os << L")";
        }

        void operator()(const ast::BinaryOp& op) const {
            // 括弧をつけるかどうかは優先順位によるが、簡易的に常につけるか、
            // ここでは単純に再帰呼び出し
            os << L"(";
            boost::apply_visitor(*this, op.left);
            
            switch (op.op) {
                case ast::OpType::EQ: os << L" = "; break;
                case ast::OpType::NE: os << L" <> "; break;
                case ast::OpType::GT: os << L" > "; break;
                case ast::OpType::LT: os << L" < "; break;
                case ast::OpType::GE: os << L" >= "; break;
                case ast::OpType::LE: os << L" <= "; break;
                case ast::OpType::AND: os << L" AND "; break;
                case ast::OpType::OR:  os << L" OR "; break;
                case ast::OpType::ADD: os << L" + "; break;
                case ast::OpType::SUB: os << L" - "; break;
                case ast::OpType::MUL: os << L" * "; break;
                case ast::OpType::DIV: os << L" / "; break;
                case ast::OpType::MOD: os << L" % "; break;
                case ast::OpType::CONCAT: os << L" || "; break;
                case ast::OpType::LIKE: os << L" LIKE "; break;
                case ast::OpType::BIT_AND: os << L" & "; break;
                case ast::OpType::BIT_OR: os << L" | "; break;
                case ast::OpType::BIT_XOR: os << L" ^ "; break;
                case ast::OpType::BIT_LSHIFT: os << L" << "; break;
                case ast::OpType::BIT_RSHIFT: os << L" >> "; break;
            }

            boost::apply_visitor(*this, op.right);
            os << L")";
        }

        void operator()(const ast::Cast& cast) const {
            os << L"CAST(";
            boost::apply_visitor(*this, cast.expr);
            os << L" AS " << cast.type_name << L")";
        }

        void operator()(const ast::FunctionCall& func) const {
            os << func.name << L"(";
            for (size_t i = 0; i < func.args.size(); ++i) {
                boost::apply_visitor(*this, func.args[i]);
                if (i < func.args.size() - 1) {
                    os << L", ";
                }
            }
            os << L")";
        }

        void operator()(const ast::Case& c) const {
            os << L"CASE";
            if (c.arg) {
                os << L" ";
                boost::apply_visitor(*this, *c.arg);
            }
            for (const auto& w : c.when_clauses) {
                os << L" WHEN ";
                boost::apply_visitor(*this, w.when);
                os << L" THEN ";
                boost::apply_visitor(*this, w.then);
            }
            if (c.else_result) {
                os << L" ELSE ";
                boost::apply_visitor(*this, *c.else_result);
            }
            os << L" END";
        }

        void operator()(const ast::Between& b) const {
            os << L"(";
            boost::apply_visitor(*this, b.expr);
            if (b.not_between) {
                os << L" NOT BETWEEN ";
            } else {
                os << L" BETWEEN ";
            }
            boost::apply_visitor(*this, b.lower);
            os << L" AND ";
            boost::apply_visitor(*this, b.upper);
            os << L")";
        }

        void operator()(const ast::In& in) const {
            os << L"(";
            boost::apply_visitor(*this, in.expr);
            if (in.not_in) {
                os << L" NOT IN (";
            } else {
                os << L" IN (";
            }
            for (size_t i = 0; i < in.values.size(); ++i) {
                boost::apply_visitor(*this, in.values[i]);
                if (i < in.values.size() - 1) {
                    os << L", ";
                }
            }
            os << L"))";
        }
    };

    // 前方宣言
    inline String generate(const ast::SelectStatement& ast);

    // TableReference を文字列化する Visitor
    struct TableReferencePrinter : boost::static_visitor<void> {
        std::wostream& os;
        TableReferencePrinter(std::wostream& os) : os(os) {}

        void operator()(const String& s) const {
            os << s;
        }

        void operator()(const ast::SelectStatement& stmt) const {
            os << L"(" << generate(stmt) << L")";
        }
    };

    // AST から SQL 文字列を生成する関数
    inline String generate(const ast::SelectStatement& ast) {
        std::wstringstream ss;
        ss << L"SELECT ";
        
        if (ast.quantifier == ast::SelectQuantifier::All) ss << L"ALL ";
        else if (ast.quantifier == ast::SelectQuantifier::Distinct) ss << L"DISTINCT ";
        else if (ast.quantifier == ast::SelectQuantifier::DistinctRow) ss << L"DISTINCTROW ";

        ExpressionPrinter exprPrinter(ss);

        for (size_t i = 0; i < ast.columns.size(); ++i) {
            boost::apply_visitor(exprPrinter, ast.columns[i].expr);
            if (ast.columns[i].alias) {
                ss << L" AS " << *ast.columns[i].alias;
            }
            if (i < ast.columns.size() - 1) {
                ss << L", ";
            }
        }
        
        ss << L" FROM ";
        TableReferencePrinter tablePrinter(ss);
        boost::apply_visitor(tablePrinter, ast.table);

        // JOIN句の生成
        for (const auto& join : ast.joins) {
            switch (join.type) {
                case ast::JoinType::INNER: ss << L" INNER JOIN "; break;
                case ast::JoinType::LEFT:  ss << L" LEFT JOIN "; break;
                case ast::JoinType::RIGHT: ss << L" RIGHT JOIN "; break;
                case ast::JoinType::FULL:  ss << L" FULL JOIN "; break;
            }
            boost::apply_visitor(tablePrinter, join.table);
            ss << L" ON ";
            boost::apply_visitor(exprPrinter, join.on);
        }

        if (ast.where) {
            ss << L" WHERE ";
            boost::apply_visitor(exprPrinter, *ast.where);
        }

        if (!ast.groupBy.empty()) {
            ss << L" GROUP BY ";
            for (size_t i = 0; i < ast.groupBy.size(); ++i) {
                boost::apply_visitor(exprPrinter, ast.groupBy[i]);
                if (i < ast.groupBy.size() - 1) {
                    ss << L", ";
                }
            }
        }

        if (ast.having) {
            ss << L" HAVING ";
            boost::apply_visitor(exprPrinter, *ast.having);
        }

        if (!ast.orderBy.empty()) {
            ss << L" ORDER BY ";
            for (size_t i = 0; i < ast.orderBy.size(); ++i) {
                ss << ast.orderBy[i].column;
                if (ast.orderBy[i].direction == ast::OrderDirection::DESC) {
                    ss << L" DESC";
                }
                if (i < ast.orderBy.size() - 1) {
                    ss << L", ";
                }
            }
        }

        if (ast.limit) {
            ss << L" LIMIT ";
            boost::apply_visitor(exprPrinter, *ast.limit);
        }

        if (ast.offset) {
            ss << L" OFFSET ";
            boost::apply_visitor(exprPrinter, *ast.offset);
        }

        // UNION句の生成
        for (const auto& u : ast.unions) {
            if (u.type == ast::SetOperationType::UnionAll) {
                ss << L" UNION ALL ";
            } else {
                ss << L" UNION ";
            }
            // 再帰呼び出し
            // boost::recursive_wrapper は暗黙的にキャストされるか、get() でアクセス
            ss << generate(u.select.get());
        }

        return ss.str();
    }
}
