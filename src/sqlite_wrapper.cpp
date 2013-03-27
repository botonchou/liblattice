#include <sqlite_wrapper.h>
#include <utility.h>

int SQLiteTable::callback(void* sqlTable, int argc, char **argv, char **azColName){
  SQLiteTable* sqlLiteTable = reinterpret_cast<SQLiteTable*>(sqlTable);
  return sqlLiteTable->callback(argc, argv, azColName);
}

int SQLiteTable::callback(int argc, char** argv, char** azColName) {
  for(int i=0; i<argc; i++)
    printf("%s\t", argv[i] ? argv[i] : "NULL");
  printf("\n");
  return 0;
}

xCallback SQLiteTable::getCallback() const {
  return SQLiteTable::callback;
}

template <>
int List<string>::callback(int argc, char **argv, char **azColName) {
  Array<string>::push_back(argv[0] ? argv[0] : "NULL");
  return 0;
}

// ****************************
// ****** SQLiteDatabase ******
// ****************************
// Static Functions goes here
int SQLiteDatabase::nTempTable = 0;
string SQLiteDatabase::TEMP_TABLE_PREFIX = "temp_";
string SQLiteDatabase::allocNewTempTable() {
  return TEMP_TABLE_PREFIX + int2str(nTempTable++);
}
// Non-Static Functions
SQLiteDatabase::SQLiteDatabase(string dbFilename): _db(NULL) {
  if(sqlite3_open_v2(dbFilename.c_str(), &_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL))
    cout << "Failed to open database file:" << dbFilename << endl;
  this->exec("PRAGMA main.cache_size=1048576;");
  this->exec("PRAGMA main.page_size=65536;");
  this->exec("PRAGMA synchronous=OFF;");
  this->exec("PRAGMA journal_mode=MEMORY;");
}

SQLiteDatabase::~SQLiteDatabase() {
  if(_db != NULL)
    sqlite3_close(_db);
}

bool SQLiteDatabase::exec(string statement) const {
  char* errMsg = NULL;
  sqlite3_exec(_db, statement.c_str(), 0, 0, &errMsg);

  if(errMsg != NULL) {
    printf("Error!!%s\nWhen executing instruction: %s", errMsg, statement.c_str());
    return false;
  }
  return true;
}

bool SQLiteDatabase::get(string statement, SQLiteTable* table) const {
  char* errMsg = NULL;
  xCallback callback = NULL;

  if(table != NULL)
    callback = table->getCallback();

  if(TempList* tempList = dynamic_cast<TempList*>(table)) {
    string tempTableName = SQLiteDatabase::allocNewTempTable();
    statement = "CREATE TEMP TABLE " + tempTableName + " AS " + statement;
    tempList->setTempTableName(tempTableName);
    tempList->bind(this);
  }

  sqlite3_exec(_db, statement.c_str(), callback, table, &errMsg);

  if(errMsg != NULL) {
    printf("Error!!%s\nWhen executing instruction: %s", errMsg, statement.c_str());
    return false;
  }
  return true;
}

void SQLiteDatabase::printTable(string tableName) const {
  SQLiteTable tempTable;
  this->get("SELECT * FROM " + tableName, &tempTable);
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

sqlite3* SQLiteDatabase::getDatabase() {
  return _db;
}
