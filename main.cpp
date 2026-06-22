#include <iostream>
#include <numeric>

#include "antlr4-runtime.h"
#include "AggrPipelineLexer.h"
#include "AggrPipelineParser.h"

#include "main.h"

using namespace antlrcpptest;
using namespace antlr4;

Table::Table(SQLQuery query): name(""), definition("(" + query.toString() + ")"), fields(query.getFieldNames()) {}

Table::Table(SQLQuery query, std::string _name): name(_name), definition("(" + query.toString() + ")"), fields(query.getFieldNames()) {}

Table::Table(std::string _name, std::string _definition, std::vector<std::string> _fields): name(_name), definition(_definition), fields(_fields) {}

Table::Table() {
  name = "unknown";
}

std::string Table::toString() {
  if (definition == "")
    return name;
  if (name == "")
    return "(" + definition + ")";
  return definition + " AS " + name;
}

std::string Table::fieldsAsString() {
  if (fields.size() == 0)
    return "";
  std::string res = fields[0];
  for(int i = 1; i < fields.size(); ++i) {
    res = res + ", " + fields[i];
  }
  return res;
}

Field::Field(std::string d, std::string n): definition(d), name(n) {}

std::string Field::toString() const {
  if (definition == "")
    return name;
  if (name == "")
    return "(" + definition + ")";
  return definition + " AS " + name;
}

SelectList::SelectList(std::vector<Field> _fields): fields(_fields) {}

SelectList Asterisk() {
  return SelectList(std::vector<Field>(1, Field("", "*")));
}

std::string SelectList::toString() const {
  if (fields.size() == 0)
    throw std::runtime_error("Select list is empty");
  std::string result = fields[0].toString();
  for (int i = 1; i < fields.size(); ++i)
    result = result + ", " + fields[i].toString();
  return result;
}

std::vector<std::string> SelectList::getFieldNames() const {
  auto res = std::vector<std::string>(fields.size());
  for (int i = 0; i < fields.size(); ++i) {
    res[i] = fields[i].name;
  }
  return res;
}

bool SelectList::isAsterisk() const {
  if (fields.size() != 1)
    return false;
  return fields[0].name == "*" && fields[0].definition == "";
}

void SelectList::setFront(Field field) {
  fields.insert(fields.begin(), field);
}

SQLQuery::SQLQuery(SelectList _select_list, Table _collection, std::string _where_clause,
  std::string _group_by_clause, std::string _order_by_clause,
  std::string _limit_clause): select_list(_select_list), collections(std::vector<Table>({_collection})),
  where_clause(_where_clause), group_by_clause(_group_by_clause),
  order_by_clause(_order_by_clause), limit_clause(_limit_clause) {}

SQLQuery::SQLQuery(SelectList _select_list, std::vector<Table> _collections, std::string _where_clause,
  std::string _group_by_clause, std::string _order_by_clause,
  std::string _limit_clause): select_list(_select_list), collections(_collections),
  where_clause(_where_clause), group_by_clause(_group_by_clause),
  order_by_clause(_order_by_clause), limit_clause(_limit_clause) {}
  
std::string SQLQuery::toString() {
  if (collections.size() > 1) {
    std::string tables = "";
    for (Table collection : collections) {
      tables = tables + ", " + collection.toString();
    }
    return "SELECT " + select_list.toString() + " FROM " + tables.substr(2) +
    (where_clause == "" ? "" : " WHERE " + where_clause) +
    (group_by_clause == "" ? "" : " GROUP BY " + group_by_clause) +
    (order_by_clause == "" ? "" : " ORDER BY " + order_by_clause) +
    (limit_clause == "" ? "" : " LIMIT " + limit_clause);
  }
  Table collection = collections[0];
  if (collection.definition == "")
    return "SELECT " + select_list.toString() + " FROM " + collection.name + 
    (where_clause == "" ? "" : " WHERE " + where_clause) +
    (group_by_clause == "" ? "" : " GROUP BY " + group_by_clause) +
    (order_by_clause == "" ? "" : " ORDER BY " + order_by_clause) +
    (limit_clause == "" ? "" : " LIMIT " + limit_clause);
  if (where_clause == "" && group_by_clause == "" && order_by_clause == "" && limit_clause == "")
    return "SELECT " + select_list.toString() + " FROM " + collection.toString();
  return "SELECT * FROM (SELECT " + select_list.toString() + " FROM " + collection.toString() + ")" + 
    (where_clause == "" ? "" : " WHERE " + where_clause) +
    (group_by_clause == "" ? "" : " GROUP BY " + group_by_clause) +
    (order_by_clause == "" ? "" : " ORDER BY " + order_by_clause) +
    (limit_clause == "" ? "" : " LIMIT " + limit_clause);
}

