# MC824H PR #18 hardware test

Initial hardware test completed on MC824H / MCA1R10 firmware CD14e.

Findings from the original PR #18 code:

- indexed `04/11` with index `0x01` tracks the real encoder correctly
- closed live encoder reaches `0` near the endpoint and later settles around `6`
- fully open live encoder is around `2834`
- the old PR #18 incorrectly scaled full open to about `96.7%` because the physical max (`04/12`, effective range `2932`) overwrote the programmed open endpoint
- initialization register traffic happened before API log attachment, so `04/13`, `04/18`, and `04/12` were not directly visible in the captured log

## Manual indexed register checks

With the component address `50.90` and controller `00.03`, these checksums have been verified:

```text
04/11 index 1 CURRENT: 00.03.50.90.08.07.CC.04.11.99.00.01.01.8C
04/12 index 1 PHYSICAL MAX: 00.03.50.90.08.07.CC.04.12.99.00.01.01.8F
04/13 index 1 CLOSED REF: 00.03.50.90.08.07.CC.04.13.99.00.01.01.8E
04/18 index 1 OPEN REF: 00.03.50.90.08.07.CC.04.18.99.00.01.01.85
```

The important next check is `04/13 index 1`. A returned value of `0` would be consistent with the captured closing sequence: live `04/11` reaches `0` before the controller reports `Closed`, then the live encoder later settles around `6` due to the mechanical endpoint/backlash.

The expected final MC824H scale is therefore likely:

```text
04/13 index 1 -> programmed CLOSED reference (likely 0)
04/18 index 1 -> programmed OPEN reference (expected around 2832)
04/11 index 1 -> CURRENT live encoder
04/12 index 1 -> physical encoder maximum (around 2932; do not use as 100%)
```

Do not merge the original PR #18 as-is. It proves the indexed request mechanism, but its scaling logic must keep `04/12` separate from the programmed open endpoint.
