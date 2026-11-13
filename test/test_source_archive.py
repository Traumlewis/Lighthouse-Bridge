from pathlib import Path
import sys
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from package_release import source_path_allowed
from build_matter import VERSION


class SourceArchiveTests(unittest.TestCase):
    def test_private_attachments_are_never_source(self):
        self.assertFalse(source_path_allowed(Path('.codex-attachments/session/Log.txt')))
        self.assertFalse(source_path_allowed(Path('nested/.codex-attachments/secret.txt')))
        self.assertFalse(source_path_allowed(Path('nested/.cache/compiler-object')))
        self.assertTrue(source_path_allowed(Path('src/main.cpp')))

    def test_matter_zip_has_every_local_header_and_build_requirements(self):
        with zipfile.ZipFile(ROOT / 'dist/matter' / f'lighthouse-bridge-{VERSION}.zip') as bundle:
            names = set(bundle.namelist())
            for header in (ROOT / 'experimental/matter/MatterBridge').glob('*.h'):
                self.assertIn('source/' + str(header.relative_to(ROOT)), names)
            self.assertIn('source/tools/requirements-matter-build.txt', names)

    def test_stable_zip_contains_no_private_attachments(self):
        with zipfile.ZipFile(ROOT / 'dist/lighthouse-bridge-v0.3.9.zip') as bundle:
            self.assertFalse(any('.codex-attachments' in Path(name).parts for name in bundle.namelist()))
