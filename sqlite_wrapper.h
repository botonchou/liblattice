#ifndef _SQLITE_WRAPPER_H
#define _SQLITE_WRAPPER_H
#include <iostream>
#include <sqlite3.h>
#include <cstdio>
#include <vector>
#include <array.h>
#include <utility.h>
using namespace std;
typedef int (*xCallback)(void*,int,char**,char**);

class SQLiteTable {
  public:
    SQLiteTable() {}

    virtual int callback(int argc, char **argv, char **azColName);
    static int callback(void *sqlTable, int argc, char **argv, char **azColName);
    virtual xCallback getCallback() const;

    friend class SQLiteDatabase;
};

class SQLiteDatabase {
public:
  SQLiteDatabase(string dbFilename);
  ~SQLiteDatabase();

  SQLiteDatabase(sqlite3* db): _db(db) {}

  bool exec(string statement) const;
  bool get(string statement, SQLiteTable* table) const;
  void printTable(string tableName) const;
  void dropTable(string tableName) const;
  void beginTransaction() const;
  void endTransaction() const;

  static string allocNewTempTable();

private:
  SQLiteDatabase() {}

  static int nTempTable;
  static string TEMP_TABLE_PREFIX;
  sqlite3* _db;
};

template <typename T>
class List : public SQLiteTable, public Array<T> {
public:
  int callback(int argc, char **argv, char **azColName) {
    Array<T>::push_back(T(argc, argv));
    return 0;
  }
  friend class SQLiteDatabase;
};

class TempList: public SQLiteTable {
public:

  void bind(const SQLiteDatabase* db) {
    _db = const_cast<SQLiteDatabase*>(db);
  }

  void setTempTableName(string tempTableName) {
    _tempTableName = tempTableName;
  }

  string getTempTableName() const {
    return _tempTableName;
  }

  int callback(int argc, char **argv, char **azColName) {
    for(int i=0; i<argc; ++i)
      printf("%s\t", argv[i] ? argv[i] : "NULL");
    printf("\n");
    return 0;
  }

  xCallback getCallback() const {
    return NULL;
  }

  void print() const {
    _db->printTable(_tempTableName);
  }

  friend class SQLiteDatabase;
private:
  SQLiteDatabase* _db;
  string _tempTableName;
};

template <>
int List<string>::callback(int argc, char **argv, char **azColName);
#endif // _SQLITE_WRAPPER_H
