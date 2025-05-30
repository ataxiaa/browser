// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/facilitated_payments/core/browser/ewallet_manager.h"

#include <cstdint>
#include <optional>
#include <vector>

#include "base/functional/callback.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/task_environment.h"
#include "components/autofill/core/browser/data_manager/payments/test_payments_data_manager.h"
#include "components/autofill/core/browser/data_model/ewallet.h"
#include "components/autofill/core/browser/test_utils/autofill_test_utils.h"
#include "components/autofill/core/common/autofill_prefs.h"
#include "components/facilitated_payments/core/browser/ewallet_manager.h"
#include "components/facilitated_payments/core/browser/ewallet_manager_test_api.h"
#include "components/facilitated_payments/core/browser/facilitated_payments_client.h"
#include "components/facilitated_payments/core/browser/mock_facilitated_payments_api_client.h"
#include "components/facilitated_payments/core/browser/mock_facilitated_payments_client.h"
#include "components/facilitated_payments/core/browser/network_api/facilitated_payments_initiate_payment_response_details.h"
#include "components/facilitated_payments/core/browser/network_api/mock_facilitated_payments_network_interface.h"
#include "components/facilitated_payments/core/metrics/facilitated_payments_metrics.h"
#include "components/facilitated_payments/core/utils/facilitated_payments_ui_utils.h"
#include "components/optimization_guide/core/mock_optimization_guide_decider.h"
#include "components/signin/public/identity_manager/account_info.h"
#include "components/sync/test/test_sync_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace payments::facilitated {
namespace {

// Returns an account info that has all the details a logged in account should
// have.
CoreAccountInfo CreateLoggedInAccountInfo() {
  CoreAccountInfo account;
  account.email = "foo@bar.com";
  account.gaia = "foo-gaia-id";
  account.account_id = CoreAccountId::FromGaiaId(account.gaia);
  return account;
}

}  // namespace

class EwalletManagerTest : public testing::Test {
 public:
  EwalletManagerTest()
      : ewallet_manager_(std::make_unique<EwalletManager>(
            &client_, /*api_client_creator=*/
            base::BindOnce(&MockFacilitatedPaymentsApiClient::CreateApiClient),
            &optimization_guide_decider_)) {
    // Using Autofill preferences since we use autofill's infra for syncing
    // eWallets.
    pref_service_ = autofill::test::PrefServiceForTesting();
    payments_data_manager_.SetPrefService(pref_service_.get());
    payments_data_manager_.SetSyncServiceForTest(&sync_service_);
    ON_CALL(client_, GetPaymentsDataManager)
        .WillByDefault(testing::Return(&payments_data_manager_));
    ON_CALL(client_, GetFacilitatedPaymentsNetworkInterface)
        .WillByDefault(testing::Return(&payments_network_interface_));
    ON_CALL(client_, IsInLandscapeMode).WillByDefault(testing::Return(false));
    ON_CALL(client_, GetCoreAccountInfo)
        .WillByDefault(testing::Return(CreateLoggedInAccountInfo()));
    ON_CALL(optimization_guide_decider_,
            CanApplyOptimization(
                testing::_, testing::_,
                testing::A<optimization_guide::OptimizationMetadata*>()))
        .WillByDefault(testing::Return(
            optimization_guide::OptimizationGuideDecision::kTrue));
    test_api(*ewallet_manager_)
        .set_scheme(PaymentLinkValidator::Scheme::kShopeePay);

    // `initiate_payment_request_details_` is lazy initialized in the
    // implementation. Initialize it here so tests depending on it won't crash.
    test_api(*ewallet_manager_)
        .set_initiate_payment_request_details(
            std::make_unique<
                FacilitatedPaymentsInitiatePaymentRequestDetails>());
  }

  void FastForwardBy(base::TimeDelta duration) {
    task_environment_.FastForwardBy(duration);
  }

  MockFacilitatedPaymentsApiClient& GetApiClient() {
    return *static_cast<MockFacilitatedPaymentsApiClient*>(
        test_api(*ewallet_manager_).GetApiClient());
  }

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  MockFacilitatedPaymentsClient client_;
  optimization_guide::MockOptimizationGuideDecider optimization_guide_decider_;
  // Order matters here because `ewallet_manager_` keeps a reference
  // to `client_` and `optimization_guide_decider_`.
  std::unique_ptr<EwalletManager> ewallet_manager_;
  std::unique_ptr<PrefService> pref_service_;
  syncer::TestSyncService sync_service_;
  autofill::TestPaymentsDataManager payments_data_manager_;
  MockFacilitatedPaymentsNetworkInterface payments_network_interface_;
};

// The manager checks for API availability after payment link validation.
TEST_F(EwalletManagerTest, ApiClientCheckedForAvailability) {
  payments_data_manager_.AddEwalletForTest(
      autofill::Ewallet(/*instrument_id=*/100, u"nickname",
                        /*display_icon_url=*/GURL("http://www.example.com"),
                        u"ewallet_name", u"account_display_name",
                        /*supported_payment_link_uris=*/
                        {u"^shopeepay:\\/\\/shopeepay\\.com\\.my\\?code=.*$",
                         u"^tngd:\\/\\/tngdigital\\.com\\.my\\?code=.*$"},
                        /*is_fido_enrolled=*/true));
  GURL supportedPaymentLink(
      "shopeepay://shopeepay.com.my?code=https://shopeepay.com.my/"
      "281011051692389958586862838?merchant=Walmart&amount=101&currency=usd");

  EXPECT_CALL(GetApiClient(), IsAvailable(testing::_));

  ewallet_manager_->TriggerEwalletPushPayment(supportedPaymentLink,
                                              GURL("https://www.example.com"));
}

