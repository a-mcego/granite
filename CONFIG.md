
# Configuration File Options

## CPU Configuration
```
cpu <type>
```
Specify the CPU type. Available options:
- `8086` or `86`: Intel 8086
- `8088` or `88`: Intel 8088
- `80186` or `186`: Intel 80186
- `80188` or `188`: Intel 80188
- `8088mc` or `88mc`: Intel 8088 (micro-coded version)
- `80286` or `286`: Intel 80286

## CPU Clock Multiplier
```
cpu_multiplier <numerator>/<denominator>
```
Set the CPU clock multiplier as a fraction. Example: `cpu_multiplier 1/2`
- Both numerator and denominator must be positive numbers
- Denominator cannot be zero

## ROM Configuration
```
rom <hex_address> <filename> [stride=<n>]
```
Load a ROM file at the specified address.
- `hex_address`: Starting address in hexadecimal
- `filename`: Path to the ROM file
- `stride`: Optional parameter to specify memory stride (default: 1)

## Drive Configuration
```
load <drive_letter> <image_filename> [chs=<cylinders>,<heads>,<sectors>]
```
Load disk images into drives:
- `drive_letter`: A-D (A: and B: for floppy drives, C: and D: for hard drives)
- `image_filename`: Path to the disk image
- `chs`: Optional parameter for hard drives to specify Cylinder,Head,Sector geometry

## Machine Type
```
machine <type>
```
Specify the machine type. Available options:
- `pc`: IBM PC
- `xt`: IBM XT
- `at`: IBM AT

## CMOS Configuration
```
cmos_file <filename>
```
Specify the CMOS configuration file path.

## Debug Options
```
trace
```
Enable instruction tracing for debugging.

## Test Mode
```
test <test_filename>
```
Run the emulator in test mode using the specified test file.

```
end_tests
```
End test mode and display results.

## Sound Configuration
```
sound <setting>
```
Configure sound output. Available settings:
- `yes`, `on`, `true`: Enable sound
- `no`, `off`, `false`: Disable sound

## Comments
Lines beginning with `#` are treated as comments and ignored.

## Example Configuration
```
# Set CPU type
cpu 8088

# Set CPU multiplier to half speed
cpu_multiplier 1/2

# Load BIOS ROM
rom F0000 bios.bin

# Load floppy disk image
load A floppy.img

# Load hard disk with custom geometry
load C harddisk.img chs=1024,16,63

# Set machine type
machine xt

# Enable sound
sound on
```