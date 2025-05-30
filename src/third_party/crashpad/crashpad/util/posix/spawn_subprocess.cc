// Copyright 2017 The Crashpad Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "util/posix/spawn_subprocess.h"

#include <errno.h>
#include <spawn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "base/check.h"
#include "base/check_op.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"
#include "util/posix/close_multiple.h"

#if BUILDFLAG(IS_ANDROID)
#include <android/api-level.h>
#endif

// crbug.com/1474421: Used to debug clusterfuzz in_process_fuzzer failures.
// Remove this once the bug is fixed.
#if __has_include("chrome/test/fuzzing/in_process_fuzzer_buildflags.h")
#include "chrome/test/fuzzing/in_process_fuzzer_buildflags.h"  // nogncheck
#define DEBUG_CLUSTERFUZZ_FAILURE BUILDFLAG(DEBUG_CLUSTERFUZZ_FAILURE)
#else
#define DEBUG_CLUSTERFUZZ_FAILURE 0
#endif

extern char** environ;

namespace crashpad {

namespace {

#if BUILDFLAG(IS_APPLE)

class PosixSpawnAttr {
 public:
  PosixSpawnAttr() {
    PCHECK((errno = posix_spawnattr_init(&attr_)) == 0)
        << "posix_spawnattr_init";
  }

  PosixSpawnAttr(const PosixSpawnAttr&) = delete;
  PosixSpawnAttr& operator=(const PosixSpawnAttr&) = delete;

  ~PosixSpawnAttr() {
    PCHECK((errno = posix_spawnattr_destroy(&attr_)) == 0)
        << "posix_spawnattr_destroy";
  }

  void SetFlags(short flags) {
    PCHECK((errno = posix_spawnattr_setflags(&attr_, flags)) == 0)
        << "posix_spawnattr_setflags";
  }

  const posix_spawnattr_t* Get() const { return &attr_; }

 private:
  posix_spawnattr_t attr_;
};

class PosixSpawnFileActions {
 public:
  PosixSpawnFileActions() {
    PCHECK((errno = posix_spawn_file_actions_init(&file_actions_)) == 0)
        << "posix_spawn_file_actions_init";
  }

  PosixSpawnFileActions(const PosixSpawnFileActions&) = delete;
  PosixSpawnFileActions& operator=(const PosixSpawnFileActions&) = delete;

  ~PosixSpawnFileActions() {
    PCHECK((errno = posix_spawn_file_actions_destroy(&file_actions_)) == 0)
        << "posix_spawn_file_actions_destroy";
  }

  void AddInheritedFileDescriptor(int fd) {
    PCHECK((errno = posix_spawn_file_actions_addinherit_np(&file_actions_,
                                                           fd)) == 0)
        << "posix_spawn_file_actions_addinherit_np";
  }

  const posix_spawn_file_actions_t* Get() const { return &file_actions_; }