// API availability is not invoked if payment link is not supported by available
// eWallet accounts.
TEST_F(EwalletManagerTest,
       UnsupportedPaymentLink_ApiClientNotCheckedForAvailability) {
  payments_data_manager_.AddEwalletForTest(
      autofill::Ewallet(/*instrument_id=*/100, u"nickname",
                        /*display_icon_url=*/GURL("http://www.example.com"),
                        u"ewallet_name", u"account_display_name",
                        /*supported_payment_link_uris=*/
                        {u"^tngd:\\/\\/tngdigital\\.com\\.my\\?code=.*$"},
                        /*is_fido_enrolled=*/true));
  GURL unsupportedPaymentLink(
      "shopeepay://shopeepay.com.my?code=https://shopeepay.com.my/"
      "281011051692389958586862838?merchant=Walmart&amount=101&currency=usd");

  EXPECT_CALL(GetApiClient(), IsAvailable(testing::_)).Times(0);

  ewallet_manager_->TriggerEwalletPushPayment(unsupportedPaymentLink,
                                              GURL("https://www.example.com"));
}

// API availability is not invoked if payment link is invalid.
TEST_F(EwalletManagerTest,
       InvalidPaymentLink_ApiClientNotCheckedForAvailability) {
  base::HistogramTester histogram_tester;
  payments_data_manager_.AddEwalletForTest(
      autofill::Ewallet(/*instrument_id=*/100, u"nickname",
                        /*display_icon_url=*/GURL("http://www.example.com"),
                        u"ewallet_name", u"account_display_name",
                        /*supported_payment_link_uris=*/
                        {u"^shopeepay:\\/\\/shopeepay\\.com\\.my\\?code=.*$",
                         u"^tngd:\\/\\/tngdigital\\.com\\.my\\?code=.*$"},
                        /*is_fido_enrolled=*/true));
  GURL invalidPaymentLink("invalid://payment");

  EXPECT_CALL(GetApiClient(), IsAvailable(testing::_)).Times(0);

  ewallet_manager_->TriggerEwalletPushPayment(invalidPaymentLink,
                                              GURL("https://www.example.com"));

  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason",
      /*sample=*/EwalletFlowExitedReason::kLinkIsInvalid,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason.ShopeePay",
      /*sample=*/EwalletFlowExitedReason::kLinkIsInvalid,
      /*expected_bucket_count=*/0);
}

// API availability is not invoked if there is no linked account.
TEST_F(EwalletManagerTest,
       NoEwalletAccount_ApiClientNotCheckedForAvailability) {
  base::HistogramTester histogram_tester;
  GURL supportedPaymentLink(
      "shopeepay://shopeepay.com.my?code=https://shopeepay.com.my/"
      "281011051692389958586862838?merchant=Walmart&amount=101&currency=usd");

  EXPECT_CALL(GetApiClient(), IsAvailable(testing::_)).Times(0);

  ewallet_manager_->TriggerEwalletPushPayment(supportedPaymentLink,
                                              GURL("https://www.example.com"));

  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason",
      /*sample=*/EwalletFlowExitedReason::kNoSupportedEwallet,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason.ShopeePay",
      /*sample=*/EwalletFlowExitedReason::kNoSupportedEwallet,
      /*expected_bucket_count=*/1);
}

// API availability is not invoked if in landscape mode.
TEST_F(EwalletManagerTest, InLandscapeMode_ApiClientNotCheckedForAvailability) {
  base::HistogramTester histogram_tester;
  payments_data_manager_.AddEwalletForTest(
      autofill::Ewallet(/*instrument_id=*/100, u"nickname",
                        /*display_icon_url=*/GURL("http://www.example.com"),
                        u"ewallet_name", u"account_display_name",
                        /*supported_payment_link_uris=*/
                        {u"^shopeepay:\\/\\/shopeepay\\.com\\.my\\?code=.*$",
                         u"^tngd:\\/\\/tngdigital\\.com\\.my\\?code=.*$"},
                        /*is_fido_enrolled=*/true));
  GURL supportedPaymentLink(
      "shopeepay://shopeepay.com.my?code=https://shopeepay.com.my/"
      "281011051692389958586862838?merchant=Walmart&amount=101&currency=usd");

  EXPECT_CALL(client_, IsInLandscapeMode)
      .Times(1)
      .WillOnce(testing::Return(true));
  EXPECT_CALL(GetApiClient(), IsAvailable(testing::_)).Times(0);

  ewallet_manager_->TriggerEwalletPushPayment(supportedPaymentLink,
                                              GURL("https://www.example.com"));

  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason",
      /*sample=*/EwalletFlowExitedReason::kLandscapeScreenOrientation,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason.ShopeePay",
      /*sample=*/EwalletFlowExitedReason::kLandscapeScreenOrientation,
      /*expected_bucket_count=*/1);
}

