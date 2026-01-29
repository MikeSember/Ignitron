import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

CONFIG_FILE = "src/Config_Definitions.h"

preset_lines = [
    "BUTTON_PRESET1_GPIO = 23,",
    "BUTTON_DRIVE_GPIO = 23,",
    "BUTTON_PRESET2_GPIO = 39,",
    "BUTTON_MOD_GPIO = 39,",
    "BUTTON_PRESET3_GPIO = 34,",
    "BUTTON_DELAY_GPIO = 34,",
    "BUTTON_PRESET4_GPIO = 35,",
    "BUTTON_REVERB_GPIO = 35,",
    "BUTTON_BANK_DOWN_GPIO = 32,",
    "BUTTON_NOISEGATE_GPIO = 32,",
    "BUTTON_BANK_UP_GPIO = 33,",
    "BUTTON_COMP_GPIO = 33",
]

ledptionalgpio = [
    "OPTIONAL_GPIO_1 = 0,",
    "OPTIONAL_GPIO_2 = 4,",
    "OPTIONAL_GPIO_3 = 12,",
    "OPTIONAL_GPIO_4 = 15",
]

ledgpio = [
    "LED_DRIVE_GPIO = 13,",
    "LED_MOD_GPIO = 12,",
    "LED_DELAY_GPIO = 14,",
    "LED_REVERB_GPIO = 27,",
    "LED_NOISEGATE_GPIO = 5,",
    "LED_COMP_GPIO = 18,",
    "LED_BANK_DOWN_GPIO = 26,",
    "LED_BANK_UP_GPIO = 25,",
    "LED_PRESET1_GPIO = 2",
    "LED_PRESET2_GPIO = 4",
    "LED_PRESET3_GPIO = 16",
    "LED_PRESET4_GPIO = 17",
]

options = ["const bool ENABLE_FX_BLINK = true;"]


class Change:
    all_changes = []

    def __init__(self, presets):
        self.presets = presets

        Change.all_changes.append(self.presets)

    def create_pin_change_dict(self):
        all_changes = Change.all_changes
        self.preset_dict = {}
        for setting in all_changes:
            preset_name, preset_value = setting.split("=")
            self.preset_dict.update({preset_name.strip(): preset_value.strip()})
        return self.preset_dict


change_preset_lines = Change(presets=preset_lines)
change_led_optional_gpio = Change(presets=ledptionalgpio)
change_led_gpio = Change(presets=ledgpio)
# change_options = Change(presets=options)


class ConfigUpdater:
    def __init__(self, config_file=CONFIG_FILE):
        self.config_file = config_file
        self.changes = self._collect_all_changes()
        self.pin_dict = self._create_pin_dict()

    def _collect_all_changes(self):
        """Collect all changes from Change.all_changes"""
        all_changes = []
        for change_list in Change.all_changes:
            all_changes.extend(change_list)
        return all_changes

    def _create_pin_dict(self):
        """Create a dictionary of pin names and values from all changes"""
        pin_dict = {}
        for change in self.changes:
            if "=" in change:
                pin_name, pin_value = change.split("=", 1)
                pin_name = pin_name.strip()
                pin_value = pin_value.strip().rstrip(",;")
                pin_dict[pin_name] = str(pin_value)
        return pin_dict

    def read_config(self):
        """Read the configuration file"""
        with open(self.config_file, "r") as f:
            lines = f.readlines()
        return lines

    def update_config(self, lines):
        """Update configuration lines with new pin values"""
        updated_lines = []
        for line in lines:
            updated_line = line
            for pin_name, pin_value in self.pin_dict.items():
                # Check if this line contains the pin definition
                if f"{pin_name} =" in line:
                    # Extract the line's indentation and structure
                    indent = len(line) - len(line.lstrip())
                    # Check if line ends with comma or semicolon
                    has_comma = line.rstrip().endswith(",")
                    has_semicolon = line.rstrip().endswith(";")

                    # Reconstruct the line with new value
                    prefix = " " * indent + pin_name + " = "
                    suffix = "," if has_comma else (";" if has_semicolon else "")
                    updated_line = prefix + pin_value + suffix + "\n"
                    break  # Only update matching pin once per line
            updated_lines.append(updated_line)
        return updated_lines

    def backup_config(self):
        """Create a backup of the configuration file"""
        backup_file = self.config_file + ".backup"
        if os.path.exists(self.config_file):
            if os.path.exists(backup_file):
                os.remove(backup_file)
            with open(self.config_file, "r") as f:
                content = f.read()
            with open(backup_file, "w") as f:
                f.write(content)
            print(f"Backup created: {backup_file}")

    def save_config(self, lines):
        """Save the updated configuration"""
        with open(self.config_file, "w") as f:
            f.writelines(lines)
        print(f"Configuration updated: {self.config_file}")

    def print_changes(self):
        """Print the changes that will be made"""
        print("\nPin changes to be applied:")
        print("-" * 50)
        for pin_name, pin_value in self.pin_dict.items():
            print(f"  {pin_name} = {pin_value}")
        print("-" * 50)

    def apply_updates(self, dry_run=False):
        """Apply all updates to the configuration file"""
        self.print_changes()

        # Read current config
        lines = self.read_config()
        print(f"\nRead {len(lines)} lines from {self.config_file}")

        # Update lines
        updated_lines = self.update_config(lines)

        # Check for changes
        changes_made = sum(
            1 for orig, updated in zip(lines, updated_lines) if orig != updated
        )
        print(f"Lines to be updated: {changes_made}")

        if not dry_run:
            # Backup original
            self.backup_config()
            # Save updated config
            self.save_config(updated_lines)
            print("Update complete!")
        else:
            print("DRY RUN - No changes written")

        return changes_made


def main():
    """Main entry point"""
    import argparse

    parser = argparse.ArgumentParser(
        description="Update Config_Definitions.h with changes from patch_pins.py"
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show what would be changed without making changes",
    )
    parser.add_argument(
        "--config",
        default=CONFIG_FILE,
        help=f"Path to Config_Definitions.h (default: {CONFIG_FILE})",
    )

    args = parser.parse_args()

    updater = ConfigUpdater(args.config)
    updater.apply_updates(dry_run=args.dry_run)


if __name__ == "__main__":
    main()