std::vector<std::string> SQLQuery::getFieldNames() {
  if (select_list.isAsterisk()) {
    if (collections.size() != 1) {
      throw std::runtime_error("getFieldNames");
    }
    return collections[0].fields;
  }
  return select_list.getFieldNames();    
}

SQLQuery::SQLQuery (Table table): select_list(Asterisk()), collections(std::vector<Table>({table})), where_clause(""),
  group_by_clause(""), order_by_clause(""), limit_clause("") {}

SQLQuery::SQLQuery (SelectList _select_list, Table table): select_list(_select_list), collections(std::vector<Table>({table})) {}

SQLQuery TableToJson(Table table, std::string alias) {
  return SQLQuery(SelectList(std::vector<Field>(1, Field("JSON_ARRAYAGG(CAST(ROW(" + table.fieldsAsString() + ") AS " + table.name + "))", alias))),
    table, "", "", "", "");
}

SQLQuery Group(std::string id, SelectList aggrExprs, SQLQuery collection) {
  if (collection.select_list.isAsterisk() && collection.group_by_clause == ""
    && collection.order_by_clause == "" && collection.limit_clause == "") {
    if (id == "NULL") {
      collection.select_list = aggrExprs;
      return collection;
    } else {
      aggrExprs.setFront(Field("", id));
      collection.select_list = aggrExprs;
      collection.group_by_clause = id;
      return collection;
    }
  } else {
    if (id == "NULL") {
      return SQLQuery(aggrExprs, Table(collection), "", "", "", "");
    } else {
      aggrExprs.setFront(Field("", id));
      return SQLQuery(aggrExprs, Table(collection), "", id, "", "");
    }
  }
}

SQLQuery Limit(std::string count, SQLQuery collection) {
  if (collection.limit_clause == "" || std::stoull(collection.limit_clause) > std::stoull(count))
    collection.limit_clause = count;
  return collection;
}

SQLQuery Match(std::string predicate, SQLQuery collection) {
  if (collection.group_by_clause == "" && collection.order_by_clause == "" && collection.limit_clause == "") {
    if (collection.where_clause == "") {
      collection.where_clause = predicate;
    } else {
      collection.where_clause = "(" + collection.where_clause + ") AND (" + predicate + ")";
    }
    return collection;
  }
  return SQLQuery(Asterisk(), Table(collection), predicate, "", "", "");
}

SQLQuery Sort(std::string sortExpr, SQLQuery collection) {
  if (collection.limit_clause == "") {
    // MongoDB $sort does not perform a stable sort, so if there was already an ORDER BY clause in our query
    // we just replace it with the new one (because the new sorting operation doesn't preserve the ordering
    // created by the previous one)
    // (Maybe add some warning for that?)
    collection.order_by_clause = sortExpr;
    return collection;
  }
  return SQLQuery(Asterisk(), Table(collection), "", "", sortExpr, "");
}


class AggrToSQLParser {
private:
  std::map<std::string, Table> tables;
  int i = 0;
public:
  std::string generateTableName() {
    return "table" + std::to_string(i++);
  }
private:
  Table getTable(std::string name) {
    return tables[name];
  }

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