// API availability is not invoked if payments data manager is not available.
TEST_F(EwalletManagerTest,
       PaymentsDataManagerUnavailable_ApiClientNotCheckedForAvailability) {
  payments_data_manager_.AddEwalletForTest(
      autofill::Ewallet(/*instrument_id=*/100, u"nickname",
                        /*display_icon_url=*/GURL("http://www.example.com"),
                        u"ewallet_name", u"account_display_name",
                        /*supported_payment_link_uris=*/
                        {u"^shopeepay:\\/\\/shopeepay\\.com\\.my\\?code=.*$",
                         u"^tngd:\\/\\/tngdigital\\.com\\.my\\?code=.*$"},
                        /*is_fido_enrolled=*/true));
  GURL supportedPaymentLink(
      "shopeepay://shopeepay.com.my?code=https://shopeepay.com.my/"
      "281011051692389958586862838?merchant=Walmart&amount=101&currency=usd");

  EXPECT_CALL(client_, GetPaymentsDataManager)
      .Times(1)
      .WillOnce(testing::Return(nullptr));
  EXPECT_CALL(GetApiClient(), IsAvailable(testing::_)).Times(0);

  ewallet_manager_->TriggerEwalletPushPayment(supportedPaymentLink,
                                              GURL("https://www.example.com"));
}

// API availability is not invoked if the user has opted out of the eWallet
// flow.
TEST_F(EwalletManagerTest, UserOptedOut_ApiClientNotCheckedForAvailability) {
  base::HistogramTester histogram_tester;
  payments_data_manager_.AddEwalletForTest(
      autofill::Ewallet(/*instrument_id=*/100, u"nickname",
                        /*display_icon_url=*/GURL("http://www.example.com"),
                        u"ewallet_name", u"account_display_name",
                        /*supported_payment_link_uris=*/
                        {u"^shopeepay:\\/\\/shopeepay\\.com\\.my\\?code=.*$",
                         u"^tngd:\\/\\/tngdigital\\.com\\.my\\?code=.*$"},
                        /*is_fido_enrolled=*/true));
  GURL supportedPaymentLink(
      "shopeepay://shopeepay.com.my?code=https://shopeepay.com.my/"
      "281011051692389958586862838?merchant=Walmart&amount=101&currency=usd");
  // Turn off eWallet pref.
  autofill::prefs::SetFacilitatedPaymentsEwallet(pref_service_.get(), false);

  EXPECT_CALL(GetApiClient(), IsAvailable(testing::_)).Times(0);

  ewallet_manager_->TriggerEwalletPushPayment(supportedPaymentLink,
                                              GURL("https://www.example.com"));

  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason",
      /*sample=*/EwalletFlowExitedReason::kUserOptedOut,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason.ShopeePay",
      /*sample=*/EwalletFlowExitedReason::kUserOptedOut,
      /*expected_bucket_count=*/1);
}

// If the facilitated payment API is available, then the manager shows the
// eWallet payment prompt.
TEST_F(EwalletManagerTest, ShowsEwalletPaymentPromptWhenApiClientAvailable) {
  base::HistogramTester histogram_tester;
  autofill::Ewallet ewallet(
      /*instrument_id=*/100, u"nickname",
      /*display_icon_url=*/GURL("http://www.example.com"), u"ewallet_name",
      u"account_display_name",
      /*supported_payment_link_uris=*/
      {u"^shopeepay:\\/\\/shopeepay\\.com\\.my\\?code=.*$",
       u"^tngd:\\/\\/tngdigital\\.com\\.my\\?code=.*$"},
      /*is_fido_enrolled=*/true);
  payments_data_manager_.AddEwalletForTest(ewallet);
  GURL supportedPaymentLink(
      "shopeepay://shopeepay.com.my?code=https://shopeepay.com.my/"
      "281011051692389958586862838?merchant=Walmart&amount=101&currency=usd");

  ewallet_manager_->TriggerEwalletPushPayment(supportedPaymentLink,
                                              GURL("https://www.example.com"));

  EXPECT_CALL(client_,
              ShowEwalletPaymentPrompt(
                  testing::UnorderedElementsAreArray({ewallet}), testing::_));

  test_api(*ewallet_manager_)
      .OnApiAvailabilityReceived(base::TimeTicks::Now() - base::Seconds(2),
                                 /*is_api_available=*/true);

  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.IsApiAvailable.Success.Latency",
      /*sample=*/2000,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.IsApiAvailable.Success.Latency.ShopeePay",
      /*sample=*/2000,
      /*expected_bucket_count=*/1);
}

// If the facilitated payment API is not available, then the manager doesn't
// show the eWallet payment prompt.
TEST_F(EwalletManagerTest,
       NotShowEwalletPaymentPromptWhenApiClientNotAvailable) {
  base::HistogramTester histogram_tester;
  EXPECT_CALL(client_, ShowEwalletPaymentPrompt).Times(0);

  test_api(*ewallet_manager_)
      .OnApiAvailabilityReceived(base::TimeTicks::Now() - base::Seconds(2),
                                 false);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.IsApiAvailable.Failure.Latency",
      /*sample=*/2000,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.IsApiAvailable.Failure.Latency.ShopeePay",
      /*sample=*/2000,
      /*expected_bucket_count=*/1);

  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason",
      /*sample=*/EwalletFlowExitedReason::kApiClientNotAvailable,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason.ShopeePay",
      /*sample=*/EwalletFlowExitedReason::kApiClientNotAvailable,
      /*expected_bucket_count=*/1);
}

