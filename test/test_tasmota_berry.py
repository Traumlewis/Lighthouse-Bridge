from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
STAGE = ROOT / '.pio/tasmota-src'

class TasmotaBerryTests(unittest.TestCase):
    def test_real_pinned_berry_lighthouse_model(self):
        executable = STAGE / 'lib/libesp32/berry/berry'
        self.assertTrue(executable.is_file(), 'Build the Tasmota prototype before the complete test suite')
        directory = STAGE / 'lib/libesp32/berry_matter/tests'
        bootstrap = (directory / 'run_tests.be').read_text().split('class TestTasmota')[0]
        program = bootstrap + '\n' + (ROOT / 'test/tasmota_runtime.be').read_text()
        with tempfile.NamedTemporaryFile(mode='w', suffix='.be') as handle:
            handle.write(program)
            handle.flush()
            result = subprocess.run([str(executable), '-g', handle.name], cwd=directory,
                                    text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn('Tasmota Berry passed:', result.stdout)
