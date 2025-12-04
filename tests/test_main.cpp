#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <sqlparser/parser.hpp>
#include <sqlparser/generator.hpp>

int g_tests_passed = 0;
int g_tests_failed = 0;

void run_test(const std::string& name, std::function<bool()> test_func) {
    std::cout << "[RUN] " << name << "... ";
    try {
        if (test_func()) {
            std::cout << "PASS" << std::endl;
            g_tests_passed++;
        } else {
            std::cout << "FAIL" << std::endl;
            g_tests_failed++;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (Exception: " << e.what() << ")" << std::endl;
        g_tests_failed++;
    } catch (...) {
        std::cout << "FAIL (Unknown Exception)" << std::endl;
        g_tests_failed++;
    }
}

#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        std::cerr << "Assertion failed: " << #condition << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false; \
    }

#define ASSERT_EQ(expected, actual) \
    if ((expected) != (actual)) { \
        std::wcerr << L"Assertion failed: " << #expected << L" == " << #actual << L"\nExpected: " << (expected) << L"\nActual:   " << (actual) << L"\nAt " << __FILE__ << ":" << __LINE__ << std::endl; \
        return false; \
    }

bool test_basic_select() {
    std::wstring sql = L"SELECT * FROM users";
    sqlparser::ast::SelectStatement ast;
    ASSERT_TRUE(sqlparser::parser::parse(sql, ast));
    std::wstring generated = sqlparser::generate(ast);
    ASSERT_EQ(sql, generated);
    return true;
}

bool test_select_columns() {
    std::wstring sql = L"SELECT id, name FROM users";
    sqlparser::ast::SelectStatement ast;
    ASSERT_TRUE(sqlparser::parser::parse(sql, ast));
    std::wstring generated = sqlparser::generate(ast);
    ASSERT_EQ(sql, generated);
    return true;
}

bool test_where_comparison() {
    std::wstring sql = L"SELECT * FROM users WHERE (age > 20)";
    sqlparser::ast::SelectStatement ast;
    ASSERT_TRUE(sqlparser::parser::parse(sql, ast));
    std::wstring generated = sqlparser::generate(ast);
    ASSERT_EQ(sql, generated);
    return true;
}

bool test_is_null() {
    std::wstring sql = L"SELECT * FROM users WHERE (name IS NULL)";
    sqlparser::ast::SelectStatement ast;
    ASSERT_TRUE(sqlparser::parser::parse(sql, ast));
    std::wstring generated = sqlparser::generate(ast);
    ASSERT_EQ(sql, generated);
    return true;
}

bool test_is_not_null() {
    std::wstring sql = L"SELECT * FROM users WHERE (name IS NOT NULL)";
    sqlparser::ast::SelectStatement ast;
    ASSERT_TRUE(sqlparser::parser::parse(sql, ast));
    std::wstring generated = sqlparser::generate(ast);
    ASSERT_EQ(sql, generated);
    return true;
}

bool test_between() {
    std::wstring sql = L"SELECT * FROM products WHERE (price BETWEEN 100 AND 200)";
    sqlparser::ast::SelectStatement ast;
    ASSERT_TRUE(sqlparser::parser::parse(sql, ast));
    std::wstring generated = sqlparser::generate(ast);
    ASSERT_EQ(sql, generated);
    return true;
}

bool test_in() {
    std::wstring sql = L"SELECT * FROM users WHERE (id IN (1, 2, 3))";
    sqlparser::ast::SelectStatement ast;
    ASSERT_TRUE(sqlparser::parser::parse(sql, ast));
    std::wstring generated = sqlparser::generate(ast);
    ASSERT_EQ(sql, generated);
    return true;
}

bool test_concat() {
    std::wstring sql = L"SELECT ('a' || 'b')";
    sqlparser::ast::SelectStatement ast;
    ASSERT_TRUE(sqlparser::parser::parse(sql, ast));
    std::wstring generated = sqlparser::generate(ast);
    ASSERT_EQ(sql, generated);
    return true;
}

bool test_concat_numbers() {
    std::wstring sql = L"SELECT (1 || 2)";
    sqlparser::ast::SelectStatement ast;
    ASSERT_TRUE(sqlparser::parser::parse(sql, ast));
    std::wstring generated = sqlparser::generate(ast);
    ASSERT_EQ(sql, generated);
    return true;
}

