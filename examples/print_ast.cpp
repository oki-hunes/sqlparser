#include <iostream>
#include <string>
#include <sqlparser/parser.hpp>

// ツリー表示用ビジター
// indent: 現在のインデント文字列
// is_last: 親から見て最後の子か (罫線の描き分けに使用)
struct AstPrinter : boost::static_visitor<void> {
    std::wostream& os;
    std::wstring   indent;
    bool           is_last;

    AstPrinter(std::wostream& os, std::wstring indent = L"", bool is_last = true)
        : os(os), indent(indent), is_last(is_last) {}

    // 現在ノードのプレフィックスを返す
    std::wstring prefix() const {
        return indent + (is_last ? L"\\-- " : L"+-- ");
    }

    // 子ノード向けのインデントを返す
    std::wstring child_indent() const {
        return indent + (is_last ? L"    " : L"|   ");
    }

    // 子リストを出力するヘルパー
    template <typename T>
    void print_children(const std::vector<T>& items,
                        const std::wstring& label,
                        std::function<void(const T&, const std::wstring&, bool)> fn) const {
        if (items.empty()) return;
        os << child_indent() << (L"+- " + label) << std::endl;
        for (size_t i = 0; i < items.size(); ++i) {
            fn(items[i], child_indent() + L"|   ", i + 1 == items.size());
        }
    }

    void print_expr(const sqlparser::ast::Expression& expr,
                    const std::wstring& ind, bool last) const;

    void operator()(const sqlparser::ast::IntLiteral& v) const {
        os << prefix() << L"IntLiteral: " << v.value << std::endl;
    }

    void operator()(const sqlparser::String& s) const {
        os << prefix() << L"Identifier: " << s << std::endl;
    }

    void operator()(const sqlparser::ast::StringLiteral& s) const {
        os << prefix() << L"StringLiteral: '" << s.value << L"'" << std::endl;
    }

    void operator()(const sqlparser::ast::BinaryOp& op) const {
        static const wchar_t* names[] = {
            L"=", L"<>", L">", L"<", L">=", L"<=",
            L"AND", L"OR",
            L"+", L"-", L"*", L"/", L"%",
            L"||", L"LIKE", L"NOT",
            L"IS NULL", L"IS NOT NULL",
            L"&", L"|", L"^", L"~", L"<<", L">>"
        };
        os << prefix() << L"BinaryOp: " << names[static_cast<int>(op.op)] << std::endl;
        print_expr(op.left,  child_indent(), false);
        print_expr(op.right, child_indent(), true);
    }

    void operator()(const sqlparser::ast::UnaryOp& op) const {
        static const wchar_t* names[] = {
            L"=", L"<>", L">", L"<", L">=", L"<=",
            L"AND", L"OR",
            L"+", L"-", L"*", L"/", L"%",
            L"||", L"LIKE", L"NOT",
            L"IS NULL", L"IS NOT NULL",
            L"&", L"|", L"^", L"~", L"<<", L">>"
        };
        os << prefix() << L"UnaryOp: " << names[static_cast<int>(op.op)] << std::endl;
        print_expr(op.expr, child_indent(), true);
    }

    void operator()(const sqlparser::ast::Cast& c) const {
        os << prefix() << L"Cast: AS " << c.type_name << std::endl;
        print_expr(c.expr, child_indent(), true);
    }

    void operator()(const sqlparser::ast::FunctionCall& f) const {
        os << prefix() << L"FunctionCall: " << f.name << L"()" << std::endl;
        for (size_t i = 0; i < f.args.size(); ++i)
            print_expr(f.args[i], child_indent(), i + 1 == f.args.size());
    }

    void operator()(const sqlparser::ast::Case& c) const {
        os << prefix() << L"Case" << std::endl;
        std::wstring ci = child_indent();
        if (c.arg) print_expr(*c.arg, ci, false);
        for (size_t i = 0; i < c.when_clauses.size(); ++i) {
            bool last_when = (i + 1 == c.when_clauses.size()) && !c.else_result;
            os << ci + (last_when ? L"\\-- " : L"+-- ") << L"When" << std::endl;
            std::wstring wi = ci + (last_when ? L"    " : L"|   ");
            print_expr(c.when_clauses[i].when, wi, false);
            print_expr(c.when_clauses[i].then, wi, true);
        }
        if (c.else_result) {
            os << ci << L"\\-- Else" << std::endl;
            print_expr(*c.else_result, ci + L"    ", true);
        }
    }

