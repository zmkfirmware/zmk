#!/usr/bin/env python3
"""
ZMK TKL Matrix Validation Script

Validates:
1. Matrix CSV format and completeness
2. GPIO pin assignments for nRF52832
3. Duplicate key detection
4. Row/Column consistency
5. DTS file generation
"""

import csv
import re
import sys
from pathlib import Path
from typing import Dict, List, Set, Tuple

# nRF52832 QFAAB GPIO Pin Mapping (P0.00 - P0.31)
# Source: Nordic nRF52832 Product Specification
NRF52832_GPIO_PINS = {
    # Port 0 pins (0-31)
    0: "P0.00", 1: "P0.01", 2: "P0.02", 3: "P0.03", 4: "P0.04", 5: "P0.05", 6: "P0.06", 7: "P0.07",
    8: "P0.08", 9: "P0.09", 10: "P0.10", 11: "P0.11", 12: "P0.12", 13: "P0.13", 14: "P0.14", 15: "P0.15",
    16: "P0.16", 17: "P0.17", 18: "P0.18", 19: "P0.19", 20: "P0.20", 21: "P0.21", 22: "P0.22", 23: "P0.23",
    24: "P0.24", 25: "P0.25", 26: "P0.26", 27: "P0.27", 28: "P0.28", 29: "P0.29", 30: "P0.30", 31: "P0.31"
}

# Reserved pins for nRF52832 (cannot use for matrix)
RESERVED_PINS = {
    # Crystal/OSC
    0, 1,  # XL1, XL2 (32kHz crystal)
    # SWD Debug
    None,  # SWDIO, SWCLK determined by board, but typical pins
    # USB (if using USB peripheral)
    None,  # VBUS, D+, D- pins vary by package
}

# Recommended pin assignments for TKL
# For 8x14 matrix (pins 3-24 used), use remaining pins for peripherals
TKL_RECOMMENDED_GPIO = {
    # I2C for Azoteq Trackpad (use TWI0: P0.26, P0.27)
    "I2C_SDA": 26,  # P0.26
    "I2C_SCL": 27,  # P0.27

    # UART for debugging (optional, use pins outside matrix range)
    "UART_TX": 25,   # P0.25
    "UART_RX": 31,   # P0.31

    # Battery ADC
    "BATTERY_ADC": 29,  # P0.29 (AIN7 - ADC input)

    # Trackpad Interrupt
    "TRACKPAD_INT": 28,  # P0.28

    # VBUS sensing (USB power detection)
    "VBUS_SENSE": 2,  # P0.02

    # Status LED
    "STATUS_LED": 30,  # P0.30
}

