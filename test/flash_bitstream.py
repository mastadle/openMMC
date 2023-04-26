#!/bin/python3
import argparse
import serial
import serial.tools.list_ports
import sys
import time

parser = argparse.ArgumentParser()
parser.add_argument("-s", "--spi_index", help="Target SPI flash index ", default=0, type=int)
parser.add_argument("-a", "--activate", help="Reprogram the FPGA", action="store_true")
parser.add_argument("-f", "--file", help="bin-file that will get written to the flash")
parser.add_argument("-o", "--verify_only", help="Verify the flash against file but don't flash", action="store_true")
parser.add_argument("-c", "--comport", help="COM Port of the MMC")
parser.add_argument("-v", "--verbose", help="Print debug output", action="store_true")

args = parser.parse_args()


com_port = None
com_busy = False

UART_BAUDRATE=921600

FLASH_PAGE_SIZE=256
ENTER="\r"

def debug_print(msg):
    if args.verbose:
        print(msg)

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
            # bin_file_bytes=bin_file_bytes[0:10*FLASH_PAGE_SIZE]

            # Ceil division
            n_pages = -1*(-len(bin_file_bytes)//FLASH_PAGE_SIZE)
            # Pad 1s
            bin_file_bytes += b'\1'*(n_pages*FLASH_PAGE_SIZE-len(bin_file_bytes))
            if not args.verify_only:
                com_port.reset_input_buffer()
                com_port.write(f"flash_init {args.spi_index} {n_pages}{ENTER}".encode("utf8"))
                debug_print(com_port.read_until())
                start = time.monotonic()
                # Expected erase time plus 10 seconds
                # com_port.timeout = len(bin_file_bytes)//(400*1024) + 10
                com_port.timeout = None
                msg = com_port.read_until()
                stop = time.monotonic()
                com_port.timeout = 20
                print(f"Waited for {stop-start:.3} s")
                print(msg)
                success_msg = "Initialising flash write"
                if msg[0:len(success_msg)].decode() != success_msg:
                    # Memory allocation didn't occur yet, we can abort
                    sys.exit(3)

            repeat_page = False
            timeout_ctr = 0
            try:
                page = 0
                uart_page_error_rate = 0
                spi_page_error_rate = 0
                while (page < n_pages):
                    if page % 100 == 0:
                        print(f"--- page {page} of {n_pages} ---")
                        print(f"uart page errors: {uart_page_error_rate}, spi page errors: {spi_page_error_rate}")
                        uart_page_error_rate = 0
                        spi_page_error_rate = 0
                    page_bytes = bin_file_bytes[page*FLASH_PAGE_SIZE:(page+1)*FLASH_PAGE_SIZE]
                    if args.verify_only:
                        # Don't wait for flash to be ready. Reads can always be used
                        cmd_str = f"flash_read {args.spi_index} {page} 0{ENTER}"
                        com_port.write(cmd_str.encode())
                        debug_print(com_port.read_until())
                        readback = com_port.read(FLASH_PAGE_SIZE)
                        if (readback != page_bytes):
                            print(f"--- page {page} of {n_pages} ---")
                            print(page_bytes)
                            print(readback)
                            spi_page_error_rate += 1
                        debug_print(com_port.read_until())
                    else:
                        cmd_str = "flash_upload" + (" r" if repeat_page else " i") + ENTER
                        com_port.write(cmd_str.encode())
                        debug_print(com_port.read_until())
                        com_port.write(page_bytes)
                        readback = com_port.read(FLASH_PAGE_SIZE)
                        msg = com_port.read_until()
                        debug_print(msg)
                        # More than just newline reply, assume timeout
                        if (len(msg) > 3):
                            timeout_ctr += 1
                            print("Wait for 1 second")
                            time.sleep(1)
                            com_port.reset_input_buffer()
                        else:
                            if (readback != page_bytes):
                                repeat_page = True
                                debug_print(page_bytes)
                                debug_print(readback)
                                uart_page_error_rate += 1
                            else:
                                # Wait for flash write to finish
                                cmd_str = f"flash_read {args.spi_index} {page} 1{ENTER}"
                                com_port.write(cmd_str.encode())
                                debug_print(com_port.read_until())
                                readback = com_port.read(FLASH_PAGE_SIZE)
                                if (readback != page_bytes):
                                    repeat_page = True
                                    debug_print(page_bytes)
                                    debug_print(readback)
                                    spi_page_error_rate += 1
                                else:
                                    repeat_page = False
                                debug_print(com_port.read_until())

                    if timeout_ctr > 100:
                        print(f"--- Timeout at page {page} of {n_pages} ---")
                        print(page_bytes)
                        print(readback)
                        break

                    if not repeat_page:
                        page += 1

            except Exception as e:
                print(e)
                exit_code = 1

            finally:
                if not args.verify_only:
                    # make sure to deallocate memory in openMMC
                    com_port.reset_input_buffer()
                    com_port.write(f"flash_final {n_pages}{ENTER}".encode())
                    debug_print(com_port.read_until())
                    if exit_code == 0:
                        print(f"Flashing successful")

    if args.activate and exit_code == 0:
        com_port.reset_input_buffer()
        com_port.write(f"flash_activate_firmware {args.spi_index}{ENTER}".encode())
        # com_port.write(f"fpga_reset{ENTER}".encode())
        debug_print(com_port.read_until())
        msg = com_port.read_until()
        debug_print(msg)
        print(f"Activated firmware on flash {args.spi_index}")

    sys.exit(exit_code)
