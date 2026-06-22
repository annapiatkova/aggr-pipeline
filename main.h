class Table;
class Field;
class SelectList;
class SQLQuery;
class AggrToSQLParser;

class Table {
public:
  std::string name;
  std::string definition;
  std::vector<std::string> fields;

  Table(SQLQuery query);

  Table(SQLQuery query, std::string _name);

  Table(std::string _name, std::string _definition, std::vector<std::string> _fields);

  Table();

  std::string toString();

  std::string fieldsAsString();
};

class Field {
public:
  std::string definition;
  std::string name;

  Field(std::string d, std::string n);

  std::string toString() const;
};


class SelectList {
private:
  std::vector<Field> fields;
public:
  SelectList(std::vector<Field> _fields);

  //static SelectList Asterisk();

  std::string toString() const;

  std::vector<std::string> getFieldNames() const;

  bool isAsterisk() const;
  void setFront(Field field);
};

SelectList Asterisk();

class SQLQuery {
public:
  SelectList select_list;
  std::vector<Table> collections;
  std::string where_clause;
  std::string group_by_clause;
  std::string order_by_clause;
  std::string limit_clause;

  SQLQuery(SelectList _select_list, Table _collection, std::string _where_clause,
    std::string _group_by_clause, std::string _order_by_clause,
    std::string _limit_clause);

  SQLQuery(SelectList _select_list, std::vector<Table> _collections, std::string _where_clause,
    std::string _group_by_clause, std::string _order_by_clause,
    std::string _limit_clause);

  std::string toString();

  std::vector<std::string> getFieldNames();

  SQLQuery (Table table);

  SQLQuery (SelectList _select_list, Table table);

};

SQLQuery TableToJson(Table table, std::string alias);

SQLQuery Group(std::string id, SelectList aggrExprs, SQLQuery collection);

SQLQuery Limit(std::string count, SQLQuery collection);

SQLQuery Match(std::string predicate, SQLQuery collection);