class MatrixValidator:
    def __init__(self, csv_path: str):
        self.csv_path = Path(csv_path)
        self.keys: List[Dict] = []
        self.positions: Set[Tuple[int, int]] = set()
        self.used_pins: Set[int] = set()
        self.errors: List[str] = []
        self.warnings: List[str] = []

    def load_csv(self) -> bool:
        """Load and parse the matrix CSV file."""
        try:
            with open(self.csv_path, 'r') as f:
                reader = csv.DictReader(f, fieldnames=['row', 'col', 'key', 'position'])
                for line_num, row in enumerate(reader, 1):
                    # Skip comments
                    if row['row'].strip().startswith('#'):
                        continue

                    # Parse row and column
                    try:
                        row_idx = int(row['row'].strip())
                        col_idx = int(row['col'].strip())
                    except ValueError:
                        self.errors.append(f"Line {line_num}: Invalid row/column values: {row['row']}, {row['col']}")
                        continue

                    self.keys.append({
                        'row': row_idx,
                        'col': col_idx,
                        'key': row['key'].strip(),
                        'position': row.get('position', '').strip(),
                        'line': line_num
                    })

                    self.positions.add((row_idx, col_idx))

            return True
        except FileNotFoundError:
            self.errors.append(f"CSV file not found: {self.csv_path}")
            return False
        except Exception as e:
            self.errors.append(f"Error reading CSV: {e}")
            return False

    def validate_duplicates(self):
        """Check for duplicate positions."""
        seen = {}
        for key_data in self.keys:
            pos = (key_data['row'], key_data['col'])
            if pos in seen:
                self.errors.append(
                    f"Duplicate position ({pos[0]},{pos[1]}): "
                    f"'{seen[pos]['key']}' and '{key_data['key']}' at lines {seen[pos]['line']}, {key_data['line']}"
                )
            else:
                seen[pos] = {'key': key_data['key'], 'line': key_data['line']}

    def validate_matrix_dimensions(self):
        """Validate matrix dimensions and report statistics."""
        if not self.keys:
            self.errors.append("No keys found in CSV")
            return

        max_row = max(k['row'] for k in self.keys) + 1
        max_col = max(k['col'] for k in self.keys) + 1

        print(f"\n✓ Matrix Dimensions: {max_row} rows × {max_col} columns = {max_row * max_col} positions")
        print(f"✓ Total Keys Defined: {len(self.keys)}")

        # Check for gaps
        for r in range(max_row):
            for c in range(max_col):
                if (r, c) not in self.positions:
                    self.warnings.append(f"Empty position at ({r}, {c})")

    def validate_gpio_pins(self, row_gpios: List[int], col_gpios: List[int]):
        """Validate GPIO pin assignments for nRF52832."""
        all_pins = row_gpios + col_gpios

        # Check for duplicates
        if len(all_pins) != len(set(all_pins)):
            duplicates = [pin for pin in all_pins if all_pins.count(pin) > 1]
            self.errors.append(f"Duplicate GPIO pins: {duplicates}")

        # Check pin range
        for pin in all_pins:
            if pin < 0 or pin > 31:
                self.errors.append(f"Invalid GPIO pin number: {pin} (must be 0-31)")

        # Check against reserved pins
        reserved = [p for p in all_pins if p in RESERVED_PINS and RESERVED_PINS[p] is not None]
        if reserved:
            self.warnings.append(f"Using potentially reserved pins: {reserved}")

        # Check total pin count
        total_pins = len(all_pins)
        if total_pins > 24:  # Reasonable limit for matrix
            self.warnings.append(f"High pin count: {total_pins} GPIOs used for matrix")

        # Add to used pins
        self.used_pins.update(all_pins)

        print(f"✓ Row GPIOs: {row_gpios} ({len(row_gpios)} pins)")
        print(f"✓ Column GPIOs: {col_gpios} ({len(col_gpios)} pins)")
        print(f"✓ Total Matrix Pins: {total_pins}")

    def check_peripheral_conflicts(self):
        """Check for conflicts with peripheral GPIO assignments."""
        for peripheral, pin in TKL_RECOMMENDED_GPIO.items():
            if pin in self.used_pins:
                self.errors.append(f"GPIO conflict: {peripheral} uses pin {pin}, which is also used for matrix")

    def generate_dts(self, row_gpios: List[int], col_gpios: List[int]) -> str:
        """Generate DTS kscan and transform nodes."""
        rows = len(row_gpios)
        cols = len(col_gpios)

        # Generate kscan node
        kscan_lines = [
            "    kscan: kscan {",
            "        compatible = \"zmk,kscan-gpio-matrix\";",
            "        label = \"KSCAN\";",
            "        diode-direction = \"col2row\";",
            "",
            "        row-gpios =",
        ]

        for i, pin in enumerate(row_gpios):
            comma = "," if i < len(row_gpios) - 1 else ";"
            kscan_lines.append(f"            <&gpio0 {pin} GPIO_ACTIVE_HIGH>{comma}")
            if pin in self.used_pins:
                pass  # Already tracked

        kscan_lines.append("")
        kscan_lines.append("        col-gpios =")

        for i, pin in enumerate(col_gpios):
            comma = "," if i < len(col_gpios) - 1 else ";"
            kscan_lines.append(f"            <&gpio0 {pin} (GPIO_ACTIVE_HIGH | GPIO_PULL_DOWN)>{comma}")

        kscan_lines.append("    };")

        # Generate matrix transform
        max_row = max(k['row'] for k in self.keys) + 1
        max_col = max(k['col'] for k in self.keys) + 1

        transform_lines = [
            "",
            "    transform: matrix_transform {",
            f"        compatible = \"zmk,matrix-transform\";",
            f"        rows = <{rows}>;",
            f"        columns = <{cols}>;",
            "",
            "        map = <",
        ]

        # Generate RC mappings
        current_row = -1
        for key_data in sorted(self.keys, key=lambda k: (k['row'], k['col'])):
            if key_data['row'] != current_row:
                if current_row >= 0:
                    transform_lines[-1] += "    "  # Add alignment for readability
                transform_lines.append("            ")
                current_row = key_data['row']

            transform_lines[-1] += f"RC({key_data['row']},{key_data['col']}) "
            if key_data['col'] == max_col - 1 and key_data['row'] < max_row - 1:
                transform_lines[-1] = transform_lines[-1].rstrip() + "\n"

        transform_lines.append("        >;")

        # Close braces
        transform_lines.extend(["    };", ""])

        return "\n".join(kscan_lines) + "\n" + "\n".join(transform_lines)

    def run_validation(self, row_gpios: List[int], col_gpios: List[int]):
        """Run all validation checks."""
        print(f"🔍 Validating ZMK Matrix: {self.csv_path.name}")
        print("=" * 60)

        if not self.load_csv():
            print("\n❌ Failed to load CSV")
            return False

        print("\n📋 CSV loaded successfully")

        # Run validations
        self.validate_duplicates()
        self.validate_matrix_dimensions()
        self.validate_gpio_pins(row_gpios, col_gpios)
        self.check_peripheral_conflicts()

        # Print results
        print("\n" + "=" * 60)

        if self.warnings:
            print(f"\n⚠️  WARNINGS ({len(self.warnings)}):")
            for warning in self.warnings:
                print(f"   {warning}")

        if self.errors:
            print(f"\n❌ ERRORS ({len(self.errors)}):")
            for error in self.errors:
                print(f"   {error}")
            return False

        print("\n✅ Validation PASSED!")
        return True

    def print_summary(self):
        """Print a summary of the matrix."""
        print("\n📊 Matrix Summary:")
        print("-" * 60)

        max_row = max(k['row'] for k in self.keys) + 1
        max_col = max(k['col'] for k in self.keys) + 1

        # Create grid visualization
        for r in range(max_row):
            row_keys = []
            for c in range(max_col):
                key = next((k for k in self.keys if k['row'] == r and k['col'] == c), None)
                if key:
                    row_keys.append(key['key'][:6].ljust(6))
                else:
                    row_keys.append("------")
            print(f"Row {r}: {' '.join(row_keys)}")


