lexer grammar AggrPipelineLexer;

DB: 'db' ;
AGGR: 'aggregate' ;

// supported aggregation stages
GROUP: '$group' ;
MATCH: '$match' ;
SORT: '$sort' ;
LIMIT: '$limit' ;

// supported aggregate functions
SUM: '$sum' ;
COUNT: '$count' ;
AVG: '$avg' ;
MAX: '$max' ;
MIN: '$min' ;

// operators for query predicates
AND: '$and' ;
OR: '$or' ;
EQ: '$eq' ;
NE: '$ne' ;
GT: '$gt' ;
GTE: '$gte' ;
LT: '$lt' ;
LTE: '$lte' ;

ID: '_id' ;
NULL_: 'null' ;

OpenPar: '(';
ClosePar: ')';
OpenSquare: '[';
CloseSquare: ']';
OpenCurly: '{';
CloseCurly: '}';
Dot: '.' ;
Comma: ',' ;
Colon: ':' ;
SingleQuote: '\'' ;
DoubleQuote: '"' ;
DollarSign: '$' ;
WS: [ \t\r\n]+ -> skip ;

INT: ('+'|'-')?[0-9]+ ;

IDENTIFIER: ([a-zA-Z]) ([a-zA-Z0-9]|'_')* ;
