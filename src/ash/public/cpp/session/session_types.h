// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_PUBLIC_CPP_SESSION_SESSION_TYPES_H_
#define ASH_PUBLIC_CPP_SESSION_SESSION_TYPES_H_

#include <stdint.h>

#include <string>

#include "ash/public/cpp/ash_public_export.h"
#include "ash/public/cpp/session/user_info.h"
#include "components/session_manager/session_manager_types.h"

namespace ash {

// The index of the user profile to use. The list is always LRU sorted so that
// index 0 is the currently active user.
using UserIndex = int;

// Represents possible user adding scenarios.
enum class AddUserSessionPolicy {
  // Adding a user is allowed.
  ALLOWED,
  // Disallowed due to primary user's policy.
  ERROR_NOT_ALLOWED_PRIMARY_USER,
  // Disallowed due to no eligible users.
  ERROR_NO_ELIGIBLE_USERS,
  // Disallowed due to reaching maximum supported user.
  ERROR_MAXIMUM_USERS_REACHED,
  // Disallowed multi-profile because device is locked to single user.
  ERROR_LOCKED_TO_SINGLE_USER,
};

// Defines the cycle direction for |CycleActiveUser|.
enum class CycleUserDirection {
  NEXT = 0,  // Cycle to the next user.
  PREVIOUS,  // Cycle to the previous user.
};

// Info about an ash session.
struct ASH_PUBLIC_EXPORT SessionInfo {
  // Whether the screen can be locked.
  bool can_lock_screen = false;

  // Whether the screen should be locked automatically before suspending.
  bool should_lock_screen_automatically = false;

  // Whether the session is in app mode, which includes a kiosk-like mode for
  // fullscreen web content or running a single [forced] Chrome or ARC app.
  bool is_running_in_app_mode = false;

  // Whether the session is a demo session, which is an ephemeral session for
  // Demo Mode.
  bool is_demo_session = false;

  // Sets whether adding a user session to ash is allowed.
  AddUserSessionPolicy add_user_session_policy = AddUserSessionPolicy::ALLOWED;

  // Current state of the ash session.
  session_manager::SessionState state = session_manager::SessionState::UNKNOWN;
};

ASH_PUBLIC_EXPORT bool operator==(const SessionInfo& a, const SessionInfo& b);

// Info about a user session in ash.
struct ASH_PUBLIC_EXPORT UserSession {
  UserSession();
  UserSession(const UserSession& other);
  ~UserSession();

  // A user session id for the user session. It is generated by session manager
  // (chrome) when a user session starts and never changes during the lifetime
  // of the session manager. The number starts at 1 for the first user session
  // and incremented by one for each subsequent user session.
  uint32_t session_id = 0;

  // Contains general user information state, like the account id, display name,
  // and avatar.
  UserInfo user_info;

  // For supervised users only, the email address of the custodian account.
  // Empty for non-supervised users. Available after profile is loaded.
  std::string custodian_email;

  // For supervised users only, the email address of the second custodian
  // account, if any. Available after profile is loaded.
  std::string second_custodian_email;
};

ASH_PUBLIC_EXPORT bool operator==(const UserSession& a, const UserSession& b);

}  // namespace ash

#endif  // ASH_PUBLIC_CPP_SESSION_SESSION_TYPES_H_
