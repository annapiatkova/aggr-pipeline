#include <iostream>

#include "antlr4-runtime.h"
#include "AggrPipelineLexer.h"
#include "AggrPipelineParser.h"

using namespace antlrcpptest;
using namespace antlr4;

const char *example1 = 
"db.orders.aggregate( [           \
  {                               \
    $group: {                     \
      _id: null,                  \
      count: { $sum: 1 }          \
    }                             \
  }                               \
] )";

const char *example2 = 
"db.orders.aggregate( [           \
  {                               \
    $group: {                     \
      _id: null,                  \
      total: { $sum: \"$price\" } \
    }                             \
  }                               \
] )";

const char *example3 = 
"db.orders.aggregate( [           \
  {                               \
    $group: {                     \
      _id: \"$cust_id\",          \
      total: { $sum: \"$price\" } \
    }                             \
  }                               \
] )";

const char *example4 =
"db.orders.aggregate( [           \
  {                               \
    $group: {                     \
      _id: \"$cust_id\",          \
      total: { $sum: \"$price\" } \
    }                             \
  },                              \
  { $sort: { total: 1 } }         \
] )";

const char *example5 =
"db.orders.aggregate( [           \
  {                               \
    $group: {                     \
      _id: \"$cust_id\",          \
      total: { $sum: \"$price\" } \
    }                             \
  },                              \
  { $limit: 5 }                   \
] )";

const char *example6 = 
"db.orders.aggregate( [              \
  {                                  \
    $group: {                        \
      _id: \"$cust_id\",             \
      count: { $sum: 1 }             \
    }                                \
  },                                 \
  { $match: { count: { $gt: 1 } } }  \
] )";

const char *example7 =
"db.orders.aggregate( [              \
   { $match: { status: 'A' } },      \
   {                                 \
     $group: {                       \
        _id: \"$cust_id\",           \
        total: { $sum: \"$price\" }  \
     }                               \
   }                                 \
] )";

const char *example8 =
"db.orders.aggregate( [                \
   { $match: { status: 'A' } },        \
   {                                   \
     $group: {                         \
        _id: \"$cust_id\",             \
        total: { $sum: \"$price\" }    \
     }                                 \
   },                                  \
   { $match: { total: { $gt: 250, $lte: 350 } } } \
] )";

const char *examples[] = { example1, example2, example3, example4, example5, example6, example7, example8 };

class SQLQuery {
private:
  std::string select_list;
  std::string collection;
  std::string where_clause;
  std::string group_by_clause;
  std::string having_clause;
  std::string order_by_clause;
  std::string limit_clause;

  SQLQuery(std::string _select_list, std::string _collection, std::string _where_clause,
    std::string _group_by_clause, std::string _having_clause, std::string _order_by_clause,
    std::string _limit_clause): select_list(_select_list), collection(_collection),
    where_clause(_where_clause), group_by_clause(_group_by_clause), having_clause(_having_clause),
    order_by_clause(_order_by_clause), limit_clause(_limit_clause) {}
public:
  std::string toString() {
    return "SELECT " + select_list + " FROM " + collection + 
    (where_clause == "" ? "" : " WHERE " + where_clause) +
    (group_by_clause == "" ? "" : " GROUP BY " + group_by_clause) +
    (having_clause == "" ? "" : " HAVING " + having_clause) +
    (order_by_clause == "" ? "" : " ORDER BY " + order_by_clause) +
    (limit_clause == "" ? "" : " LIMIT " + limit_clause);
  }

  static SQLQuery Table(std::string tableName) {
    return SQLQuery("*", tableName, "", "", "", "", "");
  }

  static SQLQuery Group(std::string id, std::string aggrExprs, SQLQuery collection) {
    if (collection.select_list == "*" && collection.group_by_clause == "" && collection.having_clause == ""
      && collection.order_by_clause == "" && collection.limit_clause == "") {
      if (id == "NULL") {
        collection.select_list = aggrExprs;
        return collection;
      } else {
        collection.select_list = id + ", " + aggrExprs;
        collection.group_by_clause = id;
        return collection;
      }
    } else {
      if (id == "NULL") {
        return SQLQuery(aggrExprs, "(" + collection.toString() + ")", "", "", "", "", "");
      } else {
        return SQLQuery(id + ", " + aggrExprs, "(" + collection.toString() + ")", "", id, "", "", "");
      }
    }
  }

  static SQLQuery Limit(std::string count, SQLQuery collection) {
    if (collection.limit_clause == "" || std::stoull(collection.limit_clause) > std::stoull(count))
      collection.limit_clause = count;
    return collection;
  }

