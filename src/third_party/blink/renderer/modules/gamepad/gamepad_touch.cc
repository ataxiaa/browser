// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/gamepad/gamepad_touch.h"

#include <array>

namespace blink {

namespace {

NotShared<DOMFloat32Array> ToFloat32Array(const double x, const double y) {
  const std::array<float, 2> values = {static_cast<float>(x),
                                       static_cast<float>(y)};
  return NotShared<DOMFloat32Array>(DOMFloat32Array::Create(values));
}

NotShared<DOMUint32Array> ToUint32Array(const uint32_t width,
                                        const uint32_t height) {
  const std::array<uint32_t, 2> values = {width, height};
  return NotShared<DOMUint32Array>(DOMUint32Array::Create(values));
}

}  // namespace

void GamepadTouch::SetPosition(float x, float y) {
  position_ = ToFloat32Array(x, y);
}

void GamepadTouch::SetSurfaceDimensions(uint32_t x, uint32_t y) {
  if (!surface_dimensions_) {
    surface_dimensions_ = ToUint32Array(x, y);
  }
  has_surface_dimensions_ = true;
}

bool GamepadTouch::IsEqual(const device::GamepadTouch& device_touch) const {
  return device_touch.touch_id == touch_id_ &&
         device_touch.surface_id == surface_id_ &&
         device_touch.has_surface_dimensions == has_surface_dimensions_ &&
         device_touch.x == position_->Item(0) &&
         device_touch.y == position_->Item(1) &&
         device_touch.surface_width == surface_dimensions_->Item(0) &&
         device_touch.surface_height == surface_dimensions_->Item(1);
}

void GamepadTouch::UpdateValuesFrom(const device::GamepadTouch& device_touch,
                                    uint32_t id_offset) {
  touch_id_ = id_offset;
  surface_id_ = device_touch.surface_id;
  position_ = ToFloat32Array(device_touch.x, device_touch.y);
  surface_dimensions_ =
      ToUint32Array(device_touch.surface_width, device_touch.surface_height);
  has_surface_dimensions_ = device_touch.has_surface_dimensions;
}

void GamepadTouch::Trace(Visitor* visitor) const {
  visitor->Trace(position_);
  visitor->Trace(surface_dimensions_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
