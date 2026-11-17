"""Shared pytest configuration: Hypothesis profiles for the fuzz tests.

- ``default``: few examples, for the regular test runs.
- ``fuzz``: many examples and no deadline, for long fuzzing sessions
  (``just fuzz``, or ``pytest --hypothesis-profile=fuzz``).
"""

from __future__ import annotations

import os

from hypothesis import HealthCheck, settings

settings.register_profile(
    "default",
    max_examples=100,
    deadline=None,
    suppress_health_check=[HealthCheck.too_slow],
)
settings.register_profile(
    "fuzz",
    max_examples=20_000,
    deadline=None,
    suppress_health_check=[HealthCheck.too_slow, HealthCheck.data_too_large],
)
settings.load_profile(os.environ.get("HYPOTHESIS_PROFILE", "default"))
