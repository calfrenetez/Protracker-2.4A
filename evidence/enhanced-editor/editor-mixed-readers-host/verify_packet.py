"""Offline HOST v2 evidence verifier; ordinary packet reads only, no imports/products/subprocess/targets."""
from pathlib import Path, PurePosixPath
import hashlib
import json
import stat

BASELINE = 'a02d53f2047753fd8673cbcc377d772da1ceb531'
ATTEMPTS = ['attempt-lvqgmuc5', 'attempt-vnbghqxd', 'attempt-exq1v9l8', 'attempt-abu1j_nf']
FAILURES = ATTEMPTS[:2]
PASSES = ATTEMPTS[2:]
CURRENT = ATTEMPTS[-1]
PREPARATION = ['prepare-head', 'prepare-index', 'prepare-archive', 'prepare-font']
VERIFICATION = ['verify-head', 'verify-index']
SAFE_LOGS = {'prepare-head', 'prepare-index', 'verify-head', 'verify-index',
             'controller-compile', 'controller-run'}
GENERATED = {'attempts.json', 'original-bindings.json', 'README.md', 'verify_packet.py'}
COPIED_COUNT = 58
INVENTORY_COUNT = 62
TOTAL_COUNT = 63
STDOUT_PIN = {'bytes': 2273, 'sha256':
              'b3afac00b309d959418568141eb81a0977fabfbfdfde3eaa7688b1d2167ff22e'}