    void operator()(const sqlparser::ast::Between& b) const {
        os << prefix() << L"Between" << (b.not_between ? L" [NOT]" : L"") << std::endl;
        std::wstring ci = child_indent();
        print_expr(b.expr,  ci, false);
        print_expr(b.lower, ci, false);
        print_expr(b.upper, ci, true);
    }

    void operator()(const sqlparser::ast::In& in) const {
        os << prefix() << L"In" << (in.not_in ? L" [NOT]" : L"") << std::endl;
        std::wstring ci = child_indent();
        print_expr(in.expr, ci, in.values.empty());
        for (size_t i = 0; i < in.values.size(); ++i)
            print_expr(in.values[i], ci, i + 1 == in.values.size());
    }

    void operator()(const sqlparser::ast::Exists& e) const {
        os << prefix() << L"Exists" << std::endl;
        std::wstring ci = child_indent();
        os << ci << L"\\-- SelectStatement" << std::endl;
        print_select(e.subquery.get(), ci + L"    ", true);
    }

    void operator()(const sqlparser::ast::WindowFunction& wf) const {
        os << prefix() << L"WindowFunction: " << wf.func.name << std::endl;
        std::wstring ci = child_indent();
        for (size_t i = 0; i < wf.func.args.size(); ++i)
            print_expr(wf.func.args[i], ci, false);
        os << ci << L"\\-- Over" << std::endl;
        std::wstring oi = ci + L"    ";
        if (!wf.window.partitionBy.empty()) {
            os << oi << L"+-- PartitionBy" << std::endl;
            for (size_t i = 0; i < wf.window.partitionBy.size(); ++i)
                print_expr(wf.window.partitionBy[i], oi + L"|   ", i + 1 == wf.window.partitionBy.size());
        }
        if (!wf.window.orderBy.empty()) {
            os << oi << L"\\-- OrderBy" << std::endl;
            for (size_t i = 0; i < wf.window.orderBy.size(); ++i) {
                auto& ob = wf.window.orderBy[i];
                bool last = i + 1 == wf.window.orderBy.size();
                os << oi + L"    " + (last ? L"\\-- " : L"+-- ")
                   << ob.column
                   << (ob.direction == sqlparser::ast::OrderDirection::DESC ? L" DESC" : L" ASC")
                   << std::endl;
            }
        }
    }

    // SelectStatement のツリー表示 (サブクエリ再帰用)
    static void print_select(const sqlparser::ast::SelectStatement& stmt,
                              const std::wstring& ind, bool last);
};

// Expression のディスパッチ
void AstPrinter::print_expr(const sqlparser::ast::Expression& expr,
                             const std::wstring& ind, bool last) const {
    AstPrinter child(os, ind, last);
    boost::apply_visitor(child, expr);
}

// TableReference のツリー表示
static void print_table_ref(const sqlparser::ast::TableReference& ref,
                             const std::wstring& ind, bool last) {
    struct Visitor : boost::static_visitor<void> {
        std::wostream& os;
        std::wstring ind;
        bool last;
        Visitor(std::wostream& os, std::wstring ind, bool last)
            : os(os), ind(ind), last(last) {}
        void operator()(const sqlparser::ast::Table& t) const {
            os << ind + (last ? L"\\-- " : L"+-- ")
               << L"Table: " << t.name;
            if (t.alias) os << L" AS " << *t.alias;
            os << std::endl;
        }
        void operator()(const sqlparser::ast::Subquery& s) const {
            os << ind + (last ? L"\\-- " : L"+-- ") << L"Subquery";
            if (s.alias) os << L" AS " << *s.alias;
            os << std::endl;
            AstPrinter::print_select(s.select.get(), ind + (last ? L"    " : L"|   "), true);
        }
    };
    boost::apply_visitor(Visitor{std::wcout, ind, last}, ref);
}

