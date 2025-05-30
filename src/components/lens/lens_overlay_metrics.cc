// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/lens/lens_overlay_metrics.h"

#include <cstddef>

#include "base/metrics/histogram_functions.h"
#include "base/metrics/user_metrics.h"
#include "base/time/time.h"
#include "components/lens/lens_features.h"
#include "components/lens/lens_overlay_mime_type.h"
#include "services/metrics/public/cpp/ukm_builders.h"

namespace lens {

std::string InvocationSourceToString(
    LensOverlayInvocationSource invocation_source) {
  switch (invocation_source) {
    case LensOverlayInvocationSource::kAppMenu:
      return "AppMenu";
    case LensOverlayInvocationSource::kContentAreaContextMenuPage:
      return "ContentAreaContextMenuPage";
    case LensOverlayInvocationSource::kContentAreaContextMenuImage:
      return "ContentAreaContextMenuImage";
    case LensOverlayInvocationSource::kToolbar:
      return "Toolbar";
    case LensOverlayInvocationSource::kFindInPage:
      return "FindInPage";
    case LensOverlayInvocationSource::kOmnibox:
      return "Omnibox";
  }
}

std::string FirstInteractionTypeToString(
    LensOverlayFirstInteractionType interaction_type) {
  switch (interaction_type) {
    case LensOverlayFirstInteractionType::kPermissionDialog:
      return "Permission";
    case LensOverlayFirstInteractionType::kLensMenu:
      return "LensMenu";
    case LensOverlayFirstInteractionType::kRegionSelect:
      return "RegionSelect";
    case LensOverlayFirstInteractionType::kTextSelect:
      return "TextSelect";
    case LensOverlayFirstInteractionType::kSearchbox:
      return "Searchbox";
  }
}

std::string MimeTypeToMetricString(lens::MimeType mime_type) {
  switch (mime_type) {
    case lens::MimeType::kPdf:
      return "Pdf";
    case lens::MimeType::kHtml:
      return "Html";
    case lens::MimeType::kPlainText:
      return "PlainText";
    default:
      return "Unknown";
  }
}

void RecordPermissionRequestedToBeShown(
    bool shown,
    LensOverlayInvocationSource invocation_source) {
  base::UmaHistogramBoolean("Lens.Overlay.PermissionBubble.Shown", shown);
  const auto histogram_name =
      "Lens.Overlay.PermissionBubble.ByInvocationSource." +
      InvocationSourceToString(invocation_source) + ".Shown";
  base::UmaHistogramBoolean(histogram_name, shown);
}

void RecordPermissionUserAction(LensPermissionUserAction user_action,
                                LensOverlayInvocationSource invocation_source) {
  base::UmaHistogramEnumeration("Lens.Overlay.PermissionBubble.UserAction",
                                user_action);
  const auto histogram_name =
      "Lens.Overlay.PermissionBubble.ByInvocationSource." +
      InvocationSourceToString(invocation_source) + ".UserAction";
  base::UmaHistogramEnumeration(histogram_name, user_action);
}

void RecordInvocation(LensOverlayInvocationSource invocation_source,
                      lens::MimeType document_content_type) {
  base::UmaHistogramEnumeration("Lens.Overlay.Invoked", invocation_source);

  // UMA Invocation sliced by document type.
  const auto sliced_invoked_histogram_name =
      "Lens.Overlay.ByDocumentType." +
      MimeTypeToMetricString(document_content_type) + ".Invoked";
  base::UmaHistogramBoolean(sliced_invoked_histogram_name, true);
}

void RecordDismissal(LensOverlayDismissalSource dismissal_source) {
  base::UmaHistogramEnumeration("Lens.Overlay.Dismissed", dismissal_source);
}

void RecordInvocationResultedInSearch(
    LensOverlayInvocationSource invocation_source,
    bool search_performed_in_session) {
  // UMA unsliced InvocationResultedInSearch.
  base::UmaHistogramBoolean("Lens.Overlay.InvocationResultedInSearch",
                            search_performed_in_session);

  // UMA InvocationResultedInSearch sliced by entry point.
  const auto sliced_search_performed_histogram_name =
      "Lens.Overlay.ByInvocationSource." +
      InvocationSourceToString(invocation_source) +
      ".InvocationResultedInSearch";
  base::UmaHistogramBoolean(sliced_search_performed_histogram_name,
                            search_performed_in_session);
}

void RecordSessionDuration(LensOverlayInvocationSource invocation_source,
                           base::TimeDelta duration) {
  // UMA unsliced session duration.
  base::UmaHistogramCustomTimes("Lens.Overlay.SessionDuration", duration,
                                /*min=*/base::Milliseconds(1),
                                /*max=*/base::Minutes(10), /*buckets=*/50);

  // UMA session duration sliced by entry point.
  const auto sliced_session_duration_histogram_name =
      "Lens.Overlay.ByInvocationSource." +
      InvocationSourceToString(invocation_source) + ".SessionDuration";
  base::UmaHistogramCustomTimes(sliced_session_duration_histogram_name,
                                duration,
                                /*min=*/base::Milliseconds(1),
                                /*max=*/base::Minutes(10), /*buckets=*/50);
}

void RecordContextualSearchboxSessionEndMetrics(
    ukm::SourceId source_id,
    bool contextual_searchbox_shown_in_session,
    bool contextual_searchbox_focused_in_session,
    bool contextual_zps_shown_in_session,
    bool contextual_zps_used_in_session,
    bool contextual_query_issued_in_session,
    lens::MimeType page_content_type) {
  // Only record if the contextual search box feature is enabled.
  if (!lens::features::IsLensOverlayContextualSearchboxEnabled()) {
    return;
  }

  // UMA contextual searchbox shown in session.
  base::UmaHistogramBoolean("Lens.Overlay.ContextualSearchBox.ShownInSession",
                            contextual_searchbox_shown_in_session);

  // UMA contextual searchbox shown in session sliced by document type.
  const auto sliced_invoked_histogram_name =
      "Lens.Overlay.ContextualSearchBox.ByPageContentType." +
      MimeTypeToMetricString(page_content_type) + ".ShownInSession";
  base::UmaHistogramBoolean(sliced_invoked_histogram_name,
                            contextual_searchbox_shown_in_session);

  // Record UKM for contextual search box shown in session.
  if (source_id != ukm::kInvalidSourceId) {
    ukm::builders::Lens_Overlay_ContextualSearchBox_ShownInSession event(
        source_id);
    event.SetWasShown(contextual_searchbox_shown_in_session)
        .SetPageContentType(static_cast<int64_t>(page_content_type))
        .Record(ukm::UkmRecorder::Get());
  }

  // Don't record the rest of the contextual searchbox metrics if it was not
  // shown in the session.
  if (!contextual_searchbox_shown_in_session) {
    return;
  }

  // UMA contextual searchbox focused in session.
  base::UmaHistogramBoolean("Lens.Overlay.ContextualSearchBox.FocusedInSession",
                            contextual_searchbox_focused_in_session);
  // UMA contextual searchbox focused in session sliced by document type.
  const auto sliced_focused_histogram_name =
      "Lens.Overlay.ContextualSearchBox.ByPageContentType." +
      MimeTypeToMetricString(page_content_type) + ".FocusedInSession";
  base::UmaHistogramBoolean(sliced_focused_histogram_name,
                            contextual_searchbox_focused_in_session);

  // UMA contextual zps shown in session.
  base::UmaHistogramBoolean("Lens.Overlay.ContextualSuggest.ZPS.ShownInSession",
                            contextual_zps_shown_in_session);
  // UMA contextual zps shown in session sliced by document type.
  const auto sliced_contextual_zps_histogram_name =
      "Lens.Overlay.ContextualSuggest.ZPS.ByPageContentType." +
      MimeTypeToMetricString(page_content_type) + ".ShownInSession";
  base::UmaHistogramBoolean(sliced_contextual_zps_histogram_name,
                            contextual_zps_shown_in_session);

  // UMA contextual zps used in session.
  base::UmaHistogramBoolean(
      "Lens.Overlay.ContextualSuggest.ZPS.SuggestionUsedInSession",
      contextual_zps_used_in_session);
  // UMA contextual zps used in session sliced by document type.
  const auto sliced_contextual_zps_used_histogram_name =
      "Lens.Overlay.ContextualSuggest.ZPS.ByPageContentType." +
      MimeTypeToMetricString(page_content_type) + ".SuggestionUsedInSession";
  base::UmaHistogramBoolean(sliced_contextual_zps_used_histogram_name,
                            contextual_zps_used_in_session);

  // UMA contextual query issued in session.
  base::UmaHistogramBoolean(
      "Lens.Overlay.ContextualSuggest.QueryIssuedInSession",
      contextual_query_issued_in_session);
  // UMA contextual query issued in session sliced by document type.
  const auto sliced_contextual_query_issued_histogram_name =
      "Lens.Overlay.ContextualSuggest.ByPageContentType." +
      MimeTypeToMetricString(page_content_type) + ".QueryIssuedInSession";
  base::UmaHistogramBoolean(sliced_contextual_query_issued_histogram_name,
                            contextual_query_issued_in_session);

  if (source_id == ukm::kInvalidSourceId) {
    return;
  }

  // UKM contextual searchbox focused in session.
  ukm::builders::Lens_Overlay_ContextualSearchbox_FocusedInSession(source_id)
      .SetFocusedInSession(contextual_searchbox_focused_in_session)
      .SetPageContentType(static_cast<int64_t>(page_content_type))
      .Record(ukm::UkmRecorder::Get());

  // UKM contextual zps shown in session.
  ukm::builders::Lens_Overlay_ContextualSuggest_ZPS_ShownInSession(source_id)
      .SetShownInSession(contextual_zps_shown_in_session)
      .SetPageContentType(static_cast<int64_t>(page_content_type))
      .Record(ukm::UkmRecorder::Get());

  // UKM contextual zps used in session.
  ukm::builders::Lens_Overlay_ContextualSuggest_ZPS_SuggestionUsedInSession(
      source_id)
      .SetSuggestionUsedInSession(contextual_zps_used_in_session)
      .SetPageContentType(static_cast<int64_t>(page_content_type))
      .Record(ukm::UkmRecorder::Get());

  // UKM contextual query issued in session.
  ukm::builders::Lens_Overlay_ContextualSuggest_QueryIssuedInSession(source_id)
      .SetQueryIssuedInSession(contextual_query_issued_in_session)
      .SetPageContentType(static_cast<int64_t>(page_content_type))
      .Record(ukm::UkmRecorder::Get());
}

void RecordSessionForegroundDuration(
    LensOverlayInvocationSource invocation_source,
    base::TimeDelta duration) {
  // UMA unsliced session duration.
  base::UmaHistogramCustomTimes("Lens.Overlay.SessionForegroundDuration",
                                duration,
                                /*min=*/base::Milliseconds(1),
                                /*max=*/base::Minutes(10), /*buckets=*/50);

  // UMA session duration sliced by entry point.
  const auto sliced_session_duration_histogram_name =
      "Lens.Overlay.ByInvocationSource." +
      InvocationSourceToString(invocation_source) +
      ".SessionForegroundDuration";
  base::UmaHistogramCustomTimes(sliced_session_duration_histogram_name,
                                duration,
                                /*min=*/base::Milliseconds(1),
                                /*max=*/base::Minutes(10), /*buckets=*/50);
}

void RecordTimeToFirstInteraction(
    LensOverlayInvocationSource invocation_source,
    base::TimeDelta time_to_first_interaction,
    LensOverlayFirstInteractionType first_interaction_type,
    ukm::SourceId source_id) {
  // UMA unsliced TimeToFirstInteraction.
  base::UmaHistogramCustomTimes("Lens.Overlay.TimeToFirstInteraction",
                                time_to_first_interaction,
                                /*min=*/base::Milliseconds(1),
                                /*max=*/base::Minutes(10), /*buckets=*/50);
  // UMA TimeToFirstInteraction sliced by entry point.
  const auto sliced_time_to_first_interaction_histogram_name =
      "Lens.Overlay.ByInvocationSource." +
      InvocationSourceToString(invocation_source) + ".TimeToFirstInteraction";
  base::UmaHistogramCustomTimes(sliced_time_to_first_interaction_histogram_name,
                                time_to_first_interaction,
                                /*min=*/base::Milliseconds(1),
                                /*max=*/base::Minutes(10), /*buckets=*/50);

  // UMA TimeToFirstInteraction sliced by interaction type.
  const auto interaction_type_histogram_name =
      "Lens.Overlay.TimeToFirstInteraction." +
      FirstInteractionTypeToString(first_interaction_type);

  base::UmaHistogramCustomTimes(interaction_type_histogram_name,
                                time_to_first_interaction,
                                /*min=*/base::Milliseconds(1),
                                /*max=*/base::Minutes(10), /*buckets=*/50);

  if (source_id == ukm::kInvalidSourceId) {
    return;
  }

  // UKM unsliced TimeToFirstInteraction.
  ukm::builders::Lens_Overlay_TimeToFirstInteraction(source_id)
      .SetAllEntryPoints(time_to_first_interaction.InMilliseconds())
      .SetFirstInteractionType(static_cast<int64_t>(first_interaction_type))
      .Record(ukm::UkmRecorder::Get());
  // UKM TimeToFirstInteraction sliced by entry point.
  ukm::builders::Lens_Overlay_TimeToFirstInteraction event(source_id);
  switch (invocation_source) {
    case lens::LensOverlayInvocationSource::kAppMenu:
      event.SetAppMenu(time_to_first_interaction.InMilliseconds());
      break;
    case lens::LensOverlayInvocationSource::kContentAreaContextMenuPage:
      event.SetContentAreaContextMenuPage(
          time_to_first_interaction.InMilliseconds());
      break;
    case lens::LensOverlayInvocationSource::kContentAreaContextMenuImage:
      // Not recorded since the image menu entry point results in a search
      // without the user having to interact with the overlay. Time to first
      // interaction in this case is essentially zero.
      break;
    case lens::LensOverlayInvocationSource::kToolbar:
      event.SetToolbar(time_to_first_interaction.InMilliseconds());
      break;
    case lens::LensOverlayInvocationSource::kFindInPage:
      event.SetFindInPage(time_to_first_interaction.InMilliseconds());
      break;
    case lens::LensOverlayInvocationSource::kOmnibox:
      event.SetOmnibox(time_to_first_interaction.InMilliseconds());
      break;
  }
  event.SetFirstInteractionType(static_cast<int64_t>(first_interaction_type))
      .Record(ukm::UkmRecorder::Get());
}

void RecordNewTabGenerated(LensOverlayNewTabSource tab_source) {
  base::UmaHistogramEnumeration("Lens.Overlay.GeneratedTab", tab_source);
}

void RecordGeneratedTabCount(int generated_tab_count) {
  base::UmaHistogramCounts100("Lens.Overlay.GeneratedTab.SessionCount",
                              generated_tab_count);
}

void RecordUKMSessionEndMetrics(
    ukm::SourceId source_id,
    LensOverlayInvocationSource invocation_source,
    bool search_performed_in_session,
    base::TimeDelta session_duration,
    lens::MimeType document_content_type,
    std::optional<base::TimeDelta> session_foreground_duration,
    std::optional<int> generated_tab_count) {
  if (source_id == ukm::kInvalidSourceId) {
    return;
  }
  ukm::builders::Lens_Overlay_SessionEnd event(source_id);

  if (session_foreground_duration.has_value()) {
    event.SetSessionForegroundDuration(
        session_foreground_duration->InMilliseconds());
  }
  if (generated_tab_count.has_value()) {
    event.SetGeneratedTabCount(*generated_tab_count);
  }

  event.SetInvocationSource(static_cast<int64_t>(invocation_source))
      .SetInvocationResultedInSearch(search_performed_in_session)
      .SetSessionDuration(session_duration.InMilliseconds())
      .SetInvocationDocumentType(static_cast<int64_t>(document_content_type))
      .Record(ukm::UkmRecorder::Get());
}

void RecordLensResponseTime(base::TimeDelta response_time) {
  base::UmaHistogramTimes("Lens.Overlay.LensResponseTime", response_time);
}

void RecordContextualSearchboxTimeToInteractionAfterNavigation(
    base::TimeDelta time_to_interaction,
    lens::MimeType page_content_type) {
  const auto sliced_time_to_interaction_histogram_name =
      "Lens.Overlay.ContextualSearchBox.ByPageContentType." +
      MimeTypeToMetricString(page_content_type) +
      ".TimeFromNavigationToFirstInteraction";
  base::UmaHistogramCustomTimes(sliced_time_to_interaction_histogram_name,
                                time_to_interaction,
                                /*min=*/base::Milliseconds(1),
                                /*max=*/base::Minutes(10), /*buckets=*/50);
}

void RecordDocumentSizeBytes(lens::MimeType page_content_type,
                             size_t document_size_bytes) {
  const auto sliced_invoked_histogram_name =
      "Lens.Overlay.ByPageContentType." +
      MimeTypeToMetricString(page_content_type) + ".DocumentSize";
  base::UmaHistogramMemoryKB(sliced_invoked_histogram_name,
                             document_size_bytes / 1000);
}

void RecordPdfPageCount(uint32_t page_count) {
  base::UmaHistogramCounts1000("Lens.Overlay.ByPageContentType.Pdf.PageCount",
                               page_count);
}

void RecordSidePanelResultStatus(SidePanelResultStatus status) {
  base::UmaHistogramEnumeration("Lens.Overlay.SidePanelResultStatus", status);
}

}  // namespace lens