  static SQLQuery Match(std::string predicate, SQLQuery collection) {
    if (collection.group_by_clause == "" && collection.having_clause == "" && collection.order_by_clause == "" && collection.limit_clause == "") {
      if (collection.where_clause == "") {
        collection.where_clause = predicate;
      } else {
        collection.where_clause = "(" + collection.where_clause + ") AND (" + predicate + ")";
      }
      return collection;
    }

    if (collection.order_by_clause == "" && collection.limit_clause == "") {
      if (collection.having_clause == "") {
        collection.having_clause = predicate;
      } else {
        collection.having_clause = "(" + collection.having_clause + ") AND (" + predicate + ")";
      }
      return collection;
    }

    return SQLQuery("*", "(" + collection.toString() + ")", predicate, "", "", "", "");
  }

  static SQLQuery Sort(std::string sortExpr, SQLQuery collection) {
    if (collection.limit_clause == "") {
      // MongoDB $sort does not perform a stable sort, so if there was already an ORDER BY clause in our query
      // we just replace it with the new one (because the new sorting operation doesn't preserve the ordering
      // created by the previous one)
      // (Maybe add some sort of warning for that?)
      collection.order_by_clause = sortExpr;
      return collection;
    }
    return SQLQuery("*", "(" + collection.toString() + ")", "", "", "", sortExpr, "");
  }
};

class AggrToSQLParser {
private:
  static std::string getText(tree::ParseTree *tree) {
    return tree::Trees::getNodeText(tree, std::vector<std::string>());
  }

  static std::string parseExpr(AggrPipelineParser::ExprContext *tree) {
    if (tree->IDENTIFIER() != nullptr)
      return tree->IDENTIFIER()->getSymbol()->getText();
    if (tree->INT() != nullptr)
      return tree->INT()->getSymbol()->getText();
    if (tree->NULL_() != nullptr)
      return "NULL";
    throw std::runtime_error("parseExpr: unknown expression: " + getText(tree));
  }

  static std::string parseAggregateFun(AggrPipelineParser::AggregateFunContext *tree) {
    if (tree->SUM() != nullptr)
      return "SUM";
    if (tree->COUNT() != nullptr)
      return "COUNT";
    if (tree->AVG() != nullptr)
      return "AVG";
    if (tree->MAX() != nullptr)
      return "MAX";
    if (tree->MIN() != nullptr)
      return "MIN";
    throw std::runtime_error("parseAggregateFun: unknown aggregate function: " + getText(tree));
  }

  static std::string parseLogicalOp(AggrPipelineParser::LogicalOpContext *tree) {
    if (tree->AND() != nullptr)
      return "AND";
    if (tree->OR() != nullptr)
      return "OR";
    throw std::runtime_error("parseLogicalOp: unknown logical operator: " + getText(tree));
  }

  static std::string parseComparisonOp(AggrPipelineParser::ComparisonOpContext *tree) {
    if (tree->EQ() != nullptr)
      return "=";
    if (tree->NE() != nullptr)
      return "!=";
    if (tree->GT() != nullptr)
      return ">";
    if (tree->GTE() != nullptr)
      return ">=";
    if (tree->LT() != nullptr)
      return "<";
    if (tree->LTE() != nullptr)
      return "<=";
    throw std::runtime_error("parseComparisonOp: unknown comparison operator: " + getText(tree));
  }

  static std::string parseLiteral(AggrPipelineParser::LiteralContext *tree) {
    if (tree->IDENTIFIER() != nullptr)
      return "'" + tree->IDENTIFIER()->getSymbol()->getText() + "'";
    if (tree->INT() != nullptr)
      return tree->INT()->getSymbol()->getText();
    throw std::runtime_error("parseLiteral: unknown literal: " + getText(tree));
  }

  static std::string parsePredicate(AggrPipelineParser::PredicateContext *tree) {
    if (tree->logicalOp() != nullptr) {
      std::vector<AggrPipelineParser::PredicateContext *> predicates = tree->predicate();
      if (predicates.size() == 1)
        return parsePredicate(predicates[0]);
      std::string logicalOp = parseLogicalOp(tree->logicalOp());
      std::string predicate = "(" + parsePredicate(predicates[0]) + ")";
      for (int i = 1; i < predicates.size(); ++i) {
        predicate = predicate + " " + logicalOp + " (" + parsePredicate(predicates[i]) + ")";
      }
      return predicate;
    } else {
      std::string field = tree->IDENTIFIER()->getSymbol()->getText();
      std::vector<AggrPipelineParser::ComparisonOpContext *> comparisonOps = tree->comparisonOp();
      std::vector<AggrPipelineParser::LiteralContext *> literals = tree->literal();
      if (comparisonOps.size() == 0) {
        if (literals.size() != 1)
          throw std::runtime_error("parsePredicate");
        return field + " = " + parseLiteral(literals[0]);
      }
      if (comparisonOps.size() != literals.size())
        throw std::runtime_error("parsePredicate");

      std::string predicate = field + " " + parseComparisonOp(comparisonOps[0]) + " " + parseLiteral(literals[0]);
      if (comparisonOps.size() == 1)
        return predicate;
      predicate = "(" + predicate + ")";
      for (int i = 1; i < comparisonOps.size(); ++i) {
        predicate = predicate + " AND (" + field + " " + parseComparisonOp(comparisonOps[i]) + " " + parseLiteral(literals[i]) + ")";
      }
      return predicate;
    }
  }