  static std::string parseIdentifierInQuotes(AggrPipelineParser::IdentifierInQuotesContext *tree) {
    if (tree->IDENTIFIER() != nullptr)
      return tree->IDENTIFIER()->getSymbol()->getText();
    if (tree->ID() != nullptr)
      return tree->ID()->getSymbol()->getText();
    throw std::runtime_error("parseIdentifierInQuotes");
  }

public:
  AggrToSQLParser(std::map<std::string, Table> _tables): tables(_tables) {
  }

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
    std::vector<Field> aggrExprs;
    for(int i = 0; i < nFields; ++i) {
      aggrExprs.push_back(Field(parseAggregateFun(aggregateFuns[i]) + "(" + parseExpr(exprs[i + 1]) + ")", fields[i]->getSymbol()->getText()));
    }
    return Group(id, SelectList(aggrExprs), collection);
  }

  static SQLQuery parseLimit(AggrPipelineParser::Limit_Context *tree, SQLQuery collection) {
    return Limit(tree->INT()->getSymbol()->getText(), collection);
  }

  static SQLQuery parseMatch(AggrPipelineParser::Match_Context *tree, SQLQuery collection) {
    return Match(parsePredicate(tree->predicate()), collection);
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
    return Sort(sortExpr, collection);
  }

  SQLQuery parseLookup(AggrPipelineParser::Lookup_Context *tree, SQLQuery collection) {
    std::string collectionToJoin = parseIdentifierInQuotes(tree->identifierInQuotes(0));
    std::string localField = parseIdentifierInQuotes(tree->identifierInQuotes(1));
    std::string foreignField = parseIdentifierInQuotes(tree->identifierInQuotes(2));
    std::string alias = parseIdentifierInQuotes(tree->identifierInQuotes(3));

    std::string tableName = generateTableName();

    std::vector<std::string> fieldNames = collection.getFieldNames();
    std::vector<Field> fields;
    for (std::string fieldName : fieldNames) {
      fields.push_back(Field("", fieldName));
    }
    fields.push_back(Field(
        Match(foreignField + " = " + tableName + "." + localField, TableToJson(getTable(collectionToJoin), alias)).toString()
      , ""));

    return SQLQuery(SelectList(fields), Table(tableName, "(" + collection.toString() + ")", collection.getFieldNames()));
  }

  SQLQuery parseProject(AggrPipelineParser::Project_Context *tree, SQLQuery collection);

  static std::vector<std::string> parseDotNotation(AggrPipelineParser::DotNotationContext *tree) {
    std::vector<std::string> res;
    for (auto ident : tree->IDENTIFIER()) {
      res.push_back(ident->getSymbol()->getText());
    }
    if (tree->ID() != nullptr)
      res.push_back(tree->ID()->getSymbol()->getText());
    return res;
  }

  SQLQuery parseStage(AggrPipelineParser::StageContext *tree, SQLQuery collection) {
    if (tree->group_() != nullptr)
      return parseGroup(tree->group_(), collection);
    if (tree->limit_() != nullptr)
      return parseLimit(tree->limit_(), collection);
    if (tree->lookup_() != nullptr)
      return parseLookup(tree->lookup_(), collection);
    if (tree->match_() != nullptr)
      return parseMatch(tree->match_(), collection);
    if (tree->sort_() != nullptr)
      return parseSort(tree->sort_(), collection);
    if (tree->project_() != nullptr)
      return parseProject(tree->project_(), collection);
    throw std::runtime_error("parseStage: unknown stage: " + getText(tree));
  }

  SQLQuery parsePipeline(AggrPipelineParser::PipelineContext *tree) {
    std::string tableName = tree->IDENTIFIER()->getSymbol()->getText();
    SQLQuery collection = SQLQuery(getTable(tableName));
    for (AggrPipelineParser::StageContext *child : tree->stage()) {
      collection = parseStage(child, collection);
    }
    return collection;
  }
};

class Node {
public:
  std::string name;
  std::string newTableName;
  std::string oldTableName;
  std::vector<std::string> fields;
  std::vector<Node> children;

  std::pair<Field, std::vector<Table>> generateForRoot(std::string parentOldTable) {
    std::string selectList = ", ";
    for (std::string field : fields) {
      selectList = selectList + ", '" + field + "', " + oldTableName + "->'" + field + "'";
    }
    for (Node child : children) {
      selectList = selectList + ", '" + child.name + "', " + child.newTableName + "." + child.name;
    }
    std::vector<Table> tables;
    tables.push_back(Table(oldTableName, "LATERAL JSON_ARRAY_ELEMENTS(" + parentOldTable + "." + name + ")", std::vector<std::string>()));

    for (Node child : children) {
      tables.push_back(Table(child.newTableName, "LATERAL (" + child.generateQuery(oldTableName) + ")", std::vector<std::string>()));
    }
    return std::pair<Field, std::vector<Table>>(Field("JSON_AGG(JSON_BUILD_OBJECT(" + selectList.substr(4) + ")) ", name), tables);
  } 

  std::string generateQuery(std::string parentOldTable = "") {
    std::string selectList = ", ";
    for (std::string field : fields) {
      selectList = selectList + ", '" + field + "', " + oldTableName + "->'" + field + "'";
    }
    for (Node child : children) {
      selectList = selectList + ", '" + child.name + "', " + child.newTableName + "." + child.name;
    }
    std::string tables = "";
    tables = tables + "JSON_ARRAY_ELEMENTS(" + parentOldTable + "->'" + name + "') " + oldTableName;
    for (Node child : children) {
      tables = tables + ", LATERAL (" + child.generateQuery(oldTableName) + ") " + child.newTableName;
    }
    return "SELECT JSON_AGG(JSON_BUILD_OBJECT(" + selectList.substr(4) + ")) " + name + "\nFROM " + tables;
  }

  Node(std::string _name, std::string _newTableName, std::string _oldTableName): name(_name), newTableName(_newTableName), oldTableName(_oldTableName), fields(std::vector<std::string>()), children(std::vector<Node>()) {}
};

