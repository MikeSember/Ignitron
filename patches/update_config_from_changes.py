import os
import sys

# Add parent directory to path to import patch_pins
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from patch_pins import Change

CONFIG_FILE = "src/Config_Definitions.h"


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
                pin_dict[pin_name] = pin_value
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
                if pin_name in line and "=" in line:
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
