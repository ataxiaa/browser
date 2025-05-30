// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_HID_CHROME_HID_DELEGATE_H_
#define CHROME_BROWSER_HID_CHROME_HID_DELEGATE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "content/public/browser/hid_chooser.h"
#include "content/public/browser/hid_delegate.h"
#include "extensions/buildflags/buildflags.h"
#include "services/device/public/mojom/hid.mojom-forward.h"
#include "third_party/blink/public/mojom/hid/hid.mojom-forward.h"
#include "url/origin.h"

namespace content {
class BrowserContext;
class RenderFrameHost;
struct GlobalRenderFrameHostId;
}  // namespace content

class HidChooser;

class ChromeHidDelegate : public content::HidDelegate {
 public:
  ChromeHidDelegate();
  ChromeHidDelegate(ChromeHidDelegate&) = delete;
  ChromeHidDelegate& operator=(ChromeHidDelegate&) = delete;
  ~ChromeHidDelegate() override;

  // content::HidDelegate:
  std::unique_ptr<content::HidChooser> RunChooser(
      content::RenderFrameHost* render_frame_host,
      std::vector<blink::mojom::HidDeviceFilterPtr> filters,
      std::vector<blink::mojom::HidDeviceFilterPtr> exclusion_filters,
      content::HidChooser::Callback callback) override;
  bool CanRequestDevicePermission(content::BrowserContext* browser_context,
                                  const url::Origin& origin) override;
  bool HasDevicePermission(content::BrowserContext* browser_context,
                           content::RenderFrameHost* render_frame_host,
                           const url::Origin& origin,
                           const device::mojom::HidDeviceInfo& device) override;
  void RevokeDevicePermission(
      content::BrowserContext* browser_context,
      content::RenderFrameHost* render_frame_host,
      const url::Origin& origin,
      const device::mojom::HidDeviceInfo& device) override;
  device::mojom::HidManager* GetHidManager(
      content::BrowserContext* browser_context) override;
  void AddObserver(content::BrowserContext* browser_context,
                   content::HidDelegate::Observer* observer) override;
  void RemoveObserver(content::BrowserContext* browser_context,
                      content::HidDelegate::Observer* observer) override;
  const device::mojom::HidDeviceInfo* GetDeviceInfo(
      content::BrowserContext* browser_context,
      const std::string& guid) override;
  bool IsFidoAllowedForOrigin(content::BrowserContext* browser_context,
                              const url::Origin& origin) override;
  bool IsKnownSecurityKey(content::BrowserContext* browser_context,
                          const device::mojom::HidDeviceInfo& device) override;
  bool IsServiceWorkerAllowedForOrigin(const url::Origin& origin) override;
  void IncrementConnectionCount(content::BrowserContext* browser_context,
                                const url::Origin& origin) override;
  void DecrementConnectionCount(content::BrowserContext* browser_context,
                                const url::Origin& origin) override;

 private:
  class ContextObservation;

  ContextObservation* GetContextObserver(
      content::BrowserContext* browser_context);

#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Opens a device chooser for the frame with id `embedder_rfh_id` if `allow`
  // is true.
  virtual void OnWebViewHidPermissionRequestCompleted(
      base::WeakPtr<HidChooser> chooser,
      content::GlobalRenderFrameHostId embedder_rfh_id,
      std::vector<blink::mojom::HidDeviceFilterPtr> filters,
      std::vector<blink::mojom::HidDeviceFilterPtr> exclusion_filters,
      content::HidChooser::Callback callback,
      bool allow);
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

  base::flat_map<content::BrowserContext*, std::unique_ptr<ContextObservation>>
      observations_;
};

#endif  // CHROME_BROWSER_HID_CHROME_HID_DELEGATE_H_
