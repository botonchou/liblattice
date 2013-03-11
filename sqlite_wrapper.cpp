#include <sqlite_wrapper.h>
SQLiteTable::~SQLiteTable() {
  sqlite3_free_table(_table);
}

void SQLiteTable::print() const {
  if(_table == NULL)
    return;

  for (int i=0; i < _rows + 1; i++) {
    for (int j=0; j < _cols; j++)
      printf("%s\t", _table[i*_cols+j]);
    printf("\n");
  }
}

SQLiteDatabase::SQLiteDatabase(string dbFilename): _db(NULL) {
  if(sqlite3_open_v2(dbFilename.c_str(), &_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL))
    cout << "Failed to open database file:" << dbFilename << endl;
}

SQLiteDatabase::~SQLiteDatabase() {
  if(_db != NULL)
    sqlite3_close(_db);
}


bool SQLiteDatabase::exec(const char* query, int (*xCallback)(void*,int,char**,char**) ) const {
  char* errMsg = NULL;
  sqlite3_exec(_db, query, xCallback, 0, &errMsg);
  if(errMsg != NULL) {
    cout << "ERROR!! " << errMsg << endl;
    cout << "Instruction: " << query << endl;
    delete errMsg;
    return false;
  }
  return true;
}

SQLiteTable SQLiteDatabase::get(char* query) const {
  char* errMsg = NULL;
  SQLiteTable result;
  sqlite3_get_table(_db, query, &(result._table), &(result._rows), &(result._cols), &errMsg);
  if(errMsg != NULL) {
    cout << "ERROR!! " << errMsg << endl;
    cout << "Instruction: " << query << endl;
    delete errMsg;
  }

  return result;
}

bool SQLiteDatabase::hasTable(string tableName) const {
  char query[128];
  sprintf(query, "SELECT count(type) FROM sqlite_master WHERE type='table' AND name='%s';", tableName.c_str());
  SQLiteTable result = this->get(query);

  return (result._table[1][0] == '1');
}
