#!/usr/bin/env python3
"""Black-box test: the executable is built once, then only JSON files change."""
import hashlib
import json
import pathlib
import shutil
import subprocess
import sys
import tempfile

exe = pathlib.Path(sys.argv[1]).resolve()
fixtures = pathlib.Path(sys.argv[2]).resolve()
count = 0

def check(condition, detail):
    global count
    count += 1
    if not condition:
        raise AssertionError(detail)

def read(p):
    return json.loads(p.read_text(encoding="utf-8-sig"))

def write(p, value):
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")

with tempfile.TemporaryDirectory(prefix="Starfall test żółć ") as temp:
    root = pathlib.Path(temp)
    app = root / "portable app"
    app.mkdir()
    local_exe = app / exe.name
    shutil.copy2(exe, local_exe)
    # Windows shared runtime dependencies, if present in build output.
    for dll in exe.parent.glob("*.dll"):
        shutil.copy2(dll, app / dll.name)
    content = app / "content"
    shutil.copytree(fixtures, content)
    original_hash = hashlib.sha256(local_exe.read_bytes()).hexdigest()
    cwd = root / "unrelated working directory"
    cwd.mkdir()
    def run(*args, ok=True, contains=""):
        result = subprocess.run([str(local_exe), *args], cwd=cwd, capture_output=True, text=True,
                                encoding="utf-8", errors="replace", timeout=30)
        output = result.stdout + result.stderr
        check((result.returncode == 0) == ok, f"exit {result.returncode}: {output}")
        check(contains in output, f"missing {contains!r}: {output}")
        return output
    run("--validate-content", contains="content validation: OK")
    run("--simulate-campaign", "--campaign", "base:classic", contains="VICTORY sectors=1")
    run("--simulate-campaign", "--campaign", "example:two_sectors", contains="VICTORY sectors=2")
    run("--simulate-campaign", "--sector", "example:relay", contains="VICTORY sectors=1")
    # Decoy in CWD must not be read.
    (cwd / "content").mkdir()
    (cwd / "content" / "game.json").write_text("BROKEN", encoding="utf-8")
    run("--validate-content", contains="content validation: OK")
    # Acceptance proof: clone sector, change ID/steps, append to route. Same binary.
    sector = read(content / "example/sectors/relay.json")
    sector["id"] = "example:my_sector"
    sector["name"] = "MY SECTOR"
    sector["steps"] = [{"type":"wave","ref":"base:patrol","complete_when":"enemies_cleared"}]
    write(content / "example/sectors/my_sector.json", sector)
    campaign_path = content / "example/campaigns/two_sectors.json"
    campaign = read(campaign_path)
    campaign["sectors"].append("example:my_sector")
    write(campaign_path, campaign)
    out = run("--simulate-campaign", "--campaign", "example:two_sectors", contains="VICTORY sectors=3")
    check("ENTER example:my_sector" in out, "cloned sector was not entered")
    check(hashlib.sha256(local_exe.read_bytes()).hexdigest() == original_hash, "binary changed")
    print("PASS: SAME BINARY, JSON-ONLY EXPANSION 2 -> 3 SECTORS; SHA256=" + original_hash)
    # Decimal boundary values and equal firing bounds must not fail due to float rounding.
    boundary_path = content / "base/waves/patrol.json"
    boundary_original = boundary_path.read_bytes()
    boundary = read(boundary_path)
    for minimum in (.1, 1.85):
        boundary.update(dive_interval=.1, fire_interval_min=minimum, fire_interval_max=minimum)
        write(boundary_path, boundary)
        run("--validate-content", contains="content validation: OK")
    boundary_path.write_bytes(boundary_original)
    # Every invalid fixture must fail before gameplay, and name the offending path/field.
    def bad(relative, mutate, fragment):
        path = content / relative
        original = path.read_bytes()
        try:
            obj = read(path)
            mutate(obj)
            write(path, obj)
            run("--validate-content", ok=False, contains=fragment)
        finally:
            path.write_bytes(original)
    bad("base/enemies/hornet.json", lambda j: j.update(hp=-1), "/hp")
    bad("base/enemies/hornet.json", lambda j: j.update(hp=3.5), "integer")
    bad("base/enemies/hornet.json", lambda j: j.update(hp="22"), "expected number")
    bad("base/enemies/hornet.json", lambda j: j.update(hp=True), "expected number")
    bad("base/enemies/hornet.json", lambda j: j.update(hpp=22), "unknown field")
    bad("base/enemies/hornet.json", lambda j: j.update(can_dive="yes"), "/can_dive")
    bad("base/enemies/hornet.json", lambda j: j.update(schema_version=99), "/schema_version")
    bad("base/enemies/hornet.json", lambda j: j.update(sprite="new_unimplemented"), "/sprite")
    bad("base/enemies/hornet.json", lambda j: j.update(name="Żółć"), "bitmap font")
    bad("base/enemies/hornet.json", lambda j: j.update(id="example:wrong_pack"), "namespace")
    bad("base/enemies/hornet.json", lambda j: j.pop("hp"), "required field")
    bad("base/waves/patrol.json", lambda j: j["spawns"][0].update(enemy="base:missing"), "/spawns/0/enemy")
    bad("base/waves/patrol.json", lambda j: j["spawns"][0].update(x=900), "/spawns/0/x")
    bad("base/waves/patrol.json", lambda j: j.update(spawns=[]), "/spawns")
    bad("base/waves/patrol.json", lambda j: j.update(spawns=j["spawns"]*20), "/spawns")
    bad("base/waves/patrol.json", lambda j: j.update(fire_interval_max=.1), "/fire_interval_max")
    bad("base/sectors/outer_rim.json", lambda j: j["steps"][0].update(ref="base:missing"), "/steps/0/ref")
    bad("base/sectors/outer_rim.json", lambda j: j["steps"][0].update(type="escort"), "/steps/0/type")
    bad("base/sectors/outer_rim.json", lambda j: j["steps"][0].update(complete_when="boss_defeated"), "/steps/0/complete_when")
    bad("base/sectors/outer_rim.json", lambda j: j["steps"][1].update(seconds=-1), "/steps/1/seconds")
    bad("base/sectors/outer_rim.json", lambda j: j.update(steps=[]), "/steps")
    bad("base/campaigns/classic.json", lambda j: j.update(sectors=["base:missing"]), "/sectors/0")
    bad("base/campaigns/classic.json", lambda j: j.update(sectors=[]), "/sectors")
    bad("base/bosses/null_crown.json", lambda j: j.update(behavior="unknown"), "/behavior")
    bad("base/bosses/null_crown.json", lambda j: j.update(hp=0), "/hp")
    bad("game.json", lambda j: j.update(packs=["../base"]), "invalid or duplicate")
    bad("game.json", lambda j: j.update(packs=["base","base"]), "invalid or duplicate")
    bad("game.json", lambda j: j.update(default_campaign="base:missing"), "/default_campaign")
    duplicate = content / "base/enemies/duplicate.json"
    duplicate.write_bytes((content / "base/enemies/hornet.json").read_bytes())
    run("--validate-content", ok=False, contains="duplicate ID")
    duplicate.unlink()
    broken = content / "base/enemies/broken.json"
    for raw, fragment in [(b'{"schema_version": 1,', "broken.json:"),
                          (b'{"id":"a","id":"b"}', "duplicate key"),
                          (b' '*(1024*1024+1), "size limit")]:
        broken.write_bytes(raw)
        run("--validate-content", ok=False, contains=fragment)
    broken.unlink()
    # Explicit missing root must fail, not silently fall back to a bundled campaign.
    run("--validate-content", "--content-root", str(root / "missing"), ok=False, contains="game.json")
    run("--validate-content", "--content-root", str(content), contains="content validation: OK")
    run("--campaign", "not_here:campaign", ok=False, contains="Unknown campaign")
    run("--sector", "not_here:sector", ok=False, contains="Unknown sector")
    check(hashlib.sha256(local_exe.read_bytes()).hexdigest() == original_hash, "binary modified by tests")
    print(f"PASS: {count} black-box checks (portable path, CLI, validation, JSON-only expansion)")
