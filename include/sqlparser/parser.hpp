#pragma once
#include <cwctype> // Added for std::towupper
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/char/unicode.hpp>
#include <boost/spirit/home/support/char_encoding/standard_wide.hpp>
#include <boost/fusion/adapted/std_tuple.hpp> // std::tuple を Fusion sequence として扱うために必要
#include <sqlparser/ast.hpp>
#include <sqlparser/config.hpp>
#include <boost/fusion/include/adapt_struct.hpp> // Added for BOOST_FUSION_ADAPT_STRUCT

namespace sqlparser::parser {
    namespace x3 = boost::spirit::x3;

    // 文字コード対応 (Unicode)
    using x3::unicode::char_;
    using x3::unicode::alnum;
    using x3::unicode::space;

    // wchar_t 用の symbols
    template <typename T>
    using wide_symbols = x3::symbols_parser<boost::spirit::char_encoding::standard_wide, T>;

    // --- 基本ルール ---

    // 予約語
    auto const select_kw = x3::no_case[x3::lit(L"SELECT")];
    auto const from_kw   = x3::no_case[x3::lit(L"FROM")];
    auto const where_kw  = x3::no_case[x3::lit(L"WHERE")];
    auto const order_kw  = x3::no_case[x3::lit(L"ORDER")];
    
    auto debug_by = [](auto& ctx) {
        std::wcout << L"Debug: matched BY" << std::endl;
    };
    auto const by_kw     = x3::lit(L"BY")[debug_by];

    // Keywords to exclude from identifiers
    struct keywords_table : wide_symbols<x3::unused_type> {
        keywords_table() {
            add
                (L"SELECT", x3::unused)
                (L"FROM", x3::unused)
                (L"WHERE", x3::unused)
                (L"ORDER", x3::unused)
                (L"BY", x3::unused)
                (L"AS", x3::unused)
                (L"AND", x3::unused)
                (L"OR", x3::unused)
                (L"CASE", x3::unused)
                (L"WHEN", x3::unused)
                (L"THEN", x3::unused)
                (L"ELSE", x3::unused)
                (L"END", x3::unused)
                (L"CAST", x3::unused)
                (L"INT", x3::unused)
                (L"LIKE", x3::unused)
                ;
        }
    } const keywords;

    // 識別子: 英数字とアンダースコア、ドット (テーブル修飾用)
    x3::rule<class identifier_class, std::wstring> const identifier = "identifier";
    auto const identifier_def = x3::raw[x3::lexeme[ 
        !(x3::no_case[keywords] >> !(alnum | char_(L'_') | char_(L'.')))
        >> +(alnum | char_(L'_') | char_(L'.')) 
    ]] [ ([](auto& ctx){ 
        auto& range = x3::_attr(ctx);
        std::wstring str(range.begin(), range.end());
        x3::_val(ctx) = str;
    }) ];
    BOOST_SPIRIT_DEFINE(identifier);

    // 前方宣言
    // x3::rule<class expression_class, ast::Expression> const expression; // 削除: 定義と重複するため

    // --- 式 (Expression) のルール ---

    // 演算子シンボル
    struct op_table : wide_symbols<ast::OpType> {
        op_table() {
            add
                (L"=",  ast::OpType::EQ)
                (L"!=", ast::OpType::NE)
                (L"<>", ast::OpType::NE)
                (L">",  ast::OpType::GT)
                (L"<",  ast::OpType::LT)
                (L">=", ast::OpType::GE)
                (L"<=", ast::OpType::LE)
                (L"LIKE", ast::OpType::LIKE)
            ;
        }
    } const op_symbol;

    struct additive_op_table : wide_symbols<ast::OpType> {
        additive_op_table() {
            add
                (L"||", ast::OpType::CONCAT)
                (L"+", ast::OpType::ADD)
                (L"-", ast::OpType::SUB)
            ;
        }
    } const additive_op;

    struct multiplicative_op_table : wide_symbols<ast::OpType> {
        multiplicative_op_table() {
            add
                (L"*", ast::OpType::MUL)
                (L"/", ast::OpType::DIV)
                (L"%", ast::OpType::MOD)
            ;
        }
    } const multiplicative_op;

    struct bitwise_op_table : wide_symbols<ast::OpType> {
        bitwise_op_table() {
            add
                (L"&", ast::OpType::BIT_AND)
                (L"|", ast::OpType::BIT_OR)
                (L"^", ast::OpType::BIT_XOR)
                (L"<<", ast::OpType::BIT_LSHIFT)
                (L">>", ast::OpType::BIT_RSHIFT)
            ;
        }
    } const bitwise_op;

    struct unary_op_table : wide_symbols<ast::OpType> {
        unary_op_table() {
            add
                (L"!", ast::OpType::NOT)
                (L"NOT", ast::OpType::NOT)
                (L"-", ast::OpType::SUB) // Unary Minus
                (L"~", ast::OpType::BIT_NOT) // Bitwise NOT
            ;
        }
    } const unary_op;

