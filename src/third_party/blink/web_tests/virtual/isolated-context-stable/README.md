# isolated-context-stable

This suite runs wpt_internal/isolated-permissions-policy/ tests with
`--isolated-context-origins=https://web-platform.test`. It's an analog
of the `isolated-context` virtual test suite which, compared to that
suite, is the same except this `isolated-context-stable` suite runs
with stable flags.

As of 2024-08-29, there's one test in this suite:
  - `wpt_internal/permissions-policy/`: Verify that the IWA-specific
    APIs are included in the permissions policy results. These APIs
    should only be available in an isolated context which this test
    runs under. See `//third_party/blink/web_tests/VirtualTestSuites`
    for where that's configured.
