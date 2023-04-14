#!/bin/python3
import argparse
import serial
import serial.tools.list_ports
import sys

parser = argparse.ArgumentParser()
parser.add_argument("-s", "--spi_index", help="Target SPI flash index ", default=0, type=int)
parser.add_argument("-a", "--activate", help="Reprogram the FPGA only. Don't flash", action="store_true")
parser.add_argument("-f", "--file", help="bin-file that will get written to the flash")
parser.add_argument("-c", "--comport", help="COM Port of the MMC")

args = parser.parse_args()


com_port = None
com_busy = False

UART_BAUDRATE=921600

FLASH_PAGE_SIZE=256

if not args.comport:
    print(u'Available serial ports on system:')
    for n, (portname, desc, hwid) in enumerate(sorted(serial.tools.list_ports.comports())):
        print(u'  {} - {}'.format(portname, desc))

    sys.exit(2)

with serial.Serial(port=args.comport,
        baudrate=UART_BAUDRATE,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=10,
        xonxoff=False,
        rtscts=False,
        dsrdtr=False) as com_port:

    exit_code = 0
    if args.file:
        with open(args.file, "rb") as f:
            bin_file_bytes=f.read()

            # Ceil division
            self.n_pages = -1*(-len(bin_file_bytes)//FLASH_PAGE_SIZE)
            # Pad 0s
            bin_file_bytes += b'\0'*(n_pages*FLASH_PAGE_SIZE-len(bin_file_bytes))

            com_port.reset_input_buffer()
            com_port.write(f"flash_init {n_pages} {args.spi_index}\r\n".encode("utf8"))
            print(com_port.read_until())
            msg = com_port.read_until()
            print(msg)
            success_msg = "Initialising flash write"
            if msg[0:len(success_msg)] != success_msg:
                # Memory allocation didn't occur yet, we can abort
                sys.exit(3)

            preincrement_page = False
            timeout_ctr = 0
            try:
                for page in range(n_pages):
                    cmd_str = "flash_upload" + (" i" if preincrement_page else " r") + "\r\n"
                    com_port.write(cmd_str.encode())
                    print(com_port.read_until())
                    page_bytes = bin_file_bytes[page*FLASH_PAGE_SIZE:(page+1)*FLASH_PAGE_SIZE]
                    com_port.write(page_bytes)
                    readback = com_port.read(FLASH_PAGE_SIZE)
                    if (readback != page_bytes):
                        page -= 1
                        preincrement_page = False
                        if (readback[0:7] == "timeout"):
                            print(readback)
                            timeout_ctr += 1
                        else:
                            print(com_port.read_until())
                    else:
                        preincrement_page = True
                        print(com_port.read_until())

                    if timeout_ctr > 100:
                        break
            except:
                exit_code = 1

            finally:
                # make sure to deallocate memory in openMMC
                com_port.reset_input_buffer()
                com_port.write(f"flash_final {n_pages}\r\n".encode())
                print(com_port.read_until())

    if args.activate and exit_code == 0:
        com_port.reset_input_buffer()
        com_port.write(f"flash_activate_firmware {args.spi_index}\r\n".encode())
        msg = com_port.read_until()
        print(msg)

    sys.exit(exit_code)