    // 式の再帰定義のためのルール宣言
    x3::rule<class expression_class, ast::Expression> const expression = "expression";
    x3::rule<class term_class, ast::Expression>       const term       = "term";
    x3::rule<class factor_class, ast::Expression>     const factor     = "factor";
    x3::rule<class null_predicate_class, ast::Expression> const null_predicate = "null_predicate";
    x3::rule<class bitwise_class, ast::Expression>    const bitwise    = "bitwise";
    x3::rule<class sum_class, ast::Expression>        const sum        = "sum";
    x3::rule<class product_class, ast::Expression>    const product    = "product";
    x3::rule<class unary_class, ast::Expression>      const unary      = "unary";
    x3::rule<class primary_class, ast::Expression>    const primary    = "primary";
    x3::rule<class cast_class, ast::Cast>             const cast_expr  = "cast_expr";
    x3::rule<class function_call_class, ast::FunctionCall> const function_call = "function_call";
    x3::rule<class case_class, ast::Case>             const case_expr  = "case_expr";
    x3::rule<class when_clause_class, ast::WhenClause> const when_clause = "when_clause";

    // CAST式
    auto const cast_kw = x3::no_case[x3::lit(L"CAST")];
    auto const as_kw = x3::no_case[x3::lit(L"AS")];
    
    // 型名: 識別子 + オプションで (数値)
    // 例: INT, VARCHAR(255)
    // ここでは簡易的に identifier または identifier(int) を文字列として取得する
    auto const type_name_def = 
        x3::lexeme[ identifier >> -(L'(' >> x3::int_ >> L')') ];

    auto const cast_expr_def = 
        cast_kw >> L'(' >> expression >> as_kw >> identifier >> L')';
    
    // 関数呼び出し
    // identifier ( args... )
    auto const function_call_def = 
        identifier >> L'(' >> (expression % L',') >> L')';

    // CASE式
    auto const case_kw = x3::no_case[x3::lit(L"CASE")];
    auto const when_kw = x3::no_case[x3::lit(L"WHEN")];
    auto const then_kw = x3::no_case[x3::lit(L"THEN")];
    auto const else_kw = x3::no_case[x3::lit(L"ELSE")];
    auto const end_kw  = x3::no_case[x3::lit(L"END")];

    auto const when_clause_def = 
        when_kw >> expression >> then_kw >> expression;

    auto const case_expr_def = 
        case_kw 
        >> -expression 
        >> +when_clause 
        >> -(else_kw >> expression) 
        >> end_kw;

    BOOST_SPIRIT_DEFINE(cast_expr, function_call, case_expr, when_clause);

    // 文字列リテラル: '...'
    x3::rule<class string_literal_class, ast::StringLiteral> const string_literal = "string_literal";
    auto const string_literal_def = x3::lexeme[L"'" >> *(x3::standard_wide::char_ - L"'") >> L"'"];
    BOOST_SPIRIT_DEFINE(string_literal);

    // 数値リテラル
    x3::rule<class int_literal_class, ast::IntLiteral> const int_literal = "int_literal";
    auto const int_literal_def = x3::int_ [ ([](auto& ctx){ 
        x3::_val(ctx) = ast::IntLiteral(x3::_attr(ctx)); 
    }) ];
    BOOST_SPIRIT_DEFINE(int_literal);

    // primary: 数値 | CAST式 | 関数呼び出し | 識別子 | * | (式)
    // 注意: function_call は identifier で始まるため、identifier より先に記述する必要がある
    // "*" を追加して SELECT * に対応
    auto const primary_def = 
        int_literal [ ([](auto& ctx){ x3::_val(ctx) = ast::Expression(x3::_attr(ctx)); }) ]
        | cast_expr [ ([](auto& ctx){ x3::_val(ctx) = ast::Expression(x3::_attr(ctx)); }) ]
        | function_call [ ([](auto& ctx){ x3::_val(ctx) = ast::Expression(x3::_attr(ctx)); }) ]
        | case_expr [ ([](auto& ctx){ x3::_val(ctx) = ast::Expression(x3::_attr(ctx)); }) ]
        | identifier [ ([](auto& ctx){ x3::_val(ctx) = ast::Expression(x3::_attr(ctx)); }) ]
        | string_literal [ ([](auto& ctx){ x3::_val(ctx) = ast::Expression(x3::_attr(ctx)); }) ]
        | (x3::lit(L"*") >> x3::attr(std::wstring(L"*"))) [ ([](auto& ctx){ x3::_val(ctx) = ast::Expression(x3::_attr(ctx)); }) ]
        | (L'(' >> expression >> L')') [ ([](auto& ctx){ x3::_val(ctx) = x3::_attr(ctx); }) ]
        ;

