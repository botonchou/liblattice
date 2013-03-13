#include <sqlite_wrapper.h>

int SQLiteTable::callback(void* sqlTable, int argc, char **argv, char **azColName){
  if(sqlTable == NULL)
    return 0;

  SQLiteTable* sqlLiteTable = reinterpret_cast<SQLiteTable*>(sqlTable);
  return sqlLiteTable->callback(argc, argv, azColName);
}

int SQLiteTable::callback(int argc, char** argv, char** azColName) {
  for(int i=0; i<argc; i++)
    printf("%s\t", azColName[i]);
  printf("\n");

  for(int i=0; i<argc; i++)
    printf("%s\t", argv[i] ? argv[i] : "NULL");
  printf("\n");
  return 0;
}

void SQLiteTable::print() const {
}

template <>
int List<string>::callback(int argc, char **argv, char **azColName) {
  _data.push_back(argv[0] ? argv[0] : "NULL");
  return 0;
}
// ****************************
// ****************************
// ****** SQLiteDatabase ******
// ****************************
SQLiteDatabase::SQLiteDatabase(string dbFilename): _db(NULL) {
  if(sqlite3_open_v2(dbFilename.c_str(), &_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL))
    cout << "Failed to open database file:" << dbFilename << endl;
}

SQLiteDatabase::~SQLiteDatabase() {
  if(_db != NULL)
    sqlite3_close(_db);
}

bool SQLiteDatabase::exec(string statement, void* table) const {
  char* errMsg = NULL;
  sqlite3_exec(_db, statement.c_str(), SQLiteTable::callback, table, &errMsg);

  if(errMsg != NULL) {
    printf("Error!!%s\nWhen executing instruction: %s", errMsg, statement.c_str());
    delete errMsg;
    return false;
  }
  return true;
}

void SQLiteDatabase::dropTable(string tableName) const {
  this->exec("DROP TABLE IF EXISTS " + tableName);
}

void SQLiteDatabase::beginTransaction() const {
  this->exec("BEGIN TRANSACTION;");
}

void SQLiteDatabase::endTransaction() const {
  this->exec("END TRANSACTION;");
}