// If the user does not select an eWallet account in the payment prompt, request
// for risk data is not made, and progress screen is not shown.
TEST_F(
    EwalletManagerTest,
    EwalletPaymentPromptNotAccepted_LoadRiskDataNotTriggered_ProgressScreenNotShown) {
  EXPECT_CALL(client_, LoadRiskData(testing::_)).Times(0);
  EXPECT_CALL(client_, ShowProgressScreen()).Times(0);

  test_api(*ewallet_manager_)
      .OnEwalletPaymentPromptResult(/*is_prompt_accepted=*/false,
                                    /*selected_instrument_id=*/0);
}

// If the user selects an eWallet account in the payment prompt, request for
// risk data is made, and progress screen is shown.
TEST_F(EwalletManagerTest,
       EwalletPaymentPromptAccepted_LoadRiskDataTriggered_ProgressScreenShown) {
  EXPECT_CALL(client_, LoadRiskData(testing::_));
  EXPECT_CALL(client_, ShowProgressScreen());

  test_api(*ewallet_manager_)
      .OnEwalletPaymentPromptResult(/*is_prompt_accepted=*/true,
                                    /*selected_instrument_id=*/100L);
}

// If the risk data is empty, then the manager does not retrieve a client token
// from the facilitated payments API client.
TEST_F(EwalletManagerTest,
       RiskDataEmpty_GetClientTokenNotCalled_ErrorScreenShown) {
  base::HistogramTester histogram_tester;

  EXPECT_CALL(GetApiClient(), GetClientToken(testing::_)).Times(0);
  EXPECT_CALL(client_, ShowErrorScreen);

  test_api(*ewallet_manager_)
      .OnRiskDataLoaded(base::TimeTicks::Now() - base::Seconds(2),
                        /*risk_data=*/"");

  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.LoadRiskData.Failure.Latency",
      /*sample=*/2000,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.LoadRiskData.Failure.Latency.ShopeePay",
      /*sample=*/2000,
      /*expected_bucket_count=*/1);
}

// If the risk data is not empty, then the manager retrieves a client token from
// the facilitated payments API client.
TEST_F(EwalletManagerTest, RiskDataNotEmpty_GetClientTokenCalled) {
  base::HistogramTester histogram_tester;

  EXPECT_CALL(GetApiClient(), GetClientToken(testing::_));

  test_api(*ewallet_manager_)
      .OnRiskDataLoaded(base::TimeTicks::Now() - base::Seconds(2),
                        /*risk_data=*/"fake risk data");

  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.LoadRiskData.Success.Latency",
      /*sample=*/2000,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.LoadRiskData.Success.Latency.ShopeePay",
      /*sample=*/2000,
      /*expected_bucket_count=*/1);
}

// If the client token is empty, an error screen will be shown.
TEST_F(EwalletManagerTest, OnGetClientToken_ClientTokenEmpty_ErrorScreenShown) {
  base::HistogramTester histogram_tester;

  EXPECT_CALL(client_, ShowErrorScreen);

  test_api(*ewallet_manager_)
      .OnGetClientToken(base::TimeTicks::Now() - base::Seconds(2),
                        std::vector<uint8_t>{});

  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason",
      /*sample=*/EwalletFlowExitedReason::kClientTokenNotAvailable,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason.ShopeePay",
      /*sample=*/EwalletFlowExitedReason::kClientTokenNotAvailable,
      /*expected_bucket_count=*/1);
}

// Test that GetClientToken related metrics are logged correctly.
TEST_F(EwalletManagerTest, LogGetClientTokenResultAndLatency) {
  for (bool get_client_token_result : {true, false}) {
    base::HistogramTester histogram_tester;

    test_api(*ewallet_manager_)
        .OnGetClientToken(base::TimeTicks::Now() - base::Seconds(2),
                          get_client_token_result
                              ? std::vector<uint8_t>{'t', 'o', 'k', 'e', 'n'}
                              : std::vector<uint8_t>{});

    std::string result = get_client_token_result ? "Success" : "Failure";

    histogram_tester.ExpectUniqueSample(
        base::StrCat({"FacilitatedPayments.Ewallet.GetClientToken.", result,
                      ".Latency"}),
        /*sample=*/2000,
        /*expected_bucket_count=*/1);
    histogram_tester.ExpectUniqueSample(
        base::StrCat({"FacilitatedPayments.Ewallet.GetClientToken.", result,
                      ".Latency.ShopeePay"}),
        /*sample=*/2000,
        /*expected_bucket_count=*/1);
  }
}

// Test that SendInitiatePaymentRequest doesn't initiates payment when
// FacilitatedPaymentsNetworkInterface is not available.
TEST_F(
    EwalletManagerTest,
    SendInitiatePaymentRequest_PaymentsNetworkInterfaceNotAvailable_InitiatePaymentNotTriggered) {
  EXPECT_CALL(client_, GetFacilitatedPaymentsNetworkInterface)
      .Times(1)
      .WillOnce(testing::Return(nullptr));

  EXPECT_CALL(payments_network_interface_,
              InitiatePayment(testing::_, testing::_, testing::_))
      .Times(0);
  EXPECT_CALL(client_, ShowErrorScreen);

  test_api(*ewallet_manager_).SendInitiatePaymentRequest();
}

