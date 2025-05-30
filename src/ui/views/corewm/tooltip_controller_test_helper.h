// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_COREWM_TOOLTIP_CONTROLLER_TEST_HELPER_H_
#define UI_VIEWS_COREWM_TOOLTIP_CONTROLLER_TEST_HELPER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "ui/aura/window_observer.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ui/views/corewm/tooltip_controller.h"
#include "ui/views/corewm/tooltip_state_manager.h"
#include "ui/views/view.h"
#include "ui/views/views_export.h"

namespace aura {
class Window;
}

namespace base {
class TimeDelta;
}

namespace views::corewm::test {

// TooltipControllerTestHelper provides access to TooltipControllers private
// state.
class TooltipControllerTestHelper : public aura::WindowObserver {
 public:
  // `root_window` must be non null.
  explicit TooltipControllerTestHelper(aura::Window* root_window);

  TooltipControllerTestHelper(const TooltipControllerTestHelper&) = delete;
  TooltipControllerTestHelper& operator=(const TooltipControllerTestHelper&) =
      delete;

  ~TooltipControllerTestHelper() override;

  TooltipController* controller() { return controller_; }
  void set_controller(TooltipController* controller) {
    controller_ = controller;
  }

  TooltipStateManager* state_manager() {
    return controller_->state_manager_.get();
  }

  // These are mostly cover methods for TooltipController private methods.
  const std::u16string& GetTooltipText();
  aura::Window* GetTooltipParentWindow();
  const aura::Window* GetObservedWindow();
  const gfx::Point& GetTooltipPosition();
  base::TimeDelta GetShowTooltipDelay();
  void HideAndReset();
  void UpdateIfRequired(TooltipTrigger trigger);
  void FireHideTooltipTimer();
  bool IsWillShowTooltipTimerRunning();
  bool IsWillHideTooltipTimerRunning();
  bool IsTooltipVisible();
  void SkipTooltipShowDelay(bool enable);
  void MockWindowActivated(aura::Window* window, bool active);

  // aura::WindowObserver:
  void OnWindowPropertyChanged(aura::Window* window,
                               const void* key,
                               intptr_t old) override;
  void OnWindowDestroyed(aura::Window* window) override;

 private:
  raw_ptr<aura::Window> root_window_;
  raw_ptr<TooltipController> controller_;
};

// Trivial View subclass that lets you set the tooltip text.
class TooltipTestView : public views::View {
  METADATA_HEADER(TooltipTestView, views::View)

 public:
  TooltipTestView();

  TooltipTestView(const TooltipTestView&) = delete;
  TooltipTestView& operator=(const TooltipTestView&) = delete;

  ~TooltipTestView() override;

  void set_tooltip_text(std::u16string tooltip_text) {
    SetCachedTooltipText(tooltip_text);
  }
};

}  // namespace views::corewm::test

#endif  // UI_VIEWS_COREWM_TOOLTIP_CONTROLLER_TEST_HELPER_H_
