#!/bin/bash
cd /root/myworks/daliui-0312/dali/dali-ui/automated-tests
export LD_LIBRARY_PATH=/root/myworks/daliui-0312/dali-env/opt/lib

PASS=0; FAIL=0

run_test() {
    local tc=$1
    build/src/dali-ui-foundation/tct-dali-ui-foundation-core "$tc" >/dev/null 2>/dev/null
    local exit_code=$?
    if [ "$exit_code" = "0" ]; then
        echo "  PASS: $tc"
        PASS=$((PASS+1))
    else
        echo "  FAIL: $tc (exit=$exit_code)"
        FAIL=$((FAIL+1))
    fi
}

echo "=== dali-ui WebView Automated Tests ==="
echo ""

run_test UtcDaliWebViewNew
run_test UtcDaliWebViewNewWithLocaleAndTimezone
run_test UtcDaliWebViewNewWithParameters
run_test UtcDaliWebViewCopyAndAssignment
run_test UtcDaliWebViewDownCast
run_test UtcDaliWebViewLoadUrl
run_test UtcDaliWebViewLoadHtmlString
run_test UtcDaliWebViewReload
run_test UtcDaliWebViewStopLoading
run_test UtcDaliWebViewSuspendResume
run_test UtcDaliWebViewScrollBy
run_test UtcDaliWebViewGoBackForward
run_test UtcDaliWebViewGetSettings
run_test UtcDaliWebViewGetBackForwardList
run_test UtcDaliWebViewUrlProperty
run_test UtcDaliWebViewUserAgentProperty
run_test UtcDaliWebViewScrollPositionProperty
run_test UtcDaliWebViewVideoHoleEnabledProperty
run_test UtcDaliWebViewMouseEventsEnabledProperty
run_test UtcDaliWebViewKeyEventsEnabledProperty
run_test UtcDaliWebViewPageZoomFactorProperty
run_test UtcDaliWebViewTextZoomFactorProperty
run_test UtcDaliWebViewRegisterPageLoadStartedCallback
run_test UtcDaliWebViewRegisterPageLoadFinishedCallback
run_test UtcDaliWebViewRegisterUrlChangedCallback
run_test UtcDaliWebViewEvaluateJavaScript
run_test UtcDaliWebViewAddJavaScriptMessageHandler
run_test UtcDaliWebViewSetScaleFactor
run_test UtcDaliWebViewClearHistory
run_test UtcDaliWebViewHighlightText
run_test UtcDaliWebViewGetContext
run_test UtcDaliWebViewGetCookieManager
run_test UtcDaliWebViewActivateAccessibility
run_test UtcDaliWebViewAddToScene
run_test UtcDaliWebViewSizeSet

echo ""
echo "=============================="
echo "Total: PASS=$PASS  FAIL=$FAIL"
echo "=============================="