    // unary: 単項演算子 (前置)
    auto make_unary_op = [](auto& ctx) {
        using boost::fusion::at_c;
        auto& attr = x3::_attr(ctx); // tuple<OpType, Expression>
        
        ast::UnaryOp op_node;
        op_node.op = at_c<0>(attr);
        op_node.expr = at_c<1>(attr);
        
        x3::_val(ctx) = op_node;
    };

    auto const unary_def = 
        (unary_op >> unary) [make_unary_op]
        | primary [ ([](auto& ctx){ x3::_val(ctx) = x3::_attr(ctx); }) ];

    // null_predicate: IS NULL / IS NOT NULL / BETWEEN (後置)
    // sum の後に適用される

    // Between の引数構造体
    struct BetweenArgs {
        bool not_between;
        ast::Expression lower;
        ast::Expression upper;
    };

    // In の引数構造体
    struct InArgs {
        bool not_in;
        std::vector<ast::Expression> values;
    };

    // SuffixOp: IS NULL (OpType) または BETWEEN (BetweenArgs) または IN (InArgs)
    using SuffixOp = boost::variant<ast::OpType, BetweenArgs, InArgs>;

    auto make_suffix_op = [](auto& ctx) {
        using boost::fusion::at_c;
        auto& attr = x3::_attr(ctx); // tuple<Expression, vector<SuffixOp>>
        
        auto& first = at_c<0>(attr);
        auto& rest = at_c<1>(attr);

        if (rest.empty()) {
            x3::_val(ctx) = first;
        } else {
            ast::Expression current = first;
            for (auto& op_variant : rest) {
                if (auto* op_type = boost::get<ast::OpType>(&op_variant)) {
                    // IS NULL / IS NOT NULL
                    ast::UnaryOp op_node;
                    op_node.op = *op_type;
                    op_node.expr = current;
                    current = op_node;
                } else if (auto* between_args = boost::get<BetweenArgs>(&op_variant)) {
                    // BETWEEN
                    ast::Between between_node;
                    between_node.expr = current;
                    between_node.lower = between_args->lower;
                    between_node.upper = between_args->upper;
                    between_node.not_between = between_args->not_between;
                    current = between_node;
                } else if (auto* in_args = boost::get<InArgs>(&op_variant)) {
                    // IN
                    ast::In in_node;
                    in_node.expr = current;
                    in_node.values = in_args->values;
                    in_node.not_in = in_args->not_in;
                    current = in_node;
                }
            }
            x3::_val(ctx) = current;
        }
    };

    x3::rule<class is_null_op_class, ast::OpType> const is_null_op = "is_null_op";
    auto const is_null_op_def = 
        (x3::no_case[x3::lit(L"IS") >> x3::lit(L"NOT") >> x3::lit(L"NULL")] >> x3::attr(ast::OpType::IS_NOT_NULL))
        | (x3::no_case[x3::lit(L"IS") >> x3::lit(L"NULL")] >> x3::attr(ast::OpType::IS_NULL));

    x3::rule<class between_op_class, BetweenArgs> const between_op = "between_op";
    auto const between_op_def = 
        (x3::no_case[x3::lit(L"NOT") >> x3::lit(L"BETWEEN")] >> x3::attr(true) >> sum >> x3::no_case[x3::lit(L"AND")] >> sum)
        | (x3::no_case[x3::lit(L"BETWEEN")] >> x3::attr(false) >> sum >> x3::no_case[x3::lit(L"AND")] >> sum);

    x3::rule<class in_op_class, InArgs> const in_op = "in_op";
    auto const in_op_def = 
        (x3::no_case[x3::lit(L"NOT") >> x3::lit(L"IN")] >> x3::attr(true) 
         | x3::no_case[x3::lit(L"IN")] >> x3::attr(false))
        >> x3::lit(L"(") >> (expression % L',') >> x3::lit(L")");

    x3::rule<class suffix_op_class, SuffixOp> const suffix_op = "suffix_op";
    auto const suffix_op_def = is_null_op | between_op | in_op;

    auto const null_predicate_def = 
        (bitwise >> *suffix_op) [make_suffix_op];

    BOOST_SPIRIT_DEFINE(is_null_op, between_op, in_op, suffix_op);

    // BOOST_SPIRIT_DEFINE(null_predicate); // Moved to the main BOOST_SPIRIT_DEFINE

    // ヘルパー: BinaryOp を構築するアクション
    auto make_binary_op = [](auto& ctx) {
        using boost::fusion::at_c;
        auto& attr = x3::_attr(ctx); // tuple<Expression, vector<tuple<OpType, Expression>>>
        
        auto& first = at_c<0>(attr);
        auto& rest = at_c<1>(attr);

        if (rest.empty()) {
            x3::_val(ctx) = first;
        } else {
            ast::Expression current = first;
            for (auto& item : rest) {
                ast::BinaryOp op_node;
                op_node.left = current;
                op_node.op = at_c<0>(item);
                op_node.right = at_c<1>(item);
                current = op_node;
            }
            x3::_val(ctx) = current;
        }
    };