// SelectStatement のツリー表示 (本体)
void AstPrinter::print_select(const sqlparser::ast::SelectStatement& stmt,
                               const std::wstring& ind, bool /*last*/) {
    auto line = [&](bool last, const std::wstring& text) {
        std::wcout << ind + (last ? L"\\-- " : L"+-- ") << text << std::endl;
    };
    auto child_ind = [&](bool last) -> std::wstring {
        return ind + (last ? L"    " : L"|   ");
    };

    // Quantifier
    if (stmt.quantifier != sqlparser::ast::SelectQuantifier::Default) {
        const wchar_t* q = L"ALL";
        if (stmt.quantifier == sqlparser::ast::SelectQuantifier::Distinct)    q = L"DISTINCT";
        if (stmt.quantifier == sqlparser::ast::SelectQuantifier::DistinctRow) q = L"DISTINCTROW";
        line(false, std::wstring(L"Quantifier: ") + q);
    }

    // Columns
    bool has_from     = true; // always emit
    bool has_where    = stmt.where.has_value();
    bool has_groupby  = !stmt.groupBy.empty();
    bool has_having   = stmt.having.has_value();
    bool has_orderby  = !stmt.orderBy.empty();
    bool has_limit    = stmt.limit.has_value();
    bool has_offset   = stmt.offset.has_value();
    bool has_unions   = !stmt.unions.empty();

    bool cols_last = !has_from && !has_where && !has_groupby &&
                     !has_having && !has_orderby && !has_limit &&
                     !has_offset && !has_unions;

    std::wcout << ind + (cols_last ? L"\\-- " : L"+-- ") << L"Columns" << std::endl;
    std::wstring ci = child_ind(cols_last);
    for (size_t i = 0; i < stmt.columns.size(); ++i) {
        bool last_col = i + 1 == stmt.columns.size();
        auto& rc = stmt.columns[i];
        std::wcout << ci + (last_col ? L"\\-- " : L"+-- ") << L"ResultColumn";
        if (rc.alias) std::wcout << L" AS " << *rc.alias;
        std::wcout << std::endl;
        AstPrinter child(std::wcout, ci + (last_col ? L"    " : L"|   "), true);
        boost::apply_visitor(child, rc.expr);
    }

    // From
    auto remaining_after_from = has_where || has_groupby || has_having ||
                                 has_orderby || has_limit || has_offset || has_unions;
    std::wcout << ind + (!remaining_after_from ? L"\\-- " : L"+-- ") << L"From" << std::endl;
    print_table_ref(stmt.table, ind + (!remaining_after_from ? L"    " : L"|   "), true);

    // Joins
    for (const auto& j : stmt.joins) {
        const wchar_t* jt = L"INNER JOIN";
        if (j.type == sqlparser::ast::JoinType::LEFT)  jt = L"LEFT JOIN";
        if (j.type == sqlparser::ast::JoinType::RIGHT) jt = L"RIGHT JOIN";
        if (j.type == sqlparser::ast::JoinType::FULL)  jt = L"FULL JOIN";
        std::wcout << ind << L"+-- " << jt << std::endl;
        print_table_ref(j.table, ind + L"|   ", false);
        std::wcout << ind << L"|   \\-- On" << std::endl;
        AstPrinter on_p(std::wcout, ind + L"|       ", true);
        boost::apply_visitor(on_p, j.on);
    }

    // Where
    if (has_where) {
        bool where_last = !has_groupby && !has_having && !has_orderby &&
                          !has_limit && !has_offset && !has_unions;
        std::wcout << ind + (where_last ? L"\\-- " : L"+-- ") << L"Where" << std::endl;
        AstPrinter wp(std::wcout, child_ind(where_last), true);
        boost::apply_visitor(wp, *stmt.where);
    }

    // Group By
    if (has_groupby) {
        bool gb_last = !has_having && !has_orderby && !has_limit && !has_offset && !has_unions;
        std::wcout << ind + (gb_last ? L"\\-- " : L"+-- ") << L"GroupBy" << std::endl;
        std::wstring gi = child_ind(gb_last);
        for (size_t i = 0; i < stmt.groupBy.size(); ++i) {
            AstPrinter gp(std::wcout, gi, i + 1 == stmt.groupBy.size());
            boost::apply_visitor(gp, stmt.groupBy[i]);
        }
    }

    // Having
    if (has_having) {
        bool hav_last = !has_orderby && !has_limit && !has_offset && !has_unions;
        std::wcout << ind + (hav_last ? L"\\-- " : L"+-- ") << L"Having" << std::endl;
        AstPrinter hp(std::wcout, child_ind(hav_last), true);
        boost::apply_visitor(hp, *stmt.having);
    }

    // Order By
    if (has_orderby) {
        bool ob_last = !has_limit && !has_offset && !has_unions;
        std::wcout << ind + (ob_last ? L"\\-- " : L"+-- ") << L"OrderBy" << std::endl;
        std::wstring oi = child_ind(ob_last);
        for (size_t i = 0; i < stmt.orderBy.size(); ++i) {
            auto& ob = stmt.orderBy[i];
            bool last_ob = i + 1 == stmt.orderBy.size();
            std::wcout << oi + (last_ob ? L"\\-- " : L"+-- ")
                       << ob.column
                       << (ob.direction == sqlparser::ast::OrderDirection::DESC ? L" DESC" : L" ASC")
                       << std::endl;
        }
    }

    // Limit / Offset
    if (has_limit) {
        bool lim_last = !has_offset && !has_unions;
        std::wcout << ind + (lim_last ? L"\\-- " : L"+-- ") << L"Limit" << std::endl;
        AstPrinter lp(std::wcout, child_ind(lim_last), true);
        boost::apply_visitor(lp, *stmt.limit);
    }
    if (has_offset) {
        bool off_last = !has_unions;
        std::wcout << ind + (off_last ? L"\\-- " : L"+-- ") << L"Offset" << std::endl;
        AstPrinter op2(std::wcout, child_ind(off_last), true);
        boost::apply_visitor(op2, *stmt.offset);
    }

    // Unions
    for (size_t i = 0; i < stmt.unions.size(); ++i) {
        bool last_u = i + 1 == stmt.unions.size();
        auto& u = stmt.unions[i];
        const wchar_t* ut = u.type == sqlparser::ast::SetOperationType::UnionAll
                          ? L"UnionAll" : L"Union";
        std::wcout << ind + (last_u ? L"\\-- " : L"+-- ") << ut << std::endl;
        print_select(u.select.get(), child_ind(last_u), true);
    }
}

