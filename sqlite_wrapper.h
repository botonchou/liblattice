#ifndef _SQLITE_WRAPPER_H
#define _SQLITE_WRAPPER_H
#include <iostream>
#include <sqlite3.h>
#include <cstdio>
using namespace std;

class SQLiteTable {
  public:
    SQLiteTable(): _table(NULL) {}
    ~SQLiteTable();

    int getCols() const { return _cols; }
    int getRows() const { return _rows; }
    char* get(int idx) { return _table[idx]; }
    void print() const;
    friend class SQLiteDatabase;
  private:
    int _cols;
    int _rows;
    char** _table;
};

class SQLiteDatabase {
public:
  SQLiteDatabase(string dbFilename);
  ~SQLiteDatabase();

  SQLiteDatabase(sqlite3* db): _db(db) {}

  bool exec(const char* query, int (*xCallback)(void*,int,char**,char**) = NULL ) const;
  SQLiteTable get(char* query) const;
  bool hasTable(string tableName) const;

private:
  SQLiteDatabase() {}
  sqlite3* _db;
};

#endif // _SQLITE_WRAPPER_H
