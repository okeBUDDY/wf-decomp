// Object: root of the game's polymorphic classes.
//
// vtable 0x451270 holds only the scalar deleting destructor. Derived classes
// without a destructor of their own get one that /OPT:ICF folds into
// ~Object (e.g. 0x406510 calls 0x401090).
#ifndef WF_OBJECT_H_
#define WF_OBJECT_H_

#include <windows.h>

class Object {
 public:
  Object();             // 0x403810
  virtual ~Object() {}  // 0x401090, scalar deleting dtor 0x401070

  // Creates the log file (once) and writes `title` as its first line.
  void OpenLog(const char* path, const char* title);  // 0x403820
  // Appends "<name>:<message>" to the log file.
  void Log(const char* format, ...);  // 0x403890
  // Shows a warning box and logs "<name>:WARNING:<message>".
  void Warning(HWND window, const char* format, ...);  // 0x4038f0
  // Logs "<name>:ERROR:<message>".
  void Error(const char* format, ...);
  // The last error message, empty without one.
  static const char* last_error() { return error_message_; }
  // Forgets the last error (after a failure that was recovered from).
  static void ClearError() { error_message_[0] = '\0'; }

 protected:
  char* name_;  // 0x04, class name used in log lines

 private:
  static char log_path_[MAX_PATH];  // 0x468880, empty while logging is off
  static char message_[256];        // 0x468988
  static char error_message_[256];  // 0x468a88
};

#endif  // WF_OBJECT_H_
