# PlatformIO pre-script: setzt FW_VERSION und GITHUB_REPO als Build-Flags.
#
# FW_VERSION:  Umgebungsvariable FW_VERSION (CI setzt sie aus dem Git-Tag),
#              sonst "git describe", sonst "0.0.0-dev".
# GITHUB_REPO: Umgebungsvariable GITHUB_REPOSITORY (in GitHub Actions gesetzt),
#              sonst aus "git remote get-url origin" abgeleitet.
import os
import re
import subprocess

Import("env")  # noqa: F821  (von PlatformIO bereitgestellt)


def git(*args):
    try:
        return subprocess.check_output(["git", *args], cwd=env["PROJECT_DIR"],  # noqa: F821
                                       stderr=subprocess.DEVNULL, text=True).strip()
    except Exception:
        return ""


version = os.environ.get("FW_VERSION") or git("describe", "--tags", "--always", "--dirty") or "0.0.0-dev"
version = version.lstrip("v")

repo = os.environ.get("GITHUB_REPOSITORY", "")
if not repo:
    m = re.search(r"github\.com[:/]([^/]+/[^/.]+)", git("remote", "get-url", "origin"))
    repo = m.group(1) if m else ""

print(f"[version.py] FW_VERSION={version} GITHUB_REPO={repo or '(keins – Internet-OTA aus)'}")
env.Append(CPPDEFINES=[  # noqa: F821
    ("FW_VERSION", env.StringifyMacro(version)),  # noqa: F821
    ("GITHUB_REPO", env.StringifyMacro(repo)),  # noqa: F821
])