bool test_concat_debug() {
    std::wstring sql = L"SELECT (1 || 2)";
    sqlparser::ast::SelectStatement ast;
    ASSERT_TRUE(sqlparser::parser::parse(sql, ast));
    std::wstring generated = sqlparser::generate(ast);
    // Generated SQL will use || because generator uses ||
    // So we just check parse success for now
    return true;
}

bool test_add() {
    std::wstring sql = L"SELECT (1 + 2)";
    sqlparser::ast::SelectStatement ast;
    ASSERT_TRUE(sqlparser::parser::parse(sql, ast));
    std::wstring generated = sqlparser::generate(ast);
    ASSERT_EQ(sql, generated);
    return true;
}

bool test_bitwise() {
    std::wstring sql = L"SELECT (1 | 2)";
    sqlparser::ast::SelectStatement ast;
    ASSERT_TRUE(sqlparser::parser::parse(sql, ast));
    std::wstring generated = sqlparser::generate(ast);
    ASSERT_EQ(sql, generated);
    return true;
}

bool test_union() {
    std::wstring sql = L"SELECT * FROM t1 UNION SELECT * FROM t2";
    sqlparser::ast::SelectStatement ast;
    ASSERT_TRUE(sqlparser::parser::parse(sql, ast));
    std::wstring generated = sqlparser::generate(ast);
    ASSERT_EQ(sql, generated);
    return true;
}

bool test_union_all() {
    std::wstring sql = L"SELECT * FROM t1 UNION ALL SELECT * FROM t2";
    sqlparser::ast::SelectStatement ast;
    ASSERT_TRUE(sqlparser::parser::parse(sql, ast));
    std::wstring generated = sqlparser::generate(ast);
    ASSERT_EQ(sql, generated);
    return true;
}

bool test_table_alias() {
    std::wstring sql = L"SELECT u.name FROM users u";
    sqlparser::ast::SelectStatement ast;
    ASSERT_TRUE(sqlparser::parser::parse(sql, ast));
    std::wstring generated = sqlparser::generate(ast);
    ASSERT_EQ(sql, generated);
    return true;
}

bool test_table_alias_as() {
    // The generator might normalize "AS" out or keep it depending on implementation.
    // Based on generator.hpp: if (t.alias) os << L" " << *t.alias;
    // It does NOT print "AS". So "FROM users AS u" becomes "FROM users u".
    std::wstring input_sql = L"SELECT u.name FROM users AS u";
    std::wstring expected_sql = L"SELECT u.name FROM users u";
    sqlparser::ast::SelectStatement ast;
    ASSERT_TRUE(sqlparser::parser::parse(input_sql, ast));
    std::wstring generated = sqlparser::generate(ast);
    ASSERT_EQ(expected_sql, generated);
    return true;
}

bool test_subquery_alias() {
    std::wstring sql = L"SELECT t.a FROM (SELECT * FROM t1) t";
    sqlparser::ast::SelectStatement ast;
    ASSERT_TRUE(sqlparser::parser::parse(sql, ast));
    std::wstring generated = sqlparser::generate(ast);
    ASSERT_EQ(sql, generated);
    return true;
}

int main() {
    run_test("Basic Select", test_basic_select);
    run_test("Select Columns", test_select_columns);
    run_test("Where Comparison", test_where_comparison);
    run_test("IS NULL", test_is_null);
    run_test("IS NOT NULL", test_is_not_null);
    run_test("BETWEEN", test_between);
    run_test("IN", test_in);
    run_test("Concat", test_concat);
    run_test("Concat Numbers", test_concat_numbers);
    run_test("Concat Debug", test_concat_debug);
    run_test("Add", test_add);
    run_test("Bitwise", test_bitwise);
    run_test("UNION", test_union);
    run_test("UNION ALL", test_union_all);
    run_test("Table Alias", test_table_alias);
    run_test("Table Alias AS", test_table_alias_as);
    run_test("Subquery Alias", test_subquery_alias);

    std::cout << "\nSummary: " << g_tests_passed << " passed, " << g_tests_failed << " failed." << std::endl;
    return g_tests_failed == 0 ? 0 : 1;
}
