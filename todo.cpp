#include <sqlite3.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

using namespace std;

void initDatabase(sqlite3*& db) {
  char* errMsg = nullptr;

  // rc = return code of an sql query
  int rc = sqlite3_open("sqlite.db", &db);
  if (rc) {
    cerr << "Error while opening database: " << sqlite3_errmsg(db) << endl;
    return;
  }

  const char* createTable = R"(
    CREATE TABLE IF NOT EXISTS tasks (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        description TEXT NOT NULL,
        datetime TEXT NOT NULL
        );
    )";

  rc = sqlite3_exec(db, createTable, nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    cerr << "Error creating table: " << errMsg << endl;
    sqlite3_free(errMsg);
  }
}

// Function to print colored text
void printColor(const string& text, const string& color) {
  string colorCode;
  if (color == "red")
    colorCode = "\033[31m";
  else if (color == "green")
    colorCode = "\033[32m";
  else if (color == "yellow")
    colorCode = "\033[33m";
  else
    colorCode = "\033[0m";  // Default
  cout << colorCode << text << "\033[0m" << endl;
}

// Function to simulate progress animation
void showLoading(const string& message) {
  cout << message;
  for (int i = 0; i < 3; ++i) {
    cout << ".";
    cout.flush();
    this_thread::sleep_for(chrono::milliseconds(500));
  }
  cout << endl;
}

// Delete an entry by task number
void deleteTask(sqlite3* db, int id) {
  const char* deleteQuery = "DELETE FROM tasks where ID=(?)";
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, deleteQuery, -1, &stmt, nullptr);
  sqlite3_bind_int(stmt, 1, id);

  if (sqlite3_step(stmt) == SQLITE_DONE) {
    printColor("Task deleted successfully", "green");
  } else {
    printColor("Failed to delete the task", "red");
  }
}

// Read all tasks from the file
void viewTasks(sqlite3* db) {
  const char* viewQuery = "SELECT id, description, datetime FROM tasks;";
  sqlite3_stmt* stmt;
  sqlite3_prepare_v2(db, viewQuery, -1, &stmt, nullptr);

  cout << "Tasks:\n";

  if (sqlite3_step(stmt) != SQLITE_ROW) {
    printColor("To-do list is empty.", "yellow");
    return;
  } else {
      sqlite3_reset(stmt);
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    int id = sqlite3_column_int(stmt, 0);
    const unsigned char* description = sqlite3_column_text(stmt, 1);
    const unsigned char* datetime = sqlite3_column_text(stmt, 2);
    cout << id << ". " << description << " [" << datetime << "]\n";
  }
}

// Add a new task to the file
void addTask(sqlite3* db, string& description) {
  int rc;
  const char* addQuery =
      R"(INSERT INTO tasks (description, datetime) VALUES (?, datetime('now','localtime'));)";
  sqlite3_stmt* stmt;
  rc = sqlite3_prepare_v2(db, addQuery, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    cout << endl << sqlite3_errmsg(db) << endl;
  }
  sqlite3_bind_text(stmt, 1, description.c_str(), -1, SQLITE_STATIC);

  if (sqlite3_step(stmt) == SQLITE_DONE) {
    cout << "Task added successfully!" << endl;
  } else {
    cerr << "Failed to add the task." << endl;
  }
}

void clearScreen() {
  #ifdef __linux
      system("clear");
  #elif _WIN64
      system("cls");
  #endif
}

int main(int argc, char* argv[]) {
  sqlite3* db;
  initDatabase(db);

  // Check if a task is passed as a terminal argument
  if (argc > 1) {
    string todoElem;
    for (int i = 1; i < argc; i++) {
      todoElem += argv[i];
      if (i < argc - 1) todoElem += " ";  // Add space between words
    }
    addTask(db, todoElem);
    return 0;
  }

  int choice;
  string description;
  int id;

  clearScreen();

  do {
    cout << endl;
    printColor("Functions:", "green");
    printColor("1. View Tasks", "green");
    printColor("2. Add Task", "green");
    printColor("3. Delete Task", "green");
    printColor("4. Exit", "red");
    cout << "What would you like to do?: ";
    cin >> choice;

    switch (choice) {
      case 1:  // View tasks
        viewTasks(db);
        break;

      case 2:  // Add task
        cout << "Enter task description: ";
        cin.ignore();
        getline(cin, description);
        addTask(db, description);
        break;

      case 3:  // Delete task
        viewTasks(db);
        cout << "Enter ID to delete: ";
        cin.ignore();
        cin >> id;
        deleteTask(db, id);
        break;

      case 4:
        showLoading("Exiting");
        printColor("Goodbye!", "yellow");
        break;

      default:
        printColor("Invalid option. Try again.", "red");
        break;
    }
  } while (choice != 4);

  sqlite3_close(db);
  return 0;
}