// Test that LogInitiatePaymentAttempt is logged correctly.
TEST_F(EwalletManagerTest, LogInitiatePaymentAttempt) {
  base::HistogramTester histogram_tester;
  EXPECT_CALL(payments_network_interface_,
              InitiatePayment(testing::_, testing::_, testing::_));

  test_api(*ewallet_manager_).SendInitiatePaymentRequest();

  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.InitiatePayment.Attempt",
      /*sample=*/true,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.InitiatePayment.Attempt.ShopeePay",
      /*sample=*/true,
      /*expected_bucket_count=*/1);
}

// Test that if the response from
// `FacilitatedPaymentsNetworkInterface::InitiatePayment` call has failure
// result, purchase action is not invoked. Instead, an error message is shown.
TEST_F(EwalletManagerTest,
       OnInitiatePaymentResponseReceived_FailureResponse_ErrorScreenShown) {
  base::HistogramTester histogram_tester;

  EXPECT_CALL(client_, ShowErrorScreen);
  EXPECT_CALL(GetApiClient(), InvokePurchaseAction).Times(0);

  auto response_details =
      std::make_unique<FacilitatedPaymentsInitiatePaymentResponseDetails>();
  response_details->action_token_ =
      std::vector<uint8_t>{'t', 'o', 'k', 'e', 'n'};
  test_api(*ewallet_manager_)
      .OnInitiatePaymentResponseReceived(
          base::TimeTicks::Now() - base::Seconds(2),
          autofill::payments::PaymentsAutofillClient::PaymentsRpcResult::
              kPermanentFailure,
          std::move(response_details));

  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.InitiatePayment.Failure.Latency",
      /*sample=*/2000,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.InitiatePayment.Failure.Latency.ShopeePay",
      /*sample=*/2000,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason",
      /*sample=*/EwalletFlowExitedReason::kInitiatePaymentFailed,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason.ShopeePay",
      /*sample=*/EwalletFlowExitedReason::kInitiatePaymentFailed,
      /*expected_bucket_count=*/1);
}

// Test that if the response from
// `FacilitatedPaymentsNetworkInterface::InitiatePayment` has empty action
// token, purchase action is not invoked. Instead, an error message is shown.
TEST_F(EwalletManagerTest,
       OnInitiatePaymentResponseReceived_NoActionToken_ErrorScreenShown) {
  base::HistogramTester histogram_tester;

  EXPECT_CALL(client_, ShowErrorScreen);
  EXPECT_CALL(GetApiClient(), InvokePurchaseAction).Times(0);

  auto response_details =
      std::make_unique<FacilitatedPaymentsInitiatePaymentResponseDetails>();
  test_api(*ewallet_manager_)
      .OnInitiatePaymentResponseReceived(
          base::TimeTicks::Now() - base::Seconds(2),
          autofill::payments::PaymentsAutofillClient::PaymentsRpcResult::
              kSuccess,
          std::move(response_details));

  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.InitiatePayment.Success.Latency",
      /*sample=*/2000,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.InitiatePayment.Success.Latency.ShopeePay",
      /*sample=*/2000,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason",
      /*sample=*/EwalletFlowExitedReason::kActionTokenNotAvailable,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason.ShopeePay",
      /*sample=*/EwalletFlowExitedReason::kActionTokenNotAvailable,
      /*expected_bucket_count=*/1);
}

// Test that if the core account is std::nullopt, purchase action is not
// invoked. Instead, an error message is shown.
TEST_F(EwalletManagerTest,
       OnInitiatePaymentResponseReceived_NoCoreAccountInfo_ErrorScreenShown) {
  base::HistogramTester histogram_tester;

  EXPECT_CALL(client_, GetCoreAccountInfo)
      .Times(1)
      .WillOnce(testing::Return(std::nullopt));

  EXPECT_CALL(client_, ShowErrorScreen);
  EXPECT_CALL(GetApiClient(), InvokePurchaseAction).Times(0);

  auto response_details =
      std::make_unique<FacilitatedPaymentsInitiatePaymentResponseDetails>();
  response_details->action_token_ =
      std::vector<uint8_t>{'t', 'o', 'k', 'e', 'n'};
  test_api(*ewallet_manager_)
      .OnInitiatePaymentResponseReceived(
          base::TimeTicks::Now() - base::Seconds(2),
          autofill::payments::PaymentsAutofillClient::PaymentsRpcResult::
              kSuccess,
          std::move(response_details));

  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.InitiatePayment.Success.Latency",
      /*sample=*/2000,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.InitiatePayment.Success.Latency.ShopeePay",
      /*sample=*/2000,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason",
      /*sample=*/EwalletFlowExitedReason::kUserLoggedOut,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason.ShopeePay",
      /*sample=*/EwalletFlowExitedReason::kUserLoggedOut,
      /*expected_bucket_count=*/1);
}

// Test that if the user is logged out, purchase action is not invoked. Instead,
// an error message is shown.
TEST_F(EwalletManagerTest,
       OnInitiatePaymentResponseReceived_LoggedOutProfile_ErrorScreenShown) {
  base::HistogramTester histogram_tester;
  ON_CALL(client_, GetCoreAccountInfo)
      .WillByDefault(testing::Return(CoreAccountInfo()));

  EXPECT_CALL(client_, ShowErrorScreen);
  EXPECT_CALL(GetApiClient(), InvokePurchaseAction).Times(0);

  auto response_details =
      std::make_unique<FacilitatedPaymentsInitiatePaymentResponseDetails>();
  response_details->action_token_ =
      std::vector<uint8_t>{'t', 'o', 'k', 'e', 'n'};
  test_api(*ewallet_manager_)
      .OnInitiatePaymentResponseReceived(
          base::TimeTicks::Now() - base::Seconds(2),
          autofill::payments::PaymentsAutofillClient::PaymentsRpcResult::
              kSuccess,
          std::move(response_details));

  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.InitiatePayment.Success.Latency",
      /*sample=*/2000,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.InitiatePayment.Success.Latency.ShopeePay",
      /*sample=*/2000,
      /*expected_bucket_count=*/1);
}

