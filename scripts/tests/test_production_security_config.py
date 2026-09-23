import importlib.util
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "production_security",
    ROOT / "scripts/verify_production_security_config.py",
)
production_security = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(production_security)


class ProductionSecurityConfigTests(unittest.TestCase):
    def valid_values(self):
        values = {key: "y" for key in production_security.REQUIRED_ENABLED}
        values.update(
            {key: "n" for key in production_security.REQUIRED_DISABLED}
        )
        values.update(production_security.REQUIRED_VALUES)
        values["CONFIG_SECURE_BOOT_SIGNING_KEY"] = '"/tmp/nara-production.pem"'
        return values

    def test_accepts_required_production_security_state(self):
        self.assertEqual(
            production_security.verify(self.valid_values()),
            [],
        )

    def test_rejects_development_flash_encryption_and_jtag(self):
        values = self.valid_values()
        values["CONFIG_SECURE_FLASH_ENCRYPTION_MODE_DEVELOPMENT"] = "y"
        values["CONFIG_SECURE_BOOT_ALLOW_JTAG"] = "y"

        errors = production_security.verify(values)
        self.assertIn(
            "CONFIG_SECURE_FLASH_ENCRYPTION_MODE_DEVELOPMENT must be disabled",
            errors,
        )
        self.assertIn("CONFIG_SECURE_BOOT_ALLOW_JTAG must be disabled", errors)

    def test_parses_not_set_lines(self):
        parsed = production_security.parse_sdkconfig(
            "# CONFIG_SECURE_BOOT_ALLOW_JTAG is not set\n"
            "CONFIG_SECURE_BOOT=y\n"
        )
        self.assertEqual(parsed["CONFIG_SECURE_BOOT_ALLOW_JTAG"], "n")
        self.assertEqual(parsed["CONFIG_SECURE_BOOT"], "y")


if __name__ == "__main__":
    unittest.main()