    auto const product_def = 
        (unary >> *(multiplicative_op >> unary)) [make_binary_op];

    auto const sum_def = 
        (product >> *(additive_op >> product)) [make_binary_op];

    auto const bitwise_def = 
        (sum >> *(bitwise_op >> sum)) [make_binary_op];

    // factor: 比較演算 (null_predicate op null_predicate)
    // 属性: tuple<Expression, vector<tuple<OpType, Expression>>>
    auto const factor_def = 
        (null_predicate >> *(op_symbol >> null_predicate)) [make_binary_op];

    // term: AND 演算
    auto make_and_op = [](auto& ctx) {
        using boost::fusion::at_c;
        auto& attr = x3::_attr(ctx);
        auto& first = at_c<0>(attr);
        auto& rest = at_c<1>(attr);

        if (rest.empty()) {
            x3::_val(ctx) = first;
        } else {
            ast::Expression current = first;
            for (auto& item : rest) {
                ast::BinaryOp op_node;
                op_node.op = ast::OpType::AND;
                op_node.left = current;
                op_node.right = item;
                current = op_node;
            }
            x3::_val(ctx) = current;
        }
    };

    auto const term_def = 
        (factor >> *(x3::no_case[x3::lit(L"AND")] >> factor)) [make_and_op];

    // expression: OR 演算
    auto make_or_op = [](auto& ctx) {
        using boost::fusion::at_c;
        auto& attr = x3::_attr(ctx);
        auto& first = at_c<0>(attr);
        auto& rest = at_c<1>(attr);

        if (rest.empty()) {
            x3::_val(ctx) = first;
        } else {
            ast::Expression current = first;
            for (auto& item : rest) {
                ast::BinaryOp op_node;
                op_node.op = ast::OpType::OR;
                op_node.left = current;
                op_node.right = item;
                current = op_node;
            }
            x3::_val(ctx) = current;
        }
    };

    auto const expression_def = 
        (term >> *(x3::no_case[x3::lit(L"OR")] >> term)) [make_or_op];

    BOOST_SPIRIT_DEFINE(expression, term, factor, null_predicate, bitwise, sum, product, unary, primary);

    // --- ResultColumn (expression の後に定義) ---
    x3::rule<class result_column_class, ast::ResultColumn> const result_column = "result_column";
    auto const as_kw_opt = x3::no_case[x3::lit(L"AS")];
    
    auto const result_column_def = 
        expression >> -(as_kw_opt >> identifier);
    
    BOOST_SPIRIT_DEFINE(result_column);

    // カラムリスト
    auto const column_list = result_column % L',';

    // --- SELECT文 ---

    // WHERE句: "WHERE" expression
    auto const where_clause = 
        x3::omit[where_kw] >> expression;

    // ORDER BY 句
    struct order_direction_table : x3::symbols_parser<boost::spirit::char_encoding::standard_wide, ast::OrderDirection> {
        order_direction_table() {
            add
                (L"ASC", ast::OrderDirection::ASC)
                (L"DESC", ast::OrderDirection::DESC)
            ;
        }
    } const order_direction;

    x3::rule<class order_by_element_class, ast::OrderByElement> const order_by_element = "order_by_element";

    auto const direction_opt = 
        (order_direction) 
        | x3::attr(ast::OrderDirection::ASC);

    auto const order_by_element_def = 
        identifier >> direction_opt;

    BOOST_SPIRIT_DEFINE(order_by_element);

    // ORDER BY リスト (ORDER BY キーワードは含まない)
    auto const order_by_list = 
        (order_by_element % L',');

    // GROUP BY リスト
    auto const group_by_list = 
        (expression % L',');

    // Quantifier
    auto const select_quantifier = 
        (x3::no_case[x3::lit(L"ALL")] >> x3::attr(ast::SelectQuantifier::All)) |
        (x3::no_case[x3::lit(L"DISTINCTROW")] >> x3::attr(ast::SelectQuantifier::DistinctRow)) |
        (x3::no_case[x3::lit(L"DISTINCT")] >> x3::attr(ast::SelectQuantifier::Distinct));

    // --- 構造解析ロジック ---

    // 大文字小文字を無視して文字列比較を行うヘルパー
    inline bool is_iequal(std::wstring const& s1, std::wstring const& s2) {
        if (s1.size() != s2.size()) return false;
        for (size_t i = 0; i < s1.size(); ++i) {
            if (std::towupper(s1[i]) != std::towupper(s2[i])) return false;
        }
        return true;
    }

