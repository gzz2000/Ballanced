"""Filesystem regression for the actual standalone migration executable."""
import hashlib
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

HELPER = str(Path(sys.argv.pop(1)).resolve())

class MigrationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="Ballance 路径 ")
        self.root = Path(self.temp.name) / "Application Support" / "Ballance"
        self.root.mkdir(parents=True)
        self.addCleanup(self.temp.cleanup)

    def put(self, name, data=b"legacy"):
        p = self.root / name
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_bytes(data)
        return p

    def run_helper(self):
        subprocess.run([HELPER, str(self.root)], check=True, capture_output=True)

    def files(self):
        return {str(p.relative_to(self.root)): hashlib.sha256(p.read_bytes()).hexdigest()
                for p in self.root.rglob("*") if p.is_file() and not p.is_symlink()}

    def test_screenshot_layout_and_nested_tas(self):
        for name in [r"ModLoader\Configs\BML.cfg", r"ModLoader\ImGui.ini",
                     r"ModLoader\palette.ini", r"ModLoader\Themes\nord.ini",
                     r"ModLoader\Themes\one-dark.ini"]:
            self.put(name, name.encode())
        (self.root / r"ModLoader\Themes").mkdir()
        self.put("..\\ModLoader\\TAS\\/竞速/录制.tas", b"record")
        self.put(r"ModLoader/Maps\地图.nmo", b"map")
        self.put("Database.tdb", b"highscores")
        self.run_helper()
        self.assertFalse(any("\\" in p.name for p in self.root.rglob("*")))
        self.assertEqual((self.root / "ModLoader/Configs/BML.cfg").read_bytes(), b"ModLoader\\Configs\\BML.cfg")
        self.assertEqual((self.root / "ModLoader/TAS/竞速/录制.tas").read_bytes(), b"record")
        self.assertEqual((self.root / "ModLoader/Maps/地图.nmo").read_bytes(), b"map")
        self.assertEqual((self.root / "Database.tdb").read_bytes(), b"highscores")
        first = self.files()
        self.run_helper()
        self.assertEqual(first, self.files())  # No duplicate backups or log churn.

    def test_conflicting_settings_preserved(self):
        self.put("ModLoader/Configs/BML.cfg", b"native fps=60")
        self.put(r"ModLoader\Configs\BML.cfg", b"legacy fps=132")
        self.run_helper()
        self.assertEqual((self.root / "ModLoader/Configs/BML.cfg").read_bytes(), b"native fps=60")
        self.assertEqual((self.root / "LegacyPathConflicts/ModLoader/Configs/BML.cfg").read_bytes(), b"legacy fps=132")
        self.put(r"ModLoader\Configs\BML.cfg", b"another legacy")
        self.run_helper()
        self.assertEqual((self.root / "LegacyPathConflicts/ModLoader/Configs/BML.cfg.legacy-1").read_bytes(), b"another legacy")

    def test_equal_copies_and_directory_merge(self):
        self.put("ModLoader/Themes/nord.ini", b"same")
        self.put(r"ModLoader\Themes\nord.ini", b"same")
        self.put(r"ModLoader\Themes/one-dark.ini", b"dark")
        self.put("ModLoader/Themes/custom.ini", b"custom")
        self.run_helper()
        self.assertFalse((self.root / "LegacyPathConflicts").exists())
        self.assertEqual((self.root / "ModLoader/Themes/one-dark.ini").read_bytes(), b"dark")
        self.assertEqual((self.root / "ModLoader/Themes/custom.ini").read_bytes(), b"custom")

    def test_type_collision_preserves_native_and_legacy(self):
        self.put("ModLoader/Themes", b"native-file")
        self.put(r"ModLoader\Themes\nord.ini", b"theme")
        self.run_helper()
        self.assertEqual((self.root / "ModLoader/Themes").read_bytes(), b"native-file")
        self.assertEqual((self.root / "LegacyPathConflicts/ModLoader/Themes/nord.ini").read_bytes(), b"theme")

    def test_symlinks_not_followed_and_parent_traversal_not_interpreted(self):
        outside = Path(self.temp.name) / "outside"
        outside.mkdir()
        (outside / "keep").write_text("untouched")
        (self.root / "ModLoader").symlink_to(outside, target_is_directory=True)
        self.put(r"ModLoader\Configs\BML.cfg", b"config")
        self.put(r"..\ModLoader\TAS\..\escape", b"not migrated")
        self.put(r"unrelated\name", b"not loader data")
        self.run_helper()
        self.assertEqual(sorted(p.name for p in outside.iterdir()), ["keep"])
        self.assertEqual((self.root / "LegacyPathConflicts/ModLoader/Configs/BML.cfg").read_bytes(), b"config")
        self.assertTrue((self.root / r"..\ModLoader\TAS\..\escape").is_file())
        self.assertTrue((self.root / r"unrelated\name").is_file())

    def test_source_symlink_preserved_as_backup(self):
        target = self.put("target.txt", b"unchanged")
        (self.root / r"ModLoader\link").symlink_to(target)
        self.run_helper()
        backup = self.root / "LegacyPathConflicts/ModLoader/link"
        self.assertTrue(backup.is_symlink())
        self.assertEqual(target.read_bytes(), b"unchanged")

    def test_concurrent_launches_and_missing_root(self):
        self.put(r"ModLoader\Themes\主题.ini", b"theme")
        processes = [subprocess.Popen([HELPER, str(self.root)]) for _ in range(4)]
        for p in processes:
            self.assertEqual(p.wait(), 0)
        self.assertEqual((self.root / "ModLoader/Themes/主题.ini").read_bytes(), b"theme")
        new_root = self.root / "new data"
        subprocess.run([HELPER, str(new_root)], check=True)
        self.assertTrue(new_root.is_dir())

if __name__ == "__main__":
    unittest.main()
