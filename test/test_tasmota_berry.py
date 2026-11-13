from pathlib import Path
import json
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
        with tempfile.TemporaryDirectory(prefix='lighthouse-berry-') as temporary:
            folder = Path(temporary)
            (folder/'mdns.be').write_text('import global\nvar m=module("mdns")\n'
                'global.lh_mdns_calls=[]\nm.add_hostname=def (host,ip6,ip4) '
                'global.lh_mdns_calls.push([host,ip6,ip4]) end\nreturn m\n')
            program = bootstrap + '\nsys.path().push(' + json.dumps(temporary) + ')\n' + (ROOT/'test/tasmota_runtime.be').read_text()
            script = folder/'run.be'
            script.write_text(program)
            result = subprocess.run([str(executable), '-g', str(script)], cwd=directory,
                                    text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        self.assertEqual(result.returncode, 0, result.stdout)
        self.assertIn('Tasmota Berry passed:', result.stdout)
