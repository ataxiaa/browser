// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/web_applications/preinstalled_web_app_config_utils.h"

#include <optional>

#include "base/auto_reset.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"

#if BUILDFLAG(IS_CHROMEOS_ASH)
#include "ash/constants/ash_switches.h"
#include "chrome/browser/ash/profiles/profile_helper.h"
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

namespace web_app {

namespace {

std::optional<base::FilePath>&
GetPreinstalledWebAppConfigDirMutableForTesting() {
  static base::NoDestructor<std::optional<base::FilePath>>
      g_config_dir_for_testing(std::nullopt);
  return *g_config_dir_for_testing.get();
}

#if BUILDFLAG(IS_CHROMEOS_ASH)
// The sub-directory of the extensions directory in which to scan for external
// web apps (as opposed to external extensions or external ARC apps).
const base::FilePath::CharType kWebAppsSubDirectory[] =
    FILE_PATH_LITERAL("web_apps");
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

}  // namespace

namespace test {

std::optional<base::FilePath> GetPreinstalledWebAppConfigDirForTesting() {
  return GetPreinstalledWebAppConfigDirMutableForTesting();
}

base::AutoReset<std::optional<base::FilePath>>
SetPreinstalledWebAppConfigDirForTesting(const base::FilePath& config_dir) {
  return {&GetPreinstalledWebAppConfigDirMutableForTesting(),  // IN-TEST
                         config_dir};
}

}  // namespace test

base::FilePath GetPreinstalledWebAppConfigDirFromCommandLine(Profile* profile) {
  std::string command_line_directory =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kPreinstalledWebAppsDir);
  if (!command_line_directory.empty())
    return base::FilePath::FromUTF8Unsafe(command_line_directory);

#if BUILDFLAG(IS_CHROMEOS_ASH)
  // As of mid 2018, only Chrome OS has default/external web apps, and
  // chrome::DIR_STANDALONE_EXTERNAL_EXTENSIONS is only defined for OS_LINUX,
  // which includes OS_CHROMEOS.

  // Exclude sign-in and lock screen profiles.
  if (!ash::ProfileHelper::IsUserProfile(profile)) {
    return {};
  }

  if (test::GetPreinstalledWebAppConfigDirForTesting()) {  // IN-TEST
    return *test::GetPreinstalledWebAppConfigDirForTesting();  // IN-TEST
  }

  // For manual testing, you can change s/STANDALONE/USER/, as writing to
  // "$HOME/.config/chromium/test-user/.config/chromium/External
  // Extensions/web_apps" does not require root ACLs, unlike
  // "/usr/share/chromium/extensions/web_apps".
  base::FilePath dir;
  if (base::PathService::Get(chrome::DIR_STANDALONE_EXTERNAL_EXTENSIONS,
                             &dir)) {
    return dir.Append(kWebAppsSubDirectory);
  }

  LOG(ERROR) << "base::PathService::Get failed";
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  return {};
}

base::FilePath GetPreinstalledWebAppExtraConfigDirFromCommandLine(
    Profile* profile) {
#if BUILDFLAG(IS_CHROMEOS_ASH)
  base::FilePath config_dir =
      GetPreinstalledWebAppConfigDirFromCommandLine(profile);
  std::string extra_config_subdir =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          ash::switches::kExtraWebAppsDir);
  if (config_dir.empty() || extra_config_subdir.empty())
    return base::FilePath();
  return config_dir.AppendASCII(extra_config_subdir);
#else
  return base::FilePath();
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)
}

base::FilePath GetPreinstalledWebAppConfigDir(Profile* profile) {
  return GetPreinstalledWebAppConfigDirFromCommandLine(profile);
}

base::FilePath GetPreinstalledWebAppExtraConfigDir(Profile* profile) {
  return GetPreinstalledWebAppExtraConfigDirFromCommandLine(profile);
}

}  // namespace web_app