// -------------------------------------------------------
// エントリポイント
// -------------------------------------------------------
void dump(const std::wstring& label, const std::wstring& sql) {
    std::wcout << L"=== " << label << L" ===" << std::endl;
    std::wcout << L"SQL: " << sql << std::endl;
    std::wcout << L"AST:" << std::endl;

    sqlparser::ast::SelectStatement ast;
    if (!sqlparser::parser::parse(sql, ast)) {
        std::wcerr << L"  (parse failed)" << std::endl << std::endl;
        return;
    }

    std::wcout << L"SelectStatement" << std::endl;
    AstPrinter::print_select(ast, L"", true);
    std::wcout << std::endl;
}

int main() {
    // 1. シンプルな SELECT
    dump(L"Simple SELECT",
         L"SELECT id, name FROM users WHERE (age > 20) ORDER BY id DESC");

    // 2. JOIN
    dump(L"JOIN",
         L"SELECT u.id, o.total FROM users u INNER JOIN orders o ON (u.id = o.user_id)");

    // 3. CASE / IN
    dump(L"CASE and IN",
         L"SELECT CASE WHEN status = 1 THEN 'active' ELSE 'inactive' END FROM t WHERE (id IN (1, 2, 3))");

    // 4. NOT EXISTS (サブクエリ)
    dump(L"NOT EXISTS",
         L"SELECT S.SHAPEID, 307 FROM GIS.WATER_POINT S WHERE (S.CLASSCD = 1) AND NOT EXISTS (SELECT * FROM GIS.WATER_METER_ATTR A WHERE (S.CODE = A.BASE_CUSTOMER_CD)) ORDER BY S.SHAPEID");

    // 5. UNION
    dump(L"UNION",
         L"SELECT id, name FROM t1 WHERE (status = 1) UNION SELECT id, name FROM t2 WHERE (status = 2)");

    // 6. ウィンドウ関数
    dump(L"Window Function",
         L"SELECT ROW_NUMBER() OVER (PARTITION BY dept ORDER BY salary DESC) FROM emp");

    return 0;
}
