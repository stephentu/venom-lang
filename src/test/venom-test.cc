#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <errno.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <ast/statement/node.h>
#include <parser/driver.h>
#include <util/filesystem.h>
#include <util/stl.h>

using namespace std;
using namespace venom;

/**
 * This Timer class taken from cryptdb
 */
class Timer {
private:
  Timer(const Timer &t);  /* no reason to copy timer objects */
public:
  Timer() { lap(); }

  /** microseconds */
  inline uint64_t lap() {
    uint64_t t0 = start;
    uint64_t t1 = cur_usec();
    start = t1;
    return t1 - t0;
  }

  /** milliseconds */
  inline double lap_ms() { return ((double)lap()) / 1000.0; }

private:
  static inline uint64_t cur_usec() {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return ((uint64_t)tv.tv_sec) * 1000000 + tv.tv_usec;
  }
  uint64_t start;
};

inline string pad(const string& orig, size_t s) {
  if (orig.size() > s) return orig;
  stringstream buf;
  buf << orig;
  buf << string(s - orig.size(), ' ');
  return buf.str();
}

void run_test(bool success, const string& srcfile, size_t alignSize) {
  // check to see if an .stdout file exists for srcfile.
  // if so, then we want to do execution also
  //
  // TODO: do the same for stderr
  string stdoutFname = util::strip_extension(srcfile) + ".stdout";
  fstream stdoutFile(stdoutFname.c_str());
  string childOutput;
  string childExpect;

  bool capture;
  if (success && stdoutFile.good()) {
    global_compile_opts.semantic_check_only = false;
    capture = true;

    // slurp the file into childExpect
    stringstream buf;
    buf << stdoutFile.rdbuf();
    childExpect = buf.str();
  } else {
    global_compile_opts.semantic_check_only = true;
    capture = false;
  }

  Timer t;

  // run test in separate process, so that any nasty errors don't
  // break the whole test suite
  int fds[2];
  if (capture) {
    if (pipe(fds) != 0) {
      cerr << "Fatal error: could not pipe (errno: " << errno << ")" << endl;
      exit(1);
    }
  }

  pid_t pid = fork();
  int status;
  if (pid == 0) {
    // child

    if (capture) {
      close(fds[0]);
      if (dup2(fds[1], STDOUT_FILENO) == -1) {
        cerr << "Fatal error: could not dup2 (errno: " << errno << ")" << endl;
        exit(1);
      }
      close(fds[1]);
    }

    compile_result result;
    bool res = compile_and_exec(srcfile, result);
    _exit(res ? 0 : 1); // *must* be _exit() *not* exit()
  } else if (pid < 0) {
    // error in fork
    cerr << "Fatal error: could not fork (errno: " << errno << ")" << endl;
    exit(1);
  } else {
    // parent

    if (capture) {
      close(fds[1]);
    }

    pid_t ret = waitpid(pid, &status, 0);
    if (ret == -1) {
      cerr << "Fatal error: waitpid (errno: " << errno << ")" << endl;
      exit(1);
    }

    if (capture) {
      stringstream ss;
      char buf[1024];
      ssize_t n;
      while ((n = read(fds[0], buf, sizeof(buf))) > 0) {
        ss << string(buf, n);
      }
      childOutput = ss.str();
    }
  }

  bool res;
  if (WIFEXITED(status)) {
    res = WEXITSTATUS(status) == 0;
  } else if (WIFSIGNALED(status)) {
    res = false;
  } else VENOM_NOT_REACHED;

  if (res && capture) {
    // compare outputs
    res = childExpect == childOutput;
  }

  double exec_ms = t.lap_ms();
  cout.setf(ios::fixed, ios::floatfield);
  cout.precision(3);
  if (success) {
    if (res) {
      cout << "File " << pad(srcfile, alignSize) << " (" << exec_ms << " ms) [OK]" << endl;
    } else {
      cout << "File " << pad(srcfile, alignSize) << " (" << exec_ms << " ms) [FAILED]" << endl;
    }
  } else {
    if (!res) {
      cout << "File " << pad(srcfile, alignSize) << " (" << exec_ms << " ms) [OK]" << endl;
    } else {
      cout << "File " << pad(srcfile, alignSize) << " (" << exec_ms << " ms) [FAILED]" << endl;
    }
  }
  cout.unsetf(ios::floatfield);
}

struct max_size_functor_t {
  inline bool operator()(const string& largest, const string& elem) const {
    return elem.size() > largest.size();
  }
} max_size_functor;

void run_tests(bool success, const vector<string>& srcfiles, const string& import_path) {
  global_compile_opts.venom_import_path = import_path;
  vector<string>::const_iterator largest =
    max_element(srcfiles.begin(), srcfiles.end(), max_size_functor);
  for (vector<string>::const_iterator it = srcfiles.begin();
       it != srcfiles.end(); ++it) {
    run_test(success, *it, largest->size());
  }
}

int main(int argc, char **argv) {
  string success_dir = "../test/success";
  string failure_dir = "../test/failure";
  while (true) {
    static struct option long_options[] = {
      {"success-dir", required_argument, 0, 's'},
      {"failure-dir", required_argument, 0, 'b'},
      {0, 0, 0, 0}
    };
    int option_index = 0;
    int c = getopt_long(argc, argv, "s:f:",
                        long_options, &option_index);
    if (c == -1) break;
    switch (c) {
    case 's':
      success_dir = optarg;
      break;
    case 'b':
      failure_dir = optarg;
      break;
    case '?':
      /* getopt_long already printed an error message. */
      break;
    default: assert(false);
    }
  }

  vector<string> success_files;
  if (!util::listfiles(success_dir, success_files,
                       util::listfiles_ext_filter("venom"), true)) {
    cerr << "Cannot open directory: " << success_dir << endl;
    return 1;
  }

  vector<string> failure_files;
  if (!util::listfiles(failure_dir, failure_files,
                       util::listfiles_ext_filter("venom"), true)) {
    cerr << "Cannot open directory: " << failure_dir << endl;
    return 1;
  }

  cout << "Running success tests" << endl;
  run_tests(true, success_files, success_dir);

  cout << endl << "Running failure tests" << endl;
  run_tests(false, failure_files, failure_dir);

  return 0;
}
