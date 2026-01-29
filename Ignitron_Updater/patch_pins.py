import os

file_name = "src/Config_Definitions.h"

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

options = ["ENABLE_FX_BLINK = true;"]


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
change_options = Change(presets=options)


class FileReader:
    def __init__(self, file_name=file_name):
        self.file = file_name
        self.lines = self.read_file()

    def read_file(self):
        with open(self.file, "r+") as f:
            lines = f.readlines()
        return lines

    def print_lines(self):
        for line in self.lines:
            print(line)


class LineUpdater(FileReader):
    def __init__(self, pins_dict, lines):
        self.pins_dict = pins_dict
        self.lines = lines
        self.updated_lines_numbers = []
        self.updated_lines = self.update_lines()

    def update_lines(self):
        lines = self.lines
        new_lines = []
        for preset_name, preset_value in self.pins_dict.items():
            for i, line in enumerate(lines):
                if preset_name in line:
                    print(f"line {i}: {line}")
                    newline = self.update_line(line, preset_value)
                    new_lines += newline
                    self.updated_lines_numbers.append(i)
                    print(f"updated to: {newline}")
                else:
                    new_lines += f"{line}"
        return new_lines

    def update_line(self, line, preset_value):
        line_return = ord("\n")
        newline_key = line.rstrip("= ")
        newline_value = f"= {preset_value}{line_return}"
        newline = f"{newline_key}{newline_value}"
        return newline


class FileWriter:
    def __init__(self, file_name=file_name, lines=[]):
        self.file = file_name
        self.lines = lines

    def save_copy(self):
        backup_string = ".self_bak"
        backup_filename = self.file + backup_string
        if os.path.exists(backup_filename):
            os.remove(backup_filename)
        os.rename(self.file, backup_filename)
        print(f"File {self.file} copied successfully to {backup_filename}.")

    def save_file(self, lines=None):
        lines = lines if lines is not None else self.lines
        self.save_copy()
        os.remove(self.file)
        with open(self.file, "w") as f:
            f.writelines(lines)
        print(f"File {self.file} updated successfully.")


class FileUpdater(LineUpdater, FileWriter):
    def __init__(self, file_name):
        self.file_name = file_name
        self.pins_dict = Change.all_changes
        self.reader = FileReader(file_name)
        self.lines = self.reader.lines
        self.writer = FileWriter(file_name, self.lines)
        self.updated_lines = self.update_lines()

    def print_updated_lines(self):
        if self.updated_lines is not None:
            for line in self.updated_lines:
                print(line)
            else:
                print("No updated lines to print.")


def update_file(file_name):
    file_updater = FileUpdater(file_name)
    file_updater.save_copy()
    file_updater.update_lines()
    file_updater.save_file()


def main():
    update_file(file_name)


if __name__ == "__main__":
    main()