  static std::string parseSortingOrder(std::string sortingOrder) {
    if (sortingOrder == "1")
      return "ASC";
    if (sortingOrder == "-1")
      return "DESC";
    throw std::runtime_error("parseSortingOrder: unknown sorting order: " + sortingOrder);
  }

public:
  static SQLQuery parseGroup(AggrPipelineParser::Group_Context *tree, SQLQuery collection) {
    std::vector<AggrPipelineParser::ExprContext *> exprs = tree->expr();
    std::vector<antlr4::tree::TerminalNode *> fields = tree->IDENTIFIER();
    std::vector<AggrPipelineParser::AggregateFunContext *> aggregateFuns = tree->aggregateFun();

    size_t nFields = fields.size();
    if (exprs.size() != nFields + 1 || aggregateFuns.size() != nFields)
      throw std::runtime_error("parseGroup");
    if (nFields < 1)
      throw std::runtime_error("parseGroup: no fields");

    std::string id = parseExpr(exprs[0]);
    std::string aggrExprs = parseAggregateFun(aggregateFuns[0]) + "(" + parseExpr(exprs[1]) + ") AS " + fields[0]->getSymbol()->getText();
    for (int i = 1; i < nFields; ++i)
      aggrExprs = aggrExprs + ", " + parseAggregateFun(aggregateFuns[i]) + "(" + parseExpr(exprs[i + 1]) + ") AS " + fields[i]->getSymbol()->getText();
    return SQLQuery::Group(id, aggrExprs, collection);
  }

  static SQLQuery parseLimit(AggrPipelineParser::Limit_Context *tree, SQLQuery collection) {
    return SQLQuery::Limit(tree->INT()->getSymbol()->getText(), collection);
  }

  static SQLQuery parseMatch(AggrPipelineParser::Match_Context *tree, SQLQuery collection) {
    return SQLQuery::Match(parsePredicate(tree->predicate()), collection);
  }

  static SQLQuery parseSort(AggrPipelineParser::Sort_Context *tree, SQLQuery collection) {
    std::vector<antlr4::tree::TerminalNode *> fields = tree->IDENTIFIER();
    std::vector<antlr4::tree::TerminalNode *> sortingOrders = tree->INT();
    if (fields.size() != sortingOrders.size() || fields.size() == 0)
      throw std::runtime_error("parseSort");
    std::string sortExpr = fields[0]->getSymbol()->getText() + " " + parseSortingOrder(sortingOrders[0]->getSymbol()->getText());
    for (int i = 1; i < fields.size(); ++i) {
      sortExpr = sortExpr + ", " + fields[i]->getSymbol()->getText() + " " + parseSortingOrder(sortingOrders[i]->getSymbol()->getText());
    }
    return SQLQuery::Sort(sortExpr, collection);
  }

  static SQLQuery parseStage(AggrPipelineParser::StageContext *tree, SQLQuery collection) {
    if (tree->group_() != nullptr)
      return parseGroup(tree->group_(), collection);
    if (tree->limit_() != nullptr)
      return parseLimit(tree->limit_(), collection);
    if (tree->match_() != nullptr)
      return parseMatch(tree->match_(), collection);
    if (tree->sort_() != nullptr)
      return parseSort(tree->sort_(), collection);
    throw std::runtime_error("parseStage: unknown stage: " + getText(tree));
  }

  static SQLQuery parsePipeline(AggrPipelineParser::PipelineContext *tree) {
    SQLQuery collection = SQLQuery::Table(tree->IDENTIFIER()->getSymbol()->getText());
    for (AggrPipelineParser::StageContext *child : tree->stage()) {
      collection = parseStage(child, collection);
    }
    return collection;
  }
};

int main(int , const char **) {
  for (auto example : examples) {
    ANTLRInputStream input(example);
    AggrPipelineLexer lexer(&input);
    CommonTokenStream tokens(&lexer);

    tokens.fill();

    AggrPipelineParser parser(&tokens);
    AggrPipelineParser::PipelineContext *pipeline = parser.pipeline();

    std::cout << AggrToSQLParser::parsePipeline(pipeline).toString() << std::endl;
  }
  return 0;
}
