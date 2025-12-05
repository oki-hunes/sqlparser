add_executable(simple_select simple_select.cpp)
target_link_libraries(simple_select PRIVATE sqlparser)

add_executable(ast_manipulation ast_manipulation.cpp)
target_link_libraries(ast_manipulation PRIVATE sqlparser)

add_executable(union_query union_query.cpp)
target_link_libraries(union_query PRIVATE sqlparser)

add_executable(union_manipulation union_manipulation.cpp)
target_link_libraries(union_manipulation PRIVATE sqlparser)

add_executable(check_union check_union.cpp)
target_link_libraries(check_union PRIVATE sqlparser)

add_executable(pg_cast pg_cast.cpp)
target_link_libraries(pg_cast PRIVATE sqlparser)

add_executable(test_custom_function test_custom_function.cpp)
target_link_libraries(test_custom_function PRIVATE sqlparser)

add_executable(repro_issue repro_issue.cpp)
target_link_libraries(repro_issue PRIVATE sqlparser)
