# MC824H PR #18 hardware test

Initial hardware test completed on MC824H / MCA1R10 firmware CD14e.

Findings from the original PR #18 code:

- indexed `04/11` with index `0x01` tracks the real encoder correctly
- closed settled encoder is around `6`
- fully open encoder is around `2834`
- the old PR #18 incorrectly scaled full open to about `96.7%` because the physical max (`04/12`, observed effective range `2932`) overwrote the programmed open endpoint
- initialization register traffic happened before API log attachment, so the next test branch re-reads the MC824H endpoint registers after startup for visible diagnostics

The branch is being rebased onto current `main` for the next test iteration.
