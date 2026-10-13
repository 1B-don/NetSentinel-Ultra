#!/usr/bin/env python3
"""Build a separate gated FAP using the owner's permitted Momentum SDK.
No SDK update, firmware operation, hardware launch or network operation is requested.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SDK_HOME = Path.home()/".ufbt-netsentinel-momentum"
UFBT = Path.home()/"flipper-build-env/bin/ufbt"
VARIANTS = {
    "gui": ("netsentinel", "NetSentinel Ultra", "NS_GUI_STORAGE_ONLY"),
    "info": ("netsentinel_esp_info", "NetSentinel ESP Info", "NS_ESP_INFO_ONLY"),
    "diagnostics": ("netsentinel_esp_diagnostics", "ESP Diagnostics v0.2", "NS_ESP_INFO_ONLY"),
    "esp-status": ("netsentinel_esp_status", "NetSentinel ESP Status", "NS_ESP_INFO_ONLY"),
    "development": ("netsentinel_development", "NetSentinel Ultra Dev", "NS_ESP_INFO_ONLY"),
    "lab-sim": ("netsentinel_lab_sim", "NetSentinel Ultra Lab Sim", "NS_ESP_INFO_ONLY"),
    "rc": ("netsentinel_ultra_v1_0_rc1", "NetSentinel Ultra v1.0 RC1", "NS_ESP_INFO_ONLY"),
}

def variant_manifest(manifest, variant):
    """Pure generation; keep root baseline and all variant gates explicit."""
    appid, title, gate = VARIANTS[variant]
    replacements = {
        'appid="netsentinel"': f'appid="{appid}"',
        'name="NetSentinel Ultra"': f'name="{title}"',
        'cdefines=["NS_GUI_STORAGE_ONLY"]': f'cdefines=["{gate}"]',
    }
    if 'fap_icon="assets/netsentinel_icon.png"' not in manifest:
        raise RuntimeError("Bundled manifest icon missing; no build attempted")
    for expected, replacement in replacements.items():
        if manifest.count(expected) != 1:
            raise RuntimeError("Default manifest contract changed; inspect before building")
        manifest = manifest.replace(expected, replacement)
    return manifest

def source_paths():
    paths = [ROOT/n for n in ("ap_parser.c", "ap_parser.h", "ns_analysis.c", "ns_analysis.h",
                              "assets/netsentinel_icon.png")]
    for folder in ("src/core", "src/flipper", "include/netsentinel"):
        paths += sorted(p for p in (ROOT/folder).glob("*") if p.suffix in (".c", ".h"))
    return paths

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--variant", choices=tuple(VARIANTS), help="explicit build variant; lab-sim contains offline synthetic workflows and retains the ESP information-only hardware gate")
    parser.add_argument("--development", action="store_true", help="build the unified v1 development app with pending radios disabled")
    args = parser.parse_args()
    if args.development and args.variant:
        parser.error("use either --development or --variant")
    variant = "development" if args.development else (args.variant or "diagnostics")
    appid, _, _ = VARIANTS[variant]
    manifest = variant_manifest((ROOT/"application.fam").read_text(), variant)
    if SDK_HOME.is_symlink() or (SDK_HOME/"current").is_symlink():
        raise RuntimeError("SDK home must be the reviewed independent cache")
    archive = ROOT/"builds/sdk-input/flipper-z-f7-sdk-mntm-dev-d3f89dfe.zip"
    if digest(archive) != "238db260ee7e8f6e9a79dd1c760e07a832351a7d92d896b4cb19082f8548b2c1":
        raise RuntimeError("Unexpected permitted SDK archive checksum")
    with zipfile.ZipFile(archive) as z:
        for name in ("components.json", "sdk_headers/sdk.opts", "sdk_headers/f7_sdk/targets/f7/api_symbols.csv"):
            if (SDK_HOME/"current"/name).read_bytes() != z.read(name):
                raise RuntimeError("Installed SDK build metadata differs from supplied archive")
    meta = json.loads((SDK_HOME/"current/components.json").read_text())["meta"]
    if meta.get("version") != "mntm-dev-d3f89dfe" or meta.get("hw_target") != "f7":
        raise RuntimeError("Unexpected SDK target/revision; no build attempted")
    table = (SDK_HOME/"current/sdk_headers/f7_sdk/targets/f7/api_symbols.csv").read_text()
    if "Version,+,87.1,," not in table.splitlines():
        raise RuntimeError("Unexpected SDK API; no build attempted")
    stage = ROOT/"builds/esp-info-app"
    if stage.is_symlink():
        raise RuntimeError("Build stage must not be a symlink")
    stage.mkdir(parents=True, exist_ok=True)
    if not stage.resolve().is_relative_to(ROOT/"builds"):
        raise RuntimeError("Build stage escaped authorized project directory")
    paths = source_paths()
    for src in paths:
        if src.is_symlink() or not src.resolve().is_relative_to(ROOT):
            raise RuntimeError("Source symlink is not permitted")
        dst = stage/src.relative_to(ROOT)
        dst.parent.mkdir(parents=True, exist_ok=True)
        if dst.is_symlink() or not dst.parent.resolve().is_relative_to(stage.resolve()):
            raise RuntimeError("Staged source symlink is not permitted")
        shutil.copy2(src, dst)
    if (stage/"application.fam").is_symlink():
        raise RuntimeError("Staged manifest must not be a symlink")
    (stage/"application.fam").write_text(manifest)
    stable = [p for p in (ROOT/"dist").glob("*.fap") if p.name != appid+".fap"]
    before = {path: digest(path) for path in stable}
    env = dict(os.environ, UFBT_HOME=str(SDK_HOME), FBT_TOOLCHAIN_PATH=str(Path.home()/".ufbt"))
    logs = ROOT/"logs"; logs.mkdir(exist_ok=True)
    for name, args in (("live-info-clean.log", ["-c"]), ("live-info-build.log", [])):
        with (logs/name).open("w") as log:
            subprocess.run([str(UFBT), *args], cwd=stage, env=env,
                stdout=log, stderr=subprocess.STDOUT, check=True)
    if any(digest(path) != expected for path, expected in before.items()):
        raise RuntimeError("Preserved stable FAP unexpectedly changed")
    src = stage/"dist"/(appid+".fap")
    dst = ROOT/"dist"/(appid+".fap")
    if dst.exists():
        preserved = ROOT/"builds/preserved-artifacts"/digest(dst)
        preserved.mkdir(parents=True, exist_ok=True)
        shutil.copy2(dst, preserved/dst.name)
    shutil.copy2(src, dst)
    print("Built separate gated FAP:", dst.name)
    print("Bytes:", dst.stat().st_size, "SHA256:", digest(dst))
    print("Stable FAPs unchanged; no device operation performed")

if __name__ == "__main__":
    main()