    // 大文字小文字を無視してキーワードを探す (括弧を考慮)
    inline size_t find_keyword(std::wstring const& sql, std::wstring const& kw, size_t start_pos = 0) {
        size_t pos = start_pos;
        size_t len = sql.length();
        size_t kw_len = kw.length();
        int paren_depth = 0;

        while (pos < len) {
            wchar_t c = sql[pos];
            if (c == L'(') {
                paren_depth++;
            } else if (c == L')') {
                if (paren_depth > 0) paren_depth--;
            } else if (paren_depth == 0) {
                // キーワードチェック
                if (pos + kw_len <= len) {
                    bool match = true;
                    for (size_t i = 0; i < kw_len; ++i) {
                        if (std::towupper(sql[pos + i]) != std::towupper(kw[i])) {
                            match = false;
                            break;
                        }
                    }
                    if (match) return pos;
                }
            }
            pos++;
        }
        return std::wstring::npos;
    }

    // テーブル参照をパースするヘルパー関数
    inline bool parse_table_reference(std::wstring const& sql, ast::TableReference& table_ref) {
        auto begin = sql.begin();
        auto end = sql.end();
        
        // 前後の空白をスキップ
        while (begin != end && std::iswspace(*begin)) ++begin;
        auto real_end = end;
        while (real_end != begin && std::iswspace(*(real_end - 1))) --real_end;

        if (begin != real_end && *begin == L'(') {
            // サブクエリ
            // 最後の ')' を探す
            auto rparen_it = real_end - 1;
            while (rparen_it != begin && *rparen_it != L')') --rparen_it;
            
            if (*rparen_it == L')') {
                 std::wstring subquery_sql(begin + 1, rparen_it);
                 ast::SelectStatement sub_stmt;
                 // 前方宣言された parse を呼び出す必要があるが、inline 定義されているため
                 // ここで再帰呼び出しを行うには parse のプロトタイプ宣言が必要。
                 // しかし、C++では定義済みの関数は呼び出せる。
                 // parse はこの関数の後に定義されるため、この関数を parse の前に置くか、
                 // parse 内でラムダを使うか、構造を変える必要がある。
                 // ここでは parse を呼び出したいが、まだ定義されていない。
                 // 解決策: parse_table_reference を parse の内部関数にするか、
                 // parse のプロトタイプを前に置く。
                 // ここでは parse のプロトタイプを前に置くことにする。
                 return false; // プレースホルダー。実際には parse を呼び出す。
            } else {
                return false;
            }
        } else {
            // 通常のテーブル名
            std::wstring table_part(begin, real_end);
            auto t_begin = table_part.begin();
            auto t_end = table_part.end();
            std::wstring table_name;
            if (!x3::phrase_parse(t_begin, t_end, identifier, x3::unicode::space, table_name)) return false;
            ast::Table t;
            t.name = table_name;
            table_ref = t;
            return true;
        }
    }

    // プロトタイプ宣言
    inline bool parse(std::wstring const& sql, ast::SelectStatement& ast);

    // 実装を修正した parse_table_reference
    inline bool parse_table_reference_impl(std::wstring const& sql, ast::TableReference& table_ref) {
        auto begin = sql.begin();
        auto end = sql.end();
        
        while (begin != end && std::iswspace(*begin)) ++begin;
        auto real_end = end;
        while (real_end != begin && std::iswspace(*(real_end - 1))) --real_end;

        // エイリアスのパース
        // 末尾から識別子を探す
        // ただし、サブクエリの場合は ')' の後にあるかもしれない
        // テーブル名の場合は "table alias" または "table AS alias"
        
        // 簡易実装: 末尾の単語をエイリアス候補としてチェック
        // ただし、AS がある場合もある
        
        // ここでは x3::phrase_parse を使って、
        // (subquery | identifier) >> -( [AS] identifier )
        // のようにパースしたいが、sql は文字列なので、
        // 文字列全体をパースするルールを作るのが良い。

        // サブクエリのルール
        // '(' >> ... >> ')'
        // これは再帰呼び出しが必要なので、x3::rule だけでは難しい（parse関数を呼びたい）
        // しかし、parse関数は SelectStatement を返す。
        
        // 戦略:
        // 1. 文字列全体をスキャンして、サブクエリかテーブル名か判断する
        // 2. エイリアス部分を分離する

        // エイリアス分離ロジック
        // 後ろから見て、識別子があればエイリアスの可能性
        // その前に AS があれば確定
        // サブクエリの場合は ')' の後ろ
        
        // もっと単純に、x3::parse で構造を定義する
        
        // テーブル名 + エイリアス
        auto const as_kw = x3::no_case[x3::lit(L"AS")];
        auto const alias_def = -(as_kw) >> identifier;
        
        // テーブル名のみのパース
        // identifier >> -alias_def
        
        // しかし、サブクエリの中身は parse() で処理したい。
        
        // 手順:
        // 1. 先頭が '(' ならサブクエリ
        if (begin != real_end && *begin == L'(') {
            // 対応する閉じ括弧を探す
            int paren_level = 0;
            auto it = begin;
            auto subquery_end = end;
            
            for (; it != end; ++it) {
                if (*it == L'(') paren_level++;
                else if (*it == L')') {
                    paren_level--;
                    if (paren_level == 0) {
                        subquery_end = it + 1; // ')' の次
                        break;
                    }
                }
            }
            
            if (subquery_end == end && paren_level > 0) return false; // 括弧が閉じていない
            
            // サブクエリ部分
            std::wstring subquery_sql(begin + 1, subquery_end - 1);
            ast::SelectStatement sub_stmt;
            if (!parse(subquery_sql, sub_stmt)) return false;
            
            ast::Subquery sub_node;
            sub_node.select = sub_stmt;
            
            // エイリアス部分
            auto alias_begin = subquery_end;
            while (alias_begin != end && std::iswspace(*alias_begin)) ++alias_begin;
            
            if (alias_begin != end) {
                std::wstring alias_part(alias_begin, end);
                std::wstring alias;
                auto a_begin = alias_part.begin();
                auto a_end = alias_part.end();
                if (x3::phrase_parse(a_begin, a_end, alias_def, x3::unicode::space, alias)) {
                    sub_node.alias = alias;
                }
            }
            
            table_ref = sub_node;
            return true;
        } else {
            // テーブル名 + エイリアス
            // identifier >> -( [AS] identifier )
            
            struct TableInfo {
                std::wstring name;
                boost::optional<std::wstring> alias;
            };
            
            auto const table_parser = 
                identifier >> -alias_def;
                
            // std::tuple<std::wstring, boost::optional<std::wstring>>
            std::tuple<std::wstring, boost::optional<std::wstring>> attr;
            
            auto t_begin = begin;
            auto t_end = real_end; // real_end はイテレータ
            
            // phrase_parse は入力イテレータを進める
            if (!x3::phrase_parse(t_begin, t_end, table_parser, x3::unicode::space, attr)) return false;
            
            // 最後までパースできたか確認 (phrase_parse はスキップ後の位置を返す)
            if (t_begin != t_end) return false; 
            
            ast::Table table_node;
            table_node.name = std::get<0>(attr);
            table_node.alias = std::get<1>(attr);
            
            table_ref = table_node;
            return true;
        }
    }

