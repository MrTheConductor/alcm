from dataclasses import dataclass

import pytest

import flash


@dataclass
class FakeResult:
    returncode: int


class FakeRunner:
    """Records every command it was called with, returns a fixed exit code."""

    def __init__(self, returncode=0):
        self.returncode = returncode
        self.calls = []

    def __call__(self, cmd):
        self.calls.append(cmd)
        return FakeResult(self.returncode)


def test_check_prerequisites_raises_when_pyocd_missing(monkeypatch):
    monkeypatch.setattr(flash.shutil, "which", lambda name: None)
    with pytest.raises(flash.FlashError, match="pyocd"):
        flash.check_prerequisites()


def test_check_prerequisites_raises_when_pack_missing(monkeypatch, tmp_path):
    monkeypatch.setattr(flash.shutil, "which", lambda name: "/usr/bin/pyocd")
    monkeypatch.setattr(flash, "_tool_dir", lambda: str(tmp_path))
    with pytest.raises(flash.FlashError, match=flash.PACK_FILENAME):
        flash.check_prerequisites()


def test_check_prerequisites_raises_when_config_missing(monkeypatch, tmp_path):
    (tmp_path / flash.PACK_FILENAME).write_bytes(b"")
    monkeypatch.setattr(flash.shutil, "which", lambda name: "/usr/bin/pyocd")
    monkeypatch.setattr(flash, "_tool_dir", lambda: str(tmp_path))
    with pytest.raises(flash.FlashError, match=flash.CONFIG_FILENAME):
        flash.check_prerequisites()


def test_check_prerequisites_passes_when_everything_present(monkeypatch, tmp_path):
    (tmp_path / flash.PACK_FILENAME).write_bytes(b"")
    (tmp_path / flash.CONFIG_FILENAME).write_text("pack:\n")
    monkeypatch.setattr(flash.shutil, "which", lambda name: "/usr/bin/pyocd")
    monkeypatch.setattr(flash, "_tool_dir", lambda: str(tmp_path))
    flash.check_prerequisites()  # must not raise


def test_check_prerequisites_actual_repo_layout():
    # No mocking - confirms the real files this module ships alongside
    # (checked into the repo) actually exist, independent of whether pyocd
    # itself happens to be installed on the machine running the tests.
    import os
    assert os.path.isfile(os.path.join(flash._tool_dir(), flash.PACK_FILENAME))
    assert os.path.isfile(os.path.join(flash._tool_dir(), flash.CONFIG_FILENAME))


def test_erase_invokes_pyocd_with_target_and_config():
    runner = FakeRunner(returncode=0)
    flash.erase(runner=runner)

    assert len(runner.calls) == 1
    cmd = runner.calls[0]
    assert cmd[0] == "pyocd"
    assert cmd[1] == "erase"
    assert "-t" in cmd and cmd[cmd.index("-t") + 1] == flash.TARGET
    assert "--config" in cmd


def test_erase_raises_on_nonzero_exit():
    runner = FakeRunner(returncode=1)
    with pytest.raises(flash.FlashError):
        flash.erase(runner=runner)


def test_load_invokes_pyocd_with_file_target_and_config():
    runner = FakeRunner(returncode=0)
    flash.load("patched.hex", runner=runner)

    assert len(runner.calls) == 1
    cmd = runner.calls[0]
    assert cmd[0] == "pyocd"
    assert cmd[1] == "load"
    assert "patched.hex" in cmd
    assert "-t" in cmd and cmd[cmd.index("-t") + 1] == flash.TARGET


def test_load_raises_on_nonzero_exit():
    runner = FakeRunner(returncode=1)
    with pytest.raises(flash.FlashError):
        flash.load("patched.hex", runner=runner)