def main():
    if len(sys.argv) < 2:
        print("Usage: python validate_matrix.py <csv_file> [row_pins...] [--col col_pins...]")
        print("\nExample:")
        print("  python validate_matrix.py tkl_matrix.csv 3 4 5 6 7 8 9 10 --col 11 12 13 14 15 16 17 18")
        print("\nFor TKL (8x8 matrix):")
        print("  python validate_matrix.py tkl_matrix.csv 3 4 5 6 7 8 9 10 --col 11 12 13 14 15 16 17 18")
        sys.exit(1)

    csv_file = sys.argv[1]

    # Parse GPIO arguments
    try:
        separator_idx = sys.argv.index('--col')
        row_gpios = list(map(int, sys.argv[2:separator_idx]))
        col_gpios = list(map(int, sys.argv[separator_idx + 1:]))
    except ValueError:
        print("❌ Error: Use '--col' to separate row and column GPIOs")
        sys.exit(1)

    validator = MatrixValidator(csv_file)

    if validator.run_validation(row_gpios, col_gpios):
        validator.print_summary()

        # Generate DTS
        print("\n📝 Generated DTS:")
        print("=" * 60)
        print(validator.generate_dts(row_gpios, col_gpios))
        print("=" * 60)

        sys.exit(0)
    else:
        sys.exit(1)


if __name__ == "__main__":
    main()