 private:
  posix_spawn_file_actions_t file_actions_;
};

#endif

}  // namespace

bool SpawnSubprocess(const std::vector<std::string>& argv,
                     const std::vector<std::string>* envp,
                     int preserve_fd,
                     bool use_path,
                     void (*child_function)()) {
  // argv_c contains const char* pointers and is terminated by nullptr. This is
  // suitable for passing to posix_spawn*() and execv*(). Although argv_c is not
  // used in the parent process, it must be built in the parent process because
  // it’s unsafe to do so in the child or grandchild process.
  std::vector<const char*> argv_c;
  argv_c.reserve(argv.size() + 1);
  for (const std::string& argument : argv) {
    argv_c.push_back(argument.c_str());
  }
  argv_c.push_back(nullptr);

  std::vector<const char*> envp_c;
  if (envp) {
    envp_c.reserve(envp->size() + 1);
    for (const std::string& variable : *envp) {
      envp_c.push_back(variable.c_str());
    }
    envp_c.push_back(nullptr);
  }

  // The three processes involved are parent, child, and grandchild. The child
  // exits immediately after spawning the grandchild, so the grandchild becomes
  // an orphan and its parent process ID becomes 1. This relieves the parent and
  // child of the responsibility to reap the grandchild with waitpid() or
  // similar. The grandchild is expected to outlive the parent process, so the
  // parent shouldn’t be concerned with reaping it. This approach means that
  // accidental early termination of the handler process will not result in a
  // zombie process.
  pid_t pid = fork();
  if (pid < 0) {
    PLOG(ERROR) << "fork";
    return false;
  }

  if (pid == 0) {
    // Child process.

    if (child_function) {
      child_function();
    }

    // Call setsid(), creating a new process group and a new session, both led
    // by this process. The new process group has no controlling terminal. This
    // disconnects it from signals generated by the parent process’ terminal.
    //
    // setsid() is done in the child instead of the grandchild so that the
    // grandchild will not be a session leader. If it were a session leader, an
    // accidental open() of a terminal device without O_NOCTTY would make that
    // terminal the controlling terminal.
    //
    // It’s not desirable for the grandchild to have a controlling terminal. The
    // grandchild manages its own lifetime, such as by monitoring clients on its
    // own and exiting when it loses all clients and when it deems it
    // appropraite to do so. It may serve clients in different process groups or
    // sessions than its original client, and receiving signals intended for its
    // original client’s process group could be harmful in that case.
    PCHECK(setsid() != -1) << "setsid";

    // &argv_c[0] is a pointer to a pointer to const char data, but because of
    // how C (not C++) works, posix_spawn*() and execv*() want a pointer to
    // a const pointer to char data. They modify neither the data nor the
    // pointers, so the const_cast is safe.
    char* const* argv_for_spawn = const_cast<char* const*>(argv_c.data());

    // This cast is safe for the same reason that the argv_for_spawn cast is.
    char* const* envp_for_spawn =
        envp ? const_cast<char* const*>(envp_c.data()) : environ;

#if BUILDFLAG(IS_ANDROID) && __ANDROID_API__ < 28
    pid = fork();
    if (pid < 0) {
      PLOG(FATAL) << "fork";
    }

    if (pid > 0) {
      // Child process.

      // _exit() instead of exit(), because fork() was called.
      _exit(EXIT_SUCCESS);
    }

    // Grandchild process.

    CloseMultipleNowOrOnExec(STDERR_FILENO + 1, preserve_fd);

    auto execve_fp = use_path ? execvpe : execve;
    execve_fp(argv_for_spawn[0], argv_for_spawn, envp_for_spawn);
    PLOG(FATAL) << (use_path ? "execvpe" : "execve") << " "
                << argv_for_spawn[0];
#else
#if BUILDFLAG(IS_APPLE)
    PosixSpawnAttr attr;
    attr.SetFlags(POSIX_SPAWN_CLOEXEC_DEFAULT);

    PosixSpawnFileActions file_actions;
    for (int fd = 0; fd <= STDERR_FILENO; ++fd) {
      file_actions.AddInheritedFileDescriptor(fd);
    }
    file_actions.AddInheritedFileDescriptor(preserve_fd);

    const posix_spawnattr_t* attr_p = attr.Get();
    const posix_spawn_file_actions_t* file_actions_p = file_actions.Get();
#else
    CloseMultipleNowOrOnExec(STDERR_FILENO + 1, preserve_fd);

    const posix_spawnattr_t* attr_p = nullptr;
    const posix_spawn_file_actions_t* file_actions_p = nullptr;
#endif

#if DEBUG_CLUSTERFUZZ_FAILURE
    // crbug.com/1474421: Used to debug clusterfuzz in_process_fuzzer failures.
    // Log at WARNING level to ensure it gets logged in.
    // Remove this once the bug is fixed.
    LOG(WARNING) << "[DEBUG] " << std::string(argv_for_spawn[0]);
#endif
    auto posix_spawn_fp = use_path ? posix_spawnp : posix_spawn;
    if ((errno = posix_spawn_fp(nullptr,
                                argv_for_spawn[0],
                                file_actions_p,
                                attr_p,
                                argv_for_spawn,
                                envp_for_spawn)) != 0) {
      PLOG(FATAL) << (use_path ? "posix_spawnp" : "posix_spawn") << " "
                  << argv_for_spawn[0];
    }

    // _exit() instead of exit(), because fork() was called.
    _exit(EXIT_SUCCESS);
#endif
  }

  // waitpid() for the child, so that it does not become a zombie process. The
  // child normally exits quickly.
  //
  // Failures from this point on may result in the accumulation of a zombie, but
  // should not be considered fatal. Log only warnings, but don’t treat these
  // failures as a failure of the function overall.
  int status;
  pid_t wait_pid = HANDLE_EINTR(waitpid(pid, &status, 0));
  if (wait_pid == -1) {
    PLOG(WARNING) << "waitpid";
    return true;
  }
  DCHECK_EQ(wait_pid, pid);

  if (WIFSIGNALED(status)) {
    int sig = WTERMSIG(status);
    LOG(WARNING) << base::StringPrintf(
        "intermediate process terminated by signal %d (%s)%s",
        sig,
        strsignal(sig),
        WCOREDUMP(status) ? " (core dumped)" : "");
  } else if (!WIFEXITED(status)) {
    LOG(WARNING) << base::StringPrintf(
        "intermediate process: unknown termination 0x%x", status);
  } else if (WEXITSTATUS(status) != EXIT_SUCCESS) {
    LOG(WARNING) << "intermediate process exited with code "
                 << WEXITSTATUS(status);
  }

  return true;
}

}  // namespace crashpad