    // SQL全体をパースする関数
    // 再帰的に呼び出されるため、UNIONの処理もここで行う
    inline bool parse(std::wstring const& sql, ast::SelectStatement& ast) {
        // UNION の分割
        // 括弧内の UNION は無視する必要があるため、単純な find_keyword では不十分かもしれないが、
        // ここではトップレベルの UNION を探す簡易的な実装とする。
        // ただし、サブクエリ内の UNION にマッチしないように注意が必要。
        // 括弧のネストレベルを考慮して分割する。

        size_t union_pos = std::wstring::npos;
        ast::SetOperationType union_type = ast::SetOperationType::Union;
        size_t union_len = 0;

        int paren_level = 0;
        for (size_t i = 0; i < sql.length(); ++i) {
            if (sql[i] == L'(') paren_level++;
            else if (sql[i] == L')') paren_level--;
            else if (paren_level == 0) {
                // UNION を探す
                // " UNION "
                if (i + 7 <= sql.length() && is_iequal(sql.substr(i, 7), L" UNION ")) {
                    union_pos = i;
                    union_len = 7;
                    union_type = ast::SetOperationType::Union;
                    
                    // UNION ALL チェック
                    if (i + 11 <= sql.length() && is_iequal(sql.substr(i, 11), L" UNION ALL ")) {
                        union_len = 11;
                        union_type = ast::SetOperationType::UnionAll;
                    }
                    break; // 最初の UNION で分割
                }
            }
        }

        if (union_pos != std::wstring::npos) {
            // UNION が見つかった場合
            std::wstring left_sql = sql.substr(0, union_pos);
            std::wstring right_sql = sql.substr(union_pos + union_len);

            // 左側をパース (再帰)
            if (!parse(left_sql, ast)) return false;

            // 右側をパース (再帰)
            ast::SelectStatement right_ast;
            if (!parse(right_sql, right_ast)) return false;

            // カラム数チェック
            if (ast.columns.size() != right_ast.columns.size()) {
                std::wcerr << L"Error: UNION column count mismatch. Left: " << ast.columns.size() << L", Right: " << right_ast.columns.size() << std::endl;
                return false;
            }

            // UNION句を追加
            ast::UnionClause union_clause;
            union_clause.type = union_type;
            union_clause.select = right_ast;
            ast.unions.push_back(union_clause);

            return true;
        }

        // UNION がない場合、通常の SELECT 文としてパース
        // 1. キーワードの位置を特定する
        size_t select_pos = find_keyword(sql, L"SELECT");
        if (select_pos != 0) return false;

        size_t from_pos = find_keyword(sql, L" FROM ", select_pos);
        if (from_pos == std::wstring::npos) return false;

        size_t where_pos = find_keyword(sql, L" WHERE ", from_pos);
        size_t group_by_pos = find_keyword(sql, L" GROUP BY ", from_pos);
        size_t having_pos = find_keyword(sql, L" HAVING ", from_pos);
        size_t order_by_pos = find_keyword(sql, L" ORDER BY ", from_pos);
        size_t limit_pos = find_keyword(sql, L" LIMIT ", from_pos);
        size_t offset_pos = find_keyword(sql, L" OFFSET ", from_pos);

        // 各セクションの終了位置を計算するヘルパー
        auto get_end_pos = [&](size_t start, std::initializer_list<size_t> candidates) {
            size_t end = sql.length();
            for (size_t pos : candidates) {
                if (pos != std::wstring::npos && pos > start && pos < end) {
                    end = pos;
                }
            }
            return end;
        };

        size_t columns_end = from_pos;
        size_t table_end = get_end_pos(from_pos, {where_pos, group_by_pos, having_pos, order_by_pos, limit_pos, offset_pos});
        size_t where_end = (where_pos != std::wstring::npos) ? get_end_pos(where_pos, {group_by_pos, having_pos, order_by_pos, limit_pos, offset_pos}) : 0;
        size_t group_by_end = (group_by_pos != std::wstring::npos) ? get_end_pos(group_by_pos, {having_pos, order_by_pos, limit_pos, offset_pos}) : 0;
        size_t having_end = (having_pos != std::wstring::npos) ? get_end_pos(having_pos, {order_by_pos, limit_pos, offset_pos}) : 0;
        size_t order_by_end = (order_by_pos != std::wstring::npos) ? get_end_pos(order_by_pos, {limit_pos, offset_pos}) : 0;
        size_t limit_end = (limit_pos != std::wstring::npos) ? get_end_pos(limit_pos, {offset_pos}) : 0;

        // 2. 各パートの切り出しとパース

        // Columns (Header)
        // SELECT (len 6) の後から FROM の前まで
        auto header_begin = sql.begin() + select_pos + 6;
        auto header_end = sql.begin() + columns_end;
        
        // boost::fusion::vector を使うためにヘッダが必要だが、
        // ここでは直接パース結果を受け取る構造体を使うか、
        // あるいは std::tuple を使う (X3 は std::tuple にも対応している)
        // しかし、ast::SelectStatement のフィールドに直接パースするのは難しい（順序が違うため）
        
        // 一時的な属性変数を用意
        boost::optional<ast::SelectQuantifier> quantifier_attr;
        std::vector<ast::ResultColumn> columns_attr;

        // X3 の attribute propagation ルールに従い、シーケンスの各要素をそれぞれの変数にバインドする
        // これには x3::rule を使うのが一番簡単だが、ローカル変数に直接入れるには参照を使う
        
        auto const parser = 
            -(select_quantifier) >> column_list;

        // std::tuple で受ける
        std::tuple<
            boost::optional<ast::SelectQuantifier>,
            std::vector<ast::ResultColumn>
        > header_attr;

        if (!x3::phrase_parse(header_begin, header_end, parser, x3::unicode::space, header_attr)) return false;

        // 代入
        if (std::get<0>(header_attr)) ast.quantifier = *std::get<0>(header_attr);
        ast.columns = std::get<1>(header_attr);

        // Table & Joins
        // " FROM " の長さは 6
        std::wstring from_section = sql.substr(from_pos + 6, table_end - (from_pos + 6));
        
        // JOIN を探す
        // 簡易的に "JOIN" を探して、その前の単語を確認する
        size_t current_pos = 0;
        
        // 最初のテーブルを探す (最初の JOIN まで、または最後まで)
        size_t first_join_pos = std::wstring::npos;
        ast::JoinType first_join_type = ast::JoinType::INNER; // ダミー
        size_t join_keyword_len = 0;

        // JOIN キーワードのリスト
        struct JoinKw { std::wstring kw; ast::JoinType type; };
        std::vector<JoinKw> join_kws = {
            {L" LEFT JOIN ", ast::JoinType::LEFT},
            {L" RIGHT JOIN ", ast::JoinType::RIGHT},
            {L" FULL JOIN ", ast::JoinType::FULL},
            {L" INNER JOIN ", ast::JoinType::INNER},
            {L" JOIN ", ast::JoinType::INNER}
        };

        // 最も手前にある JOIN を探す
        for (const auto& jk : join_kws) {
            size_t p = find_keyword(from_section, jk.kw, 0);
            if (p != std::wstring::npos) {
                if (first_join_pos == std::wstring::npos || p < first_join_pos) {
                    first_join_pos = p;
                    first_join_type = jk.type;
                    join_keyword_len = jk.kw.length();
                }
            }
        }

        std::wstring primary_table_str;
        if (first_join_pos == std::wstring::npos) {
            primary_table_str = from_section;
        } else {
            primary_table_str = from_section.substr(0, first_join_pos);
        }

        if (!parse_table_reference_impl(primary_table_str, ast.table)) return false;

        // JOIN があればループ処理
        current_pos = first_join_pos;
        while (current_pos != std::wstring::npos) {
            // current_pos は JOIN キーワードの開始位置
            // join_keyword_len は見つかったキーワードの長さ
            // first_join_type はそのタイプ

            // 次のパートへ進む
            size_t after_join = current_pos + join_keyword_len;
            
            // ON を探す
            size_t on_pos = find_keyword(from_section, L" ON ", after_join);
            if (on_pos == std::wstring::npos) return false; // ON がない

            // テーブル部分
            std::wstring joined_table_str = from_section.substr(after_join, on_pos - after_join);
            ast::Join join_node;
            join_node.type = first_join_type;
            if (!parse_table_reference_impl(joined_table_str, join_node.table)) return false;

            // 次の JOIN を探す
            size_t next_join_pos = std::wstring::npos;
            ast::JoinType next_join_type = ast::JoinType::INNER;
            size_t next_join_len = 0;

            for (const auto& jk : join_kws) {
                size_t p = find_keyword(from_section, jk.kw, on_pos + 4); // " ON " の後から
                if (p != std::wstring::npos) {
                    if (next_join_pos == std::wstring::npos || p < next_join_pos) {
                        next_join_pos = p;
                        next_join_type = jk.type;
                        next_join_len = jk.kw.length();
                    }
                }
            }

            // 条件部分
            size_t condition_start = on_pos + 4; // " ON " len
            size_t condition_end = (next_join_pos == std::wstring::npos) ? from_section.length() : next_join_pos;
            std::wstring condition_str = from_section.substr(condition_start, condition_end - condition_start);

            // 条件パース
            auto cond_begin = condition_str.begin();
            auto cond_end = condition_str.end();
            if (!x3::phrase_parse(cond_begin, cond_end, expression, x3::unicode::space, join_node.on)) return false;

            ast.joins.push_back(join_node);

            // ループ更新
            current_pos = next_join_pos;
            first_join_type = next_join_type;
            join_keyword_len = next_join_len;
        }

        // WHERE
        if (where_pos != std::wstring::npos) {
            // " WHERE " の長さは 7
            auto where_begin = sql.begin() + where_pos + 7;
            auto where_end_iter = sql.begin() + where_end;
            ast::Expression expr;
            if (!x3::phrase_parse(where_begin, where_end_iter, expression, x3::unicode::space, expr)) return false;
            ast.where = expr;
        }

        // GROUP BY
        if (group_by_pos != std::wstring::npos) {
            // " GROUP BY " の長さは 10
            auto gb_begin = sql.begin() + group_by_pos + 10;
            auto gb_end_iter = sql.begin() + group_by_end;
            if (!x3::phrase_parse(gb_begin, gb_end_iter, group_by_list, x3::unicode::space, ast.groupBy)) return false;
        }

        // HAVING
        if (having_pos != std::wstring::npos) {
            // " HAVING " の長さは 8
            auto hav_begin = sql.begin() + having_pos + 8;
            auto hav_end_iter = sql.begin() + having_end;
            ast::Expression expr;
            if (!x3::phrase_parse(hav_begin, hav_end_iter, expression, x3::unicode::space, expr)) return false;
            ast.having = expr;
        }

        // ORDER BY
        if (order_by_pos != std::wstring::npos) {
            // " ORDER BY " の長さは 10
            auto order_begin = sql.begin() + order_by_pos + 10;
            auto order_end_iter = sql.begin() + order_by_end;
            if (!x3::phrase_parse(order_begin, order_end_iter, order_by_list, x3::unicode::space, ast.orderBy)) return false;
        }

        // LIMIT
        if (limit_pos != std::wstring::npos) {
            // " LIMIT " の長さは 7
            auto limit_begin = sql.begin() + limit_pos + 7;
            auto limit_end_iter = sql.begin() + limit_end;
            ast::Expression expr;
            if (!x3::phrase_parse(limit_begin, limit_end_iter, expression, x3::unicode::space, expr)) return false;
            ast.limit = expr;
        }

        // OFFSET
        if (offset_pos != std::wstring::npos) {
            // " OFFSET " の長さは 8
            auto offset_begin = sql.begin() + offset_pos + 8;
            auto offset_end_iter = sql.end();
            ast::Expression expr;
            if (!x3::phrase_parse(offset_begin, offset_end_iter, expression, x3::unicode::space, expr)) return false;
            ast.offset = expr;
        }

        return true;
    }

    // 古い grammar は削除または非推奨
    // auto const grammar = select_stmt; 
}

// Forward declaration of BetweenArgs for Fusion adaptation
namespace sqlparser::parser {
    struct BetweenArgs;
    struct InArgs;
}

BOOST_FUSION_ADAPT_STRUCT(
    sqlparser::parser::BetweenArgs,
    not_between, lower, upper
)

BOOST_FUSION_ADAPT_STRUCT(
    sqlparser::parser::InArgs,
    not_in, values
)