README_PIN = {'bytes': 1894, 'sha256': 'daccc2c2bd669143678b38eb46818a65cc7ff418f6c678d4383255d985d70647'}
# Immutable saved originals and independently planned sanitized byte identities.
# These bindings include both independent review/verification pairs.
COPY_SPECS = {
  "attempt-lvqgmuc5/run.json": {
    "source": "attempt-lvqgmuc5/run.json",
    "original": {
      "bytes": 174039,
      "sha256": "a226ce6b7b201dc8bc7ade4780cbd5d0df1dd4d9ae16cc3f4a6f3ca37f7ab357"
    },
    "published_pin": {
      "bytes": 173427,
      "sha256": "53e6f79460a402ee2d108dd5264cf713e24ccf7fb0fb65afe04273e87c580648"
    },
    "transform": "redact"
  },
  "attempt-lvqgmuc5/prepare-head.stdout": {
    "source": "attempt-lvqgmuc5/prepare-head.stdout",
    "original": {
      "bytes": 41,
      "sha256": "918f0008f33c464fe54006cfead307e5935759a1422c6e451b2474c1a9b1e061"
    },
    "published_pin": {
      "bytes": 41,
      "sha256": "918f0008f33c464fe54006cfead307e5935759a1422c6e451b2474c1a9b1e061"
    },
    "transform": "redact"
  },
  "attempt-lvqgmuc5/prepare-head.stderr": {
    "source": "attempt-lvqgmuc5/prepare-head.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-lvqgmuc5/prepare-index.stdout": {
    "source": "attempt-lvqgmuc5/prepare-index.stdout",
    "original": {
      "bytes": 1197014,
      "sha256": "1601e89645535bd9c804ebdca7488f865c5492dbeaa97740c9cbd31dcd96ef85"
    },
    "published_pin": {
      "bytes": 1197014,
      "sha256": "1601e89645535bd9c804ebdca7488f865c5492dbeaa97740c9cbd31dcd96ef85"
    },
    "transform": "redact"
  },
  "attempt-lvqgmuc5/prepare-index.stderr": {
    "source": "attempt-lvqgmuc5/prepare-index.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-lvqgmuc5/prepare-archive.stderr": {
    "source": "attempt-lvqgmuc5/prepare-archive.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-lvqgmuc5/prepare-font.stderr": {
    "source": "attempt-lvqgmuc5/prepare-font.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-lvqgmuc5/controller-compile.stdout": {
    "source": "attempt-lvqgmuc5/controller-compile.stdout",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-lvqgmuc5/controller-compile.stderr": {
    "source": "attempt-lvqgmuc5/controller-compile.stderr",
    "original": {
      "bytes": 6196,
      "sha256": "d131016fc8f0ccb83ebeed9affcf0803816ecb738a03c64274feb82f3306deaa"
    },
    "published_pin": {
      "bytes": 6196,
      "sha256": "d131016fc8f0ccb83ebeed9affcf0803816ecb738a03c64274feb82f3306deaa"
    },
    "transform": "redact"
  },
  "attempt-vnbghqxd/run.json": {
    "source": "attempt-vnbghqxd/run.json",
    "original": {
      "bytes": 174565,
      "sha256": "61b6320cb0a7e2b1648e63eeb55713525c99153afdc2f554bf8b1bfff97246d3"
    },
    "published_pin": {
      "bytes": 173953,
      "sha256": "5d9477be4078a35e5d11ab735c52af282d15d2d9b368affe354601b8f2b2af5e"
    },
    "transform": "redact"
  },
  "attempt-vnbghqxd/prepare-head.stdout": {
    "source": "attempt-vnbghqxd/prepare-head.stdout",
    "original": {
      "bytes": 41,
      "sha256": "918f0008f33c464fe54006cfead307e5935759a1422c6e451b2474c1a9b1e061"
    },
    "published_pin": {
      "bytes": 41,
      "sha256": "918f0008f33c464fe54006cfead307e5935759a1422c6e451b2474c1a9b1e061"
    },
    "transform": "redact"
  },
  "attempt-vnbghqxd/prepare-head.stderr": {
    "source": "attempt-vnbghqxd/prepare-head.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-vnbghqxd/prepare-index.stdout": {
    "source": "attempt-vnbghqxd/prepare-index.stdout",
    "original": {
      "bytes": 1197014,
      "sha256": "1601e89645535bd9c804ebdca7488f865c5492dbeaa97740c9cbd31dcd96ef85"
    },
    "published_pin": {
      "bytes": 1197014,
      "sha256": "1601e89645535bd9c804ebdca7488f865c5492dbeaa97740c9cbd31dcd96ef85"
    },
    "transform": "redact"
  },
  "attempt-vnbghqxd/prepare-index.stderr": {
    "source": "attempt-vnbghqxd/prepare-index.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-vnbghqxd/prepare-archive.stderr": {
    "source": "attempt-vnbghqxd/prepare-archive.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-vnbghqxd/prepare-font.stderr": {
    "source": "attempt-vnbghqxd/prepare-font.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-vnbghqxd/controller-compile.stdout": {
    "source": "attempt-vnbghqxd/controller-compile.stdout",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-vnbghqxd/controller-compile.stderr": {
    "source": "attempt-vnbghqxd/controller-compile.stderr",
    "original": {
      "bytes": 2397,
      "sha256": "ad70d630a3b3fbb970ea106d6b23cee7f13f6b3fa092045b4510ef2c362c30ac"
    },
    "published_pin": {
      "bytes": 2397,
      "sha256": "ad70d630a3b3fbb970ea106d6b23cee7f13f6b3fa092045b4510ef2c362c30ac"
    },
    "transform": "redact"
  },
  "attempt-exq1v9l8/run.json": {
    "source": "attempt-exq1v9l8/run.json",
    "original": {
      "bytes": 177756,
      "sha256": "c1ba9deeedfde6f45cb27b1fca4cf236362c99f4af9666fdca3c1cd39642f568"
    },
    "published_pin": {
      "bytes": 176784,
      "sha256": "7275b62ef58b46bd21788c2060436d1173a80b7ec89e7b0b474735b23afcf3d5"
    },
    "transform": "redact"
  },
  "attempt-exq1v9l8/prepare-head.stdout": {
    "source": "attempt-exq1v9l8/prepare-head.stdout",
    "original": {
      "bytes": 41,
      "sha256": "918f0008f33c464fe54006cfead307e5935759a1422c6e451b2474c1a9b1e061"
    },
    "published_pin": {
      "bytes": 41,
      "sha256": "918f0008f33c464fe54006cfead307e5935759a1422c6e451b2474c1a9b1e061"
    },
    "transform": "redact"
  },
  "attempt-exq1v9l8/prepare-head.stderr": {
    "source": "attempt-exq1v9l8/prepare-head.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-exq1v9l8/prepare-index.stdout": {
    "source": "attempt-exq1v9l8/prepare-index.stdout",
    "original": {
      "bytes": 1197014,
      "sha256": "1601e89645535bd9c804ebdca7488f865c5492dbeaa97740c9cbd31dcd96ef85"
    },
    "published_pin": {
      "bytes": 1197014,
      "sha256": "1601e89645535bd9c804ebdca7488f865c5492dbeaa97740c9cbd31dcd96ef85"
    },
    "transform": "redact"
  },
  "attempt-exq1v9l8/prepare-index.stderr": {
    "source": "attempt-exq1v9l8/prepare-index.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-exq1v9l8/prepare-archive.stderr": {
    "source": "attempt-exq1v9l8/prepare-archive.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-exq1v9l8/prepare-font.stderr": {
    "source": "attempt-exq1v9l8/prepare-font.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-exq1v9l8/verify-head.stdout": {
    "source": "attempt-exq1v9l8/verify-head.stdout",
    "original": {
      "bytes": 41,
      "sha256": "918f0008f33c464fe54006cfead307e5935759a1422c6e451b2474c1a9b1e061"
    },
    "published_pin": {
      "bytes": 41,
      "sha256": "918f0008f33c464fe54006cfead307e5935759a1422c6e451b2474c1a9b1e061"
    },
    "transform": "redact"
  },
  "attempt-exq1v9l8/verify-head.stderr": {
    "source": "attempt-exq1v9l8/verify-head.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-exq1v9l8/verify-index.stdout": {
    "source": "attempt-exq1v9l8/verify-index.stdout",
    "original": {
      "bytes": 1197014,
      "sha256": "1601e89645535bd9c804ebdca7488f865c5492dbeaa97740c9cbd31dcd96ef85"
    },
    "published_pin": {
      "bytes": 1197014,
      "sha256": "1601e89645535bd9c804ebdca7488f865c5492dbeaa97740c9cbd31dcd96ef85"
    },
    "transform": "redact"
  },
  "attempt-exq1v9l8/verify-index.stderr": {
    "source": "attempt-exq1v9l8/verify-index.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-exq1v9l8/controller-compile.stdout": {
    "source": "attempt-exq1v9l8/controller-compile.stdout",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-exq1v9l8/controller-compile.stderr": {
    "source": "attempt-exq1v9l8/controller-compile.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-exq1v9l8/controller-run.stdout": {
    "source": "attempt-exq1v9l8/controller-run.stdout",
    "original": {
      "bytes": 2273,
      "sha256": "b3afac00b309d959418568141eb81a0977fabfbfdfde3eaa7688b1d2167ff22e"
    },
    "published_pin": {
      "bytes": 2273,
      "sha256": "b3afac00b309d959418568141eb81a0977fabfbfdfde3eaa7688b1d2167ff22e"
    },
    "transform": "redact"
  },
  "attempt-exq1v9l8/controller-run.stderr": {
    "source": "attempt-exq1v9l8/controller-run.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-abu1j_nf/run.json": {
    "source": "attempt-abu1j_nf/run.json",
    "original": {
      "bytes": 177755,
      "sha256": "70631a4bf0e49efa9ced9f34e879cd6ec248f3cf31e668bdf43b1533a2c6ce1e"
    },
    "published_pin": {
      "bytes": 176783,
      "sha256": "693fd30f66a16a23379db7046d4756a271fa85bc3e4aab3253029bdeff0356f2"
    },
    "transform": "redact"
  },
  "attempt-abu1j_nf/prepare-head.stdout": {
    "source": "attempt-abu1j_nf/prepare-head.stdout",
    "original": {
      "bytes": 41,
      "sha256": "918f0008f33c464fe54006cfead307e5935759a1422c6e451b2474c1a9b1e061"
    },
    "published_pin": {
      "bytes": 41,
      "sha256": "918f0008f33c464fe54006cfead307e5935759a1422c6e451b2474c1a9b1e061"
    },
    "transform": "redact"
  },
  "attempt-abu1j_nf/prepare-head.stderr": {
    "source": "attempt-abu1j_nf/prepare-head.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-abu1j_nf/prepare-index.stdout": {
    "source": "attempt-abu1j_nf/prepare-index.stdout",
    "original": {
      "bytes": 1197014,
      "sha256": "1601e89645535bd9c804ebdca7488f865c5492dbeaa97740c9cbd31dcd96ef85"
    },
    "published_pin": {
      "bytes": 1197014,
      "sha256": "1601e89645535bd9c804ebdca7488f865c5492dbeaa97740c9cbd31dcd96ef85"
    },
    "transform": "redact"
  },
  "attempt-abu1j_nf/prepare-index.stderr": {
    "source": "attempt-abu1j_nf/prepare-index.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-abu1j_nf/prepare-archive.stderr": {
    "source": "attempt-abu1j_nf/prepare-archive.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-abu1j_nf/prepare-font.stderr": {
    "source": "attempt-abu1j_nf/prepare-font.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-abu1j_nf/verify-head.stdout": {
    "source": "attempt-abu1j_nf/verify-head.stdout",
    "original": {
      "bytes": 41,
      "sha256": "918f0008f33c464fe54006cfead307e5935759a1422c6e451b2474c1a9b1e061"
    },
    "published_pin": {
      "bytes": 41,
      "sha256": "918f0008f33c464fe54006cfead307e5935759a1422c6e451b2474c1a9b1e061"
    },
    "transform": "redact"
  },
  "attempt-abu1j_nf/verify-head.stderr": {
    "source": "attempt-abu1j_nf/verify-head.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-abu1j_nf/verify-index.stdout": {
    "source": "attempt-abu1j_nf/verify-index.stdout",
    "original": {
      "bytes": 1197014,
      "sha256": "1601e89645535bd9c804ebdca7488f865c5492dbeaa97740c9cbd31dcd96ef85"
    },
    "published_pin": {
      "bytes": 1197014,
      "sha256": "1601e89645535bd9c804ebdca7488f865c5492dbeaa97740c9cbd31dcd96ef85"
    },
    "transform": "redact"
  },
  "attempt-abu1j_nf/verify-index.stderr": {
    "source": "attempt-abu1j_nf/verify-index.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-abu1j_nf/controller-compile.stdout": {
    "source": "attempt-abu1j_nf/controller-compile.stdout",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-abu1j_nf/controller-compile.stderr": {
    "source": "attempt-abu1j_nf/controller-compile.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "attempt-abu1j_nf/controller-run.stdout": {
    "source": "attempt-abu1j_nf/controller-run.stdout",
    "original": {
      "bytes": 2273,
      "sha256": "b3afac00b309d959418568141eb81a0977fabfbfdfde3eaa7688b1d2167ff22e"
    },
    "published_pin": {
      "bytes": 2273,
      "sha256": "b3afac00b309d959418568141eb81a0977fabfbfdfde3eaa7688b1d2167ff22e"
    },
    "transform": "redact"
  },
  "attempt-abu1j_nf/controller-run.stderr": {
    "source": "attempt-abu1j_nf/controller-run.stderr",
    "original": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "published_pin": {
      "bytes": 0,
      "sha256": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    "transform": "redact"
  },
  "reviews/current-v5-review.json": {
    "source": "review-final-v2/review.json",
    "original": {
      "bytes": 19685,
      "sha256": "55b392241186f25ffa94525eee74b68497fdf089d96c264d639555b3e9f7871a"
    },
    "published_pin": {
      "bytes": 19685,
      "sha256": "55b392241186f25ffa94525eee74b68497fdf089d96c264d639555b3e9f7871a"
    },
    "transform": "redact"
  },
  "reviews/current-v5-verification.json": {
    "source": "review-final-v2/verification.json",
    "original": {
      "bytes": 27719,
      "sha256": "85aee0b4db1ed3e25c8ef260351cd5f92188fa3d4b401fca2d11f960aee8b37f"
    },
    "published_pin": {
      "bytes": 27359,
      "sha256": "d335ad9cd7ce9be37664726debcc725e7c67d892a10e02d80d27df8da9520e19"
    },
    "transform": "redact"
  },
  "reviews/historical-v4-review.json": {
    "source": "review-final-v1/review.json",
    "original": {
      "bytes": 15095,
      "sha256": "35078616098561386178b7bf15beff2c7b153b9c017d136dd6c444d9233911e3"
    },
    "published_pin": {
      "bytes": 15095,
      "sha256": "35078616098561386178b7bf15beff2c7b153b9c017d136dd6c444d9233911e3"
    },
    "transform": "redact"
  },
  "reviews/historical-v4-verification.json": {
    "source": "review-final-v1/verification.json",
    "original": {
      "bytes": 19774,
      "sha256": "e3f2081ec13729f572bf4a8be668a651cc6761f7ab6bbc836fa733eccfd298e5"
    },
    "published_pin": {
      "bytes": 19774,
      "sha256": "e3f2081ec13729f572bf4a8be668a651cc6761f7ab6bbc836fa733eccfd298e5"
    },
    "transform": "redact"
  },
  "reviews/root-host-pass-custody-v2.json": {
    "source": "root-host-pass-custody-v2.json",
    "original": {
      "bytes": 5057,
      "sha256": "89fe835adfb2d3bf23afcb2b979000371094169302fc597bcb31a9a3100cc4eb"
    },
    "published_pin": {
      "bytes": 4929,
      "sha256": "0155e6f6e8a4408bdbc1d8d338bcadd40a28b231ce1ffae477f8d621ce998faa"
    },
    "transform": "omit_scope_then_redact"
  },
  "source-history/draft-source-v1.json": {
    "source": "draft-source-v1.json",
    "original": {
      "bytes": 1182,
      "sha256": "b9b4da8445d345522d736ed7c81c382aa3975d7b2d2fcbd32171be52b556eb50"
    },
    "published_pin": {
      "bytes": 1182,
      "sha256": "b9b4da8445d345522d736ed7c81c382aa3975d7b2d2fcbd32171be52b556eb50"
    },
    "transform": "redact"
  },
  "source-history/draft-source-v2.json": {
    "source": "draft-source-v2.json",
    "original": {
      "bytes": 2136,
      "sha256": "1742f586046c835bb8e2a7428eed6446d1655499b962c7e859e80342dad42ace"
    },
    "published_pin": {
      "bytes": 2136,
      "sha256": "1742f586046c835bb8e2a7428eed6446d1655499b962c7e859e80342dad42ace"
    },
    "transform": "redact"
  },
  "source-history/draft-source-v3.json": {
    "source": "draft-source-v3.json",
    "original": {
      "bytes": 2723,
      "sha256": "73ec8da0347efb8bb58057fda422ddbd26b6e3db287b97f87637cc0e8615a083"
    },
    "published_pin": {
      "bytes": 2723,
      "sha256": "73ec8da0347efb8bb58057fda422ddbd26b6e3db287b97f87637cc0e8615a083"
    },
    "transform": "redact"
  },
  "source-history/draft-source-v4.json": {
    "source": "draft-source-v4.json",
    "original": {
      "bytes": 2566,
      "sha256": "1e0f622487615eec44ded0c13d27adf36d75c3c53677c59be6db264fb065c496"
    },
    "published_pin": {
      "bytes": 2566,
      "sha256": "1e0f622487615eec44ded0c13d27adf36d75c3c53677c59be6db264fb065c496"
    },
    "transform": "redact"
  },
  "source-history/draft-source-v5.json": {
    "source": "draft-source-v5.json",
    "original": {
      "bytes": 2517,
      "sha256": "a91d7fa448b26a042a3ccc9511a45d9e9d122c6312309a7790cbfe7afbeb4451"
    },
    "published_pin": {
      "bytes": 2517,
      "sha256": "a91d7fa448b26a042a3ccc9511a45d9e9d122c6312309a7790cbfe7afbeb4451"
    },
    "transform": "redact"
  }
}

SOURCE_TEXT = '$PRIVATE/outputs/editor-mixed-readers-prepare/v1'
REPO_TEXT = '$PRIVATE/work/Protracker-2.4A'

def require(ok, message):
    if not ok:
        raise RuntimeError(message)

def pin_bytes(data):
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}

def digest(record):
    return {key: record[key] for key in ['bytes', 'sha256']}

def safe_name(name):
    p = PurePosixPath(name)
    require(isinstance(name, str) and bool(p.parts) and not p.is_absolute() and
            '..' not in p.parts and p.as_posix() == name, 'Unsafe relative packet name')

def call_labels(name):
    passed = name in PASSES
    return (PREPARATION + (VERIFICATION if passed else []),
            ['controller-compile'] + (['controller-run'] if passed else []))

def log_names(name):
    prep, product = call_labels(name)
    return [name + '/' + label + '.' + stream for label in prep + product
            for stream in ['stdout', 'stderr'] if label in SAFE_LOGS or stream == 'stderr']

def binding(name):
    return COPY_SPECS[name]['original']

def expected_ledger(records):
    return [{'attempt': name, 'status': records[name + '/run.json']['status'],
             'current': name == CURRENT, 'historical_pass': name == PASSES[0],
             'manifest_original': binding(name + '/run.json'),
             'manifest_published': COPY_SPECS[name + '/run.json']['published_pin'],
             'published_logs': log_names(name)} for name in ATTEMPTS]

def call_schema(run, name, units):
    passed = name in PASSES
    prep, product = call_labels(name)
    require([c['label'] for c in run['preparation_calls']] == prep and
            [c['label'] for c in run['calls']] == product, 'Exact call-label schema')
    require(run['head'] == BASELINE and run['source_file_count'] == 937 and
            len(run['inputs']) == 937 and run['ordered_units'] == units and
            len(units) == 78, 'Baseline/source/unit schema')
    require(len(run['overlays']) == (6 if name == FAILURES[0] else 9), 'Overlay count')
    for name_in_source in [*run['inputs'], *run['overlays'], *units]:
        safe_name(name_in_source)
    if passed:
        require((run['status'], run['phase'], run['first_failure']) == ('PASS', 'COMPLETE', None),
                'Pass not complete')
    else:
        require(run['status'] == 'FAIL' and run['phase'] == 'COMPILE' and
                run['first_failure']['label'] == 'controller-compile', 'Actual first compile failure')
    folder = SOURCE_TEXT + '/' + name
    require(run['attempt'] == folder, 'Manifest attempt path')
    flags = ['-std=c99', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-UNDEBUG',
             '-fsanitize=address,undefined', '-Isrc/core', '-I.']
    git_args = {'prepare-head': ['rev-parse', 'HEAD'], 'verify-head': ['rev-parse', 'HEAD'],
                'prepare-index': ['ls-files', '--stage'], 'verify-index': ['ls-files', '--stage'],
                'prepare-archive': ['archive', BASELINE, 'src', 'tests'],
                'prepare-font': ['show', BASELINE + ':vendor/pt23f/raw/ptfont.raw']}
    for call in run['preparation_calls'] + run['calls']:
        label = call['label']
        require(call['driver_reaped'] is True and call['group_quiescent'] is True and
                call['owned_new_session'] is True and call['pid'] == call['pgid'] and
                call['cleanup_reserve_seconds'] == 5, 'Owned call custody')
        for stream in ['stdout', 'stderr']:
            require(call[stream] == folder + '/' + label + '.' + stream, 'Exact log path')
            key = name + '/' + label + '.' + stream
            if key in COPY_SPECS:
                require(binding(key) == digest(call[stream + '_snapshot']), 'Original raw-log binding')
            else:
                require(stream == 'stdout' and label in ['prepare-archive', 'prepare-font'],
                        'Unexpected omitted log')
        if label in git_args:
            argv, cwd, limit = ['git', *git_args[label]], REPO_TEXT, 30
        elif label == 'controller-compile':
            argv, cwd, limit = ['cc', *flags, *units, '-o', folder + '/controller'], folder + '/source', 120
        else:
            argv, cwd, limit = [folder + '/controller'], folder + '/source', 180
        require(call['argv'] == argv and call['cwd'] == cwd and call['whole_call_seconds'] == limit and
                0 <= call['elapsed_seconds'] < limit, 'Exact argv/cwd/bounds')
        failed = name in FAILURES and label == 'controller-compile'
        require(call['status'] == ('FAIL' if failed else 'PASS') and
                call['returncode'] == (1 if failed else 0), 'Call status/returncode')

def validate_records(records):
    current_review = records['reviews/current-v5-review.json']
    current_verify = records['reviews/current-v5-verification.json']
    old_review = records['reviews/historical-v4-review.json']
    old_verify = records['reviews/historical-v4-verification.json']
    for attempt, review, verify, review_name, verify_name in [
        (CURRENT, current_review, current_verify, 'current-v5-review.json', 'current-v5-verification.json'),
        (PASSES[0], old_review, old_verify, 'historical-v4-review.json', 'historical-v4-verification.json')]:
        require(review['review_status'] == 'BOUNDED_PASS_SAVED_HOST_QUALIFICATION' and
                review['findings'] == [] and review['qualified_attempt'] == attempt, 'Independent result')
        require(digest(review['actual_run_manifest']) == digest(verify['run_manifest']) ==
                binding(attempt + '/run.json'), 'Independent original manifest provenance')
        require(review['verification_sha256'] == binding('reviews/' + verify_name)['sha256'],
                'Independent original verification hash provenance')
        if attempt == CURRENT:
            require(digest(review['verification']) == binding('reviews/' + verify_name),
                    'Current original verification byte provenance')
        else:
            require(review['verification'] == 'verification.json',
                    'Historical original review verification field')
        for rows in [review['prior_actual_failures'], verify['preserved_prior_actual_failures']]:
            require([r['attempt'] for r in rows] == FAILURES, 'First failure order')
            for row in rows:
                require(digest(row['run']) == binding(row['attempt'] + '/run.json'), 'First failure provenance')
    for record in [current_review, current_verify]:
        old = record['historical_v4_host']
        require(digest(old['run']) == binding(PASSES[0] + '/run.json') and
                digest(old['review']) == binding('reviews/historical-v4-review.json') and
                digest(old['verification']) == binding('reviews/historical-v4-verification.json'),
                'Historical pass independent provenance')
    units = current_verify['ordered_units']
    require(current_verify['ordered_unit_count'] == 78 and len(units) == 78 and
            current_verify['source_file_count'] == 937 and len(current_verify['overlays']) == 9 and
            len(current_verify['protected16']) == 16 and current_verify['empty_staged_diff'] is True,
            'Current reviewed source/protected controls')
    for name in ATTEMPTS:
        call_schema(records[name + '/run.json'], name, units)
    for version in range(1, 6):
        draft = records['source-history/draft-source-v%d.json' % version]
        require(draft['baseline'] == BASELINE and
                draft['status'] == 'DRAFT_SOURCE_ONLY_NOT_COMPILED_NOT_RUN', 'Draft history changed')
    custody = records['reviews/root-host-pass-custody-v2.json']
    require(custody['status'] == 'ROOT_SAVED_V5_HOST_CUSTODY_AND_INDEPENDENT_REVIEW_PASS' and
            digest(custody['manifest']) == binding(CURRENT + '/run.json') and
            digest(custody['independent_review']) == binding('reviews/current-v5-review.json') and
            digest(custody['independent_verification']) == binding('reviews/current-v5-verification.json') and
            digest(custody['source_freeze']) == binding('source-history/draft-source-v5.json') and
            custody['source_count'] == 937 and custody['overlay_count'] == 9 and
            custody['ordered_unit_count'] == 78 and custody['protected_count'] == 16 and
            custody['complete_software_lines'] == 7 and custody['head'] == BASELINE and
            custody['index_empty'] is True and custody['protected_full_modes_preserved'] is True,
            'Root custody independent provenance')
    actual_calls = records[CURRENT + '/run.json']['preparation_calls'] + records[CURRENT + '/run.json']['calls']
    require([c['label'] for c in custody['calls']] == [c['label'] for c in actual_calls], 'Custody eight labels')
    for saved, actual in zip(custody['calls'], actual_calls):
        require(saved['driver_reaped'] is True and saved['group_quiescent'] is True and
                saved['elapsed_seconds'] == actual['elapsed_seconds'] and
                digest(saved['stdout']) == digest(actual['stdout_snapshot']) and
                digest(saved['stderr']) == digest(actual['stderr_snapshot']), 'Root raw-call provenance')
    return units

def read(path):
    require(stat.S_ISREG(path.lstat().st_mode), 'Nonordinary packet file')
    data = path.read_bytes()
    private_prefix = b'/' + b'Users/' + b'james1/'
    require(private_prefix not in data, 'Unredacted private path')
    return data

def main():
    root = Path(__file__).resolve().parent
    files = json.loads(read(root / 'files.json'))
    require(set(files) == set(COPY_SPECS) | GENERATED and len(files) == INVENTORY_COUNT,
            'Exact planned inventory')
    actual = set()
    for path in root.rglob('*'):
        mode = path.lstat().st_mode
        require(stat.S_ISREG(mode) or stat.S_ISDIR(mode), 'Nonordinary packet entry')
        if stat.S_ISREG(mode):
            actual.add(path.relative_to(root).as_posix())
    require(actual == set(files) | {'files.json'} and len(actual) == TOTAL_COUNT,
            'Unexpected/missing packet entries')
    for name, record in files.items():
        safe_name(name)
        require(pin_bytes(read(root / name)) == record, 'File inventory changed')
    rows = json.loads(read(root / 'original-bindings.json'))
    require(len(rows) == COPIED_COUNT and len({r['published'] for r in rows}) == COPIED_COUNT and
            {r['published'] for r in rows} == set(COPY_SPECS), 'Exact binding set')
    for row in rows:
        spec = COPY_SPECS[row['published']]
        require(row['original'] == spec['original'] and row['published_pin'] == spec['published_pin'] ==
                files[row['published']] and row['transform'] == spec['transform'],
                'Original/published/projection binding')
    records = {}
    for name, spec in COPY_SPECS.items():
        data = read(root / name)
        require(pin_bytes(data) == spec['published_pin'], 'Immutable sanitized evidence changed')
        if name.endswith('.json'):
            records[name] = json.loads(data)
    require('scope' not in records['reviews/root-host-pass-custody-v2.json'], 'Private custody scope present')
    units = validate_records(records)
    ledger = json.loads(read(root / 'attempts.json'))
    require(ledger == expected_ledger(records) and [r['current'] for r in ledger] ==
            [False, False, False, True] and all(type(r['current']) is bool for r in ledger),
            'Exact four-attempt/current history')
    for name in PASSES:
        data = read(root / name / 'controller-run.stdout')
        require(pin_bytes(data) == STDOUT_PIN and len(data.splitlines()) == 7, 'Full seven pass lines')
        verify_name = 'current-v5-verification.json' if name == CURRENT else 'historical-v4-verification.json'
        require(data.decode().splitlines() == records['reviews/' + verify_name]['full_stdout_markers'],
                'Independent complete marker provenance')
        for call in records[name + '/run.json']['preparation_calls'] + records[name + '/run.json']['calls']:
            require(read(root / name / (call['label'] + '.stderr')) == b'', 'Passed call stderr')
    require(pin_bytes(read(root / 'README.md')) == README_PIN, 'README changed')
    print(json.dumps({'status': 'PASS_SAVED_HOST_PACKET_V2_ONLY', 'bound_files': len(files),
                      'total_files': TOTAL_COUNT, 'attempts': 4, 'current_sources': 937,
                      'units': len(units), 'complete_lines': 7}))

if __name__ == '__main__':
    main()
