#ifndef _SQLITE_WRAPPER_H
#define _SQLITE_WRAPPER_H
#include <iostream>
#include <sqlite3.h>
#include <cstdio>
#include <vector>
using namespace std;

class SQLiteTable {
  public:
    SQLiteTable() {
    }

    virtual int size() const = 0;
    friend class SQLiteDatabase;

    void print() const;
    virtual int callback(int argc, char **argv, char **azColName);
    static int callback(void *sqlTable, int argc, char **argv, char **azColName);
};

template <typename T>
class List : public SQLiteTable {
public:
  List() {}

  int size() const { return _data.size(); }

  T& operator[] (size_t idx) { return _data[idx]; }
  const T& operator[] (size_t idx) const { return _data[idx]; }
  int callback(int argc, char **argv, char **azColName) {
    _data.push_back(T(argc, argv));
    return 0;
  }
  
private:
  vector<T> _data;
};

template <>
int List<string>::callback(int argc, char **argv, char **azColName);

class SQLiteDatabase {
public:
  SQLiteDatabase(string dbFilename);
  ~SQLiteDatabase();

  SQLiteDatabase(sqlite3* db): _db(db) {}

  bool exec(string statement, void* table = NULL) const;
  void dropTable(string tableName) const;
  void beginTransaction() const;
  void endTransaction() const;

private:
  SQLiteDatabase() {}
  sqlite3* _db;
};

#endif // _SQLITE_WRAPPER_H