// Test that the puchase action is invoked after receiving a success response
// from the `FacilitatedPaymentsNetworkInterface::InitiatePayment` call.
TEST_F(EwalletManagerTest,
       OnInitiatePaymentResponseReceived_InvokePurchaseActionTriggered) {
  base::HistogramTester histogram_tester;

  EXPECT_CALL(GetApiClient(), InvokePurchaseAction);

  auto response_details =
      std::make_unique<FacilitatedPaymentsInitiatePaymentResponseDetails>();
  response_details->action_token_ =
      std::vector<uint8_t>{'t', 'o', 'k', 'e', 'n'};
  test_api(*ewallet_manager_)
      .OnInitiatePaymentResponseReceived(
          base::TimeTicks::Now() - base::Seconds(2),
          autofill::payments::PaymentsAutofillClient::PaymentsRpcResult::
              kSuccess,
          std::move(response_details));

  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.InitiatePayment.Success.Latency",
      /*sample=*/2000,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.InitiatePayment.Success.Latency.ShopeePay",
      /*sample=*/2000,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.InitiatePurchaseAction.Attempt",
      /*sample=*/true,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.InitiatePurchaseAction.Attempt.ShopeePay",
      /*sample=*/true,
      /*expected_bucket_count=*/1);
}

// Test that the `EWALLET_MERCHANT_ALLOWLIST` optimization type is
// registered when EwalletManager is created.
TEST_F(EwalletManagerTest, RegisterEwalletAllowlist) {
  ewallet_manager_.reset();

  EXPECT_CALL(optimization_guide_decider_,
              RegisterOptimizationTypes(testing::ElementsAre(
                  optimization_guide::proto::EWALLET_MERCHANT_ALLOWLIST)));

  ewallet_manager_ = std::make_unique<EwalletManager>(
      &client_,
      /*api_client_creator=*/
      base::BindOnce(&MockFacilitatedPaymentsApiClient::CreateApiClient),
      &optimization_guide_decider_);
}

// Test that API availability is invoked for websites in the allowlist.
TEST_F(EwalletManagerTest,
       TriggerEwalletPushPayment_UrlInAllowlist_ApiAvailabilityInvoked) {
  GURL page_url("https://example.com/");
  payments_data_manager_.AddEwalletForTest(autofill::Ewallet(
      /*instrument_id=*/100, u"nickname",
      /*display_icon_url=*/page_url, u"ewallet_name", u"account_display_name",
      /*supported_payment_link_uris=*/
      {u"^shopeepay:\\/\\/shopeepay\\.com\\.my\\?code=.*$",
       u"^tngd:\\/\\/tngdigital\\.com\\.my\\?code=.*$"},
      /*is_fido_enrolled=*/true));
  GURL supported_payment_link(
      "shopeepay://shopeepay.com.my?code=https://shopeepay.com.my/"
      "281011051692389958586862838?merchant=Walmart&amount=101&currency=usd");

  EXPECT_CALL(
      optimization_guide_decider_,
      CanApplyOptimization(
          testing::Eq(page_url),
          testing::Eq(optimization_guide::proto::EWALLET_MERCHANT_ALLOWLIST),
          testing::Matcher<optimization_guide::OptimizationMetadata*>(
              testing::Eq(nullptr))))
      .WillOnce(testing::Return(
          optimization_guide::OptimizationGuideDecision::kTrue));
  EXPECT_CALL(GetApiClient(), IsAvailable);

  ewallet_manager_->TriggerEwalletPushPayment(supported_payment_link, page_url);
}

// Test that API availability is not invoked for webpages not in the
// allowlist.
TEST_F(EwalletManagerTest,
       TriggerEwalletPushPayment_UrlNotInAllowlist_ApiAvailabilityNotInvoked) {
  base::HistogramTester histogram_tester;
  GURL page_url("https://example.com/");
  payments_data_manager_.AddEwalletForTest(
      autofill::Ewallet(/*instrument_id=*/100, u"nickname",
                        /*display_icon_url=*/GURL("http://www.example.com"),
                        u"ewallet_name", u"account_display_name",
                        /*supported_payment_link_uris=*/
                        {u"^shopeepay:\\/\\/shopeepay\\.com\\.my\\?code=.*$",
                         u"^tngd:\\/\\/tngdigital\\.com\\.my\\?code=.*$"},
                        /*is_fido_enrolled=*/true));
  GURL supported_payment_link(
      "shopeepay://shopeepay.com.my?code=https://shopeepay.com.my/"
      "281011051692389958586862838?merchant=Walmart&amount=101&currency=usd");

  EXPECT_CALL(
      optimization_guide_decider_,
      CanApplyOptimization(
          testing::Eq(page_url),
          testing::Eq(optimization_guide::proto::EWALLET_MERCHANT_ALLOWLIST),
          testing::Matcher<optimization_guide::OptimizationMetadata*>(
              testing::Eq(nullptr))))
      .WillOnce(testing::Return(
          optimization_guide::OptimizationGuideDecision::kFalse));
  EXPECT_CALL(GetApiClient(), IsAvailable).Times(0);

  ewallet_manager_->TriggerEwalletPushPayment(supported_payment_link, page_url);

  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason",
      /*sample=*/EwalletFlowExitedReason::kNotInAllowlist,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason.ShopeePay",
      /*sample=*/EwalletFlowExitedReason::kNotInAllowlist,
      /*expected_bucket_count=*/0);
}