void addNode(Node &node, std::vector<std::string> &field, int index, AggrToSQLParser &parser) {
  if (field.size() == index + 1) {
    for (std::string f : node.fields) {
      if (f == field[index]) {
        return;
      }
    }
    node.fields.push_back(field[index]);
    return;
  }
  for (Node child : node.children) {
    if (child.name == field[index]) {
      addNode(child, field, index + 1, parser);
      return;
    }
  }
  node.children.push_back(Node(field[index], parser.generateTableName(), parser.generateTableName()));
  addNode(node.children[node.children.size() - 1], field, index + 1, parser);
  return;
}

SQLQuery Select(std::vector<std::vector<std::string>> fields, SQLQuery collection, AggrToSQLParser &parser) {
  std::vector<Node> roots;
  std::vector<std::string> _fields;
  for (auto field : fields) {
    if (field.size() == 0) {
      throw std::runtime_error("Select: empty field");
    }
    auto it = std::find_if(roots.begin(), roots.end(), [&field](Node node){ return node.name == field[0]; });
    if (it == roots.end()) {
      if (field.size() == 1) {
        _fields.push_back(field[0]);
      } else {
        Node root = Node(field[0], "", parser.generateTableName());
        addNode(root, field, 1, parser);
        roots.push_back(root);
      }
    } else {
      if (field.size() == 1) {
        _fields.push_back(field[0]);
      } else {
        addNode(*it, field, 1, parser);
      }
    }
  }
  std::string tableName = parser.generateTableName();
  std::vector<Field> selectList;
  std::vector<Table> tables = std::vector<Table>({Table(tableName, "(" + collection.toString() + ")", collection.getFieldNames())});
  for (std::string f : _fields) {
    selectList.push_back(Field("", f));
  }
  for (Node root : roots) {
    auto res = root.generateForRoot(tableName);
    selectList.push_back(res.first);
    tables.insert(tables.end(), res.second.begin(), res.second.end());
  }
  std::string groupByClause = "";
  if (_fields.size() > 0) {
    groupByClause = _fields[0];
    for (int i = 1; i < _fields.size(); ++i) {
      groupByClause = groupByClause + ", " + _fields[i];
    }
  }
  return SQLQuery(SelectList(selectList), tables, "", groupByClause, "", "");
}

 SQLQuery AggrToSQLParser::parseProject(AggrPipelineParser::Project_Context *tree, SQLQuery collection) {
    bool includedFields = false, excludedFields = false;
    for (auto child : tree->projectSpec()) {
      if (child->ID() == nullptr) {
        if (child->INT()->getSymbol()->getText() == "0") {
          excludedFields = true;
        } else {
          includedFields = true;
        }
      }
    }
    if (excludedFields && includedFields)
      throw std::runtime_error("$project");
    if (includedFields) {
      std::vector<std::vector<std::string>> fields;
      bool includeId = true;
      for (auto child : tree->projectSpec()) {
        if (child->ID() != nullptr && child->INT()->getSymbol()->getText() == "0")
          includeId = false;
        if (child->INT()->getSymbol()->getText() != "0"){
          if (child->IDENTIFIER() != nullptr)
            fields.push_back(std::vector<std::string>({child->IDENTIFIER()->getSymbol()->getText()}));
          if (child->dotNotation() != nullptr)
            fields.push_back(parseDotNotation(child->dotNotation()));
        }
      }
      return Select(fields, collection, *this);
    }
    return collection;
  }

int main(int argc, const char **argv) {
  std::ifstream tableDefsStream(argv[1]);
  std::ifstream pipelineStream(argv[2]);
  std::string defs;
  std::map<std::string, Table> tables;
  while (true) {
    std::getline(tableDefsStream, defs, ';');
    if (std::find(defs.begin(), defs.end(), '(') != defs.end()) {
      std::istringstream input;
      input.str(defs);
      std::string tableName;
      std::getline(input, tableName, '(');
      std::vector<std::string> fields;
      while (true) {
        std::string field;
        std::getline(input, field, ',');
        if (field[field.size() - 1] == ')') {
          fields.push_back(field.substr(0, field.size() - 1));
          break;
        } else {
          fields.push_back(field);
        }
      }
      tables[tableName] = Table(tableName, "", fields);
    } else {
      break;
    }
  }

  std::string pipelineString;
  std::getline(pipelineStream, pipelineString, ';');

  std::cout << "Aggregation pipeline:" << std::endl << pipelineString << std::endl << std::endl;

  ANTLRInputStream input(pipelineString);
  AggrPipelineLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  tokens.fill();
  AggrPipelineParser parser(&tokens);
  AggrPipelineParser::PipelineContext *pipeline = parser.pipeline();
  AggrToSQLParser myParser(tables);
  std::cout << "SQL:" << std::endl << myParser.parsePipeline(pipeline).toString() << std::endl;
  return 0;
}
