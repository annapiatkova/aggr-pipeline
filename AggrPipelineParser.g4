parser grammar AggrPipelineParser;

options {
	tokenVocab = AggrPipelineLexer;
}

pipeline: DB Dot IDENTIFIER Dot AGGR OpenPar OpenSquare (OpenCurly stage CloseCurly Comma)* OpenCurly stage CloseCurly (Comma)? CloseSquare ClosePar ;

stage: group_
     | limit_
     | match_
     | sort_ ;

group_: GROUP Colon OpenCurly ID Colon expr (Comma IDENTIFIER Colon OpenCurly aggregateFun Colon expr CloseCurly)+ CloseCurly ;

limit_: LIMIT Colon INT ;

match_: MATCH Colon OpenCurly predicate CloseCurly ;

sort_: SORT Colon OpenCurly (IDENTIFIER Colon INT Comma)* IDENTIFIER Colon INT (Comma)? CloseCurly ;

aggregateFun: SUM
            | COUNT
            | AVG
            | MAX
            | MIN ;

expr: DoubleQuote DollarSign IDENTIFIER DoubleQuote
    | INT
    | NULL_ ;

logicalOp : AND | OR ;
comparisonOp: EQ | NE | GT | GTE | LT | LTE ;

literal: DoubleQuote IDENTIFIER DoubleQuote
       | SingleQuote IDENTIFIER SingleQuote
       | INT ; 

predicate: IDENTIFIER Colon OpenCurly (comparisonOp Colon literal Comma)* comparisonOp Colon literal Comma? CloseCurly
         | IDENTIFIER Colon literal
         | logicalOp Colon OpenSquare (OpenCurly predicate CloseCurly)+ CloseSquare ;