// Test that API availability is not invoked if the allowlist is not
// yet available
TEST_F(
    EwalletManagerTest,
    TriggerEwalletPushPayment_AllowlistNotAvailable_ApiAvailabilityNotInvoked) {
  GURL page_url("https://example.com/");
  payments_data_manager_.AddEwalletForTest(
      autofill::Ewallet(/*instrument_id=*/100, u"nickname",
                        /*display_icon_url=*/GURL("http://www.example.com"),
                        u"ewallet_name", u"account_display_name",
                        /*supported_payment_link_uris=*/
                        {u"^shopeepay:\\/\\/shopeepay\\.com\\.my\\?code=.*$",
                         u"^tngd:\\/\\/tngdigital\\.com\\.my\\?code=.*$"},
                        /*is_fido_enrolled=*/true));
  GURL supported_payment_link(
      "shopeepay://shopeepay.com.my?code=https://shopeepay.com.my/"
      "281011051692389958586862838?merchant=Walmart&amount=101&currency=usd");

  EXPECT_CALL(
      optimization_guide_decider_,
      CanApplyOptimization(
          testing::Eq(page_url),
          testing::Eq(optimization_guide::proto::EWALLET_MERCHANT_ALLOWLIST),
          testing::Matcher<optimization_guide::OptimizationMetadata*>(
              testing::Eq(nullptr))))
      .WillOnce(testing::Return(
          optimization_guide::OptimizationGuideDecision::kUnknown));
  EXPECT_CALL(GetApiClient(), IsAvailable).Times(0);

  ewallet_manager_->TriggerEwalletPushPayment(supported_payment_link, page_url);
}

// Test that when the eWallet FOP selector is shown, its latency is logged.
TEST_F(EwalletManagerTest, FopSelectorShown_LatencyHistogramLogged) {
  base::HistogramTester histogram_tester;
  autofill::Ewallet supported_ewallet(
      /*instrument_id=*/100, u"nickname",
      /*display_icon_url=*/GURL("http://www.example.com"), u"ewallet_name",
      u"account_display_name",
      /*supported_payment_link_uris=*/
      {u"^shopeepay:\\/\\/shopeepay\\.com\\.my\\?code=.*$",
       u"^tngd:\\/\\/tngdigital\\.com\\.my\\?code=.*$"},
      /*is_fido_enrolled=*/true);
  payments_data_manager_.AddEwalletForTest(supported_ewallet);
  GURL page_url("https://example.com/");
  GURL supported_payment_link(
      "shopeepay://shopeepay.com.my?code=https://shopeepay.com.my/"
      "281011051692389958586862838?merchant=Walmart&amount=101&currency=usd");

  // Simulate eWallet payment flow is triggered.
  ewallet_manager_->TriggerEwalletPushPayment(supported_payment_link, page_url);
  // Fully mocked time, does not advance by itself.
  FastForwardBy(base::Seconds(2));
  // Simulate that the FOP selector was shown successfully.
  test_api(*ewallet_manager_)
      .ShowEwalletPaymentPrompt({supported_ewallet}, base::DoNothing());
  test_api(*ewallet_manager_).OnUiEvent(UiEvent::kNewScreenShown);

  // Verify that when the eWallet FOP selector is shown, latency histogram is
  // logged.
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.FopSelectorShown."
      "LatencyAfterDetectingPaymentLink",
      /*sample=*/2000,
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.FopSelectorShown."
      "LatencyAfterDetectingPaymentLink.ShopeePay",
      /*sample=*/2000,
      /*expected_bucket_count=*/1);
}

class EwalletManagerTestForUiScreens
    : public EwalletManagerTest,
      public testing::WithParamInterface<UiState> {
 public:
  void SetUp() override {
    // Default state.
    EXPECT_EQ(test_api(*ewallet_manager_).ui_state(), UiState::kHidden);

    switch (ui_state()) {
      case UiState::kFopSelector: {
        const std::vector<autofill::Ewallet> ewallets = {
            autofill::test::CreateEwalletAccount(100L)};
        test_api(*ewallet_manager_)
            .ShowEwalletPaymentPrompt(std::move(ewallets), base::DoNothing());
        break;
      }
      case UiState::kProgressScreen:
        test_api(*ewallet_manager_).ShowProgressScreen();
        break;
      case UiState::kErrorScreen:
        test_api(*ewallet_manager_).ShowErrorScreen();
        break;
      case UiState::kHidden:
        NOTREACHED();
    }
  }

  UiState ui_state() { return GetParam(); }
};

INSTANTIATE_TEST_SUITE_P(EwalletManagerTest,
                         EwalletManagerTestForUiScreens,
                         testing::Values(UiState::kFopSelector,
                                         UiState::kProgressScreen,
                                         UiState::kErrorScreen));

// Test that when a new screen is shown, UI state reflects the current UI being
// shown.
TEST_P(EwalletManagerTestForUiScreens, NewScreenShown) {
  base::HistogramTester histogram_tester;

  // Simulate new screen was shown successfully.
  test_api(*ewallet_manager_).OnUiEvent(UiEvent::kNewScreenShown);

  // Verify feature has updated the UI state.
  EXPECT_EQ(test_api(*ewallet_manager_).ui_state(), ui_state());

  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.UiScreenShown",
      /*sample=*/ui_state(),
      /*expected_bucket_count=*/1);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.UiScreenShown.ShopeePay",
      /*sample=*/ui_state(),
      /*expected_bucket_count=*/1);
}

// Test that when a new screen could not be shown, UI state is updated.
TEST_P(EwalletManagerTestForUiScreens, NewScreenCouldNotBeShown) {
  base::HistogramTester histogram_tester;

  // Simulate new screen could not be shown.
  test_api(*ewallet_manager_).OnUiEvent(UiEvent::kScreenClosedNotByUser);

  // Verify that the UI state is hidden.
  EXPECT_EQ(test_api(*ewallet_manager_).ui_state(), UiState::kHidden);

  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason",
      /*sample=*/EwalletFlowExitedReason::kFopSelectorClosedNotByUser,
      /*expected_bucket_count=*/ui_state() == UiState::kFopSelector ? 1 : 0);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason.ShopeePay",
      /*sample=*/EwalletFlowExitedReason::kFopSelectorClosedNotByUser,
      /*expected_bucket_count=*/ui_state() == UiState::kFopSelector ? 1 : 0);
}

// Test that when the UI screen is closed, but it was not due to a user action,
// the feature updates the UI state.
TEST_P(EwalletManagerTestForUiScreens, ScreenClosedNotByUser) {
  base::HistogramTester histogram_tester;

  // Simulate new screen was shown successfully.
  test_api(*ewallet_manager_).OnUiEvent(UiEvent::kNewScreenShown);
  // Simulate UI screen was closed, but it was not due to a user action.
  test_api(*ewallet_manager_).OnUiEvent(UiEvent::kScreenClosedNotByUser);

  // Verify that the UI state is hidden.
  EXPECT_EQ(test_api(*ewallet_manager_).ui_state(), UiState::kHidden);

  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason",
      /*sample=*/EwalletFlowExitedReason::kFopSelectorClosedNotByUser,
      /*expected_bucket_count=*/ui_state() == UiState::kFopSelector ? 1 : 0);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason.ShopeePay",
      /*sample=*/EwalletFlowExitedReason::kFopSelectorClosedNotByUser,
      /*expected_bucket_count=*/ui_state() == UiState::kFopSelector ? 1 : 0);
}

// Test that when the UI screen is closed by the user, the feature updates the
// UI state.
TEST_P(EwalletManagerTestForUiScreens, ScreenClosedByUser) {
  base::HistogramTester histogram_tester;

  // Simulate new screen was shown successfully.
  test_api(*ewallet_manager_).OnUiEvent(UiEvent::kNewScreenShown);
  // Simulate UI screen was closed by the user.
  test_api(*ewallet_manager_).OnUiEvent(UiEvent::kScreenClosedByUser);

  // Verify that the UI state is hidden.
  EXPECT_EQ(test_api(*ewallet_manager_).ui_state(), UiState::kHidden);

  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason",
      /*sample=*/EwalletFlowExitedReason::kFopSelectorClosedByUser,
      /*expected_bucket_count=*/ui_state() == UiState::kFopSelector ? 1 : 0);
  histogram_tester.ExpectUniqueSample(
      "FacilitatedPayments.Ewallet.PayflowExitedReason.ShopeePay",
      /*sample=*/EwalletFlowExitedReason::kFopSelectorClosedByUser,
      /*expected_bucket_count=*/ui_state() == UiState::kFopSelector ? 1 : 0);
}

// Test that when Chrome fails to invoke purchase action, the error screen is
// shown.
TEST_F(EwalletManagerTest,
       OnTransactionResult_CouldNotInvoke_ErrorScreenShown) {
  EXPECT_CALL(client_, ShowErrorScreen);

  test_api(*ewallet_manager_)
      .OnTransactionResult(PurchaseActionResult::kCouldNotInvoke);
}

// Test that when Chrome is successful in invoking the purchase action, the UI
// screen is dismissed.
TEST_F(EwalletManagerTest, OnTransactionResult_ResultOk_UiScreenDismissed) {
  // `DismissPrompt` is called once when the purchase action result is
  // received, and again when the test fixture destroys the `manager_`.
  EXPECT_CALL(client_, DismissPrompt).Times(2);

  test_api(*ewallet_manager_)
      .OnTransactionResult(PurchaseActionResult::kResultOk);
}

// Test that when Chrome is successful in invoking the purchase action, the UI
// screen is dismissed.
TEST_F(EwalletManagerTest,
       OnTransactionResult_ResultCanceled_UiScreenDismissed) {
  // `DismissPrompt` is called once when the purchase action result is
  // received, and again when the test fixture destroys the `manager_`.
  EXPECT_CALL(client_, DismissPrompt).Times(2);

  test_api(*ewallet_manager_)
      .OnTransactionResult(PurchaseActionResult::kResultCanceled);
}

}  // namespace payments::facilitated
