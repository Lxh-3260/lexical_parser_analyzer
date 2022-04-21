# lexical-parser-analyzer
词法分析器-语法分析器
3型文法部分参考的github上python星最多的那几个大佬写的，并对其进行了修改和注释
我写的文法在grammar3里，参考的文法如下：
start:$ identifier
start:$ integer
start:$ scientific
start:$ complex
start:$ limiter
start:$ operator
limiter:,
limiter:;
limiter:[
limiter:]
limiter:(
limiter:)
limiter:{
limiter:}
operator:+
operator:-
operator:*
operator:/
operator:%
operator:^
operator:&
operator:=
operator:>
operator:<
operator:> equal_tail
operator:< equal_tail
equal_tail:=
identifier:_
identifier:alphabet
identifier:_ identifier_tail
identifier:alphabet identifier_tail
identifier_tail:_
identifier_tail:digit
identifier_tail:alphabet
identifier_tail:_ identifier_tail
identifier_tail:digit identifier_tail
identifier_tail:alphabet identifier_tail
integer:digit
integer:digit integer_tail
integer_tail:digit
integer_tail:digit integer_tail
scientific:digit
scientific:digit scientific_tail
scientific_tail:digit
scientific_tail:digit scientific_tail
scientific_tail:. decimal
scientific_tail:e signed_index
decimal:digit
decimal:digit decimal_tail
decimal_tail:digit
decimal_tail:digit decimal_tail
decimal_tail:e signed_index
signed_index:+ index
signed_index:- index
signed_index:digit
signed_index:digit index_tail
index:digit
index:digit index_tail
index_tail:digit
index_tail:digit index_tail
complex:digit
complex:digit complex_first_tail
complex_first_tail:digit complex_first_tail
complex_first_tail:+ complex_second_tail
complex_second_tail:i
complex_second_tail:digit complex_second_tail
