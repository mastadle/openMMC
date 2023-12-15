#!/bin/python3
import argparse
import serial
import serial.tools.list_ports
import sys
import time
import tqdm

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

def _print(msg, debug=False, pbar=None):
    if args.verbose or not debug:
        if pbar is not None:
            pbar.write(str(msg))
        else:
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

    # Might be required on Windows
    # com_port.set_buffer_size(4096, 4096)

    exit_code = 0
    if args.file:
        with open(args.file, "rb") as f:
            bin_file_bytes=f.read()
            # Write only 1000 pages for debugging the script
            # bin_file_bytes=bin_file_bytes[0:1000*FLASH_PAGE_SIZE]

            # Ceil division
            n_pages = -1*(-len(bin_file_bytes)//FLASH_PAGE_SIZE)
            # Pad 1s
            bin_file_bytes += b'\1'*(n_pages*FLASH_PAGE_SIZE-len(bin_file_bytes))
            if not args.verify_only:
                com_port.reset_input_buffer()
                com_port.write(f"flash_init {args.spi_index} {n_pages}{ENTER}".encode("utf8"))
                _print(com_port.read_until(), debug=True)
                start = time.monotonic()
                # Expected erase time plus 10 seconds
                # com_port.timeout = len(bin_file_bytes)//(400*1024) + 10
                com_port.timeout = None
                msg = com_port.read_until()
                stop = time.monotonic()
                com_port.timeout = 20
                _print(f"Waited for {stop-start:.3} s")
                _print(msg)
                success_msg = "Initialising flash write"
                if msg[0:len(success_msg)].decode() != success_msg:
                    # Memory allocation didn't occur yet, we can abort
                    sys.exit(3)

            repeat_page = False
            timeout_ctr = 0
            try:
                page = 0
                page_errors = {
                        "uart page errors": 0,
                        "spi page errors": 0
                }
                with tqdm.tqdm(total=n_pages, postfix=page_errors) as pbar:
                    while (page < n_pages):
                        page_bytes = bin_file_bytes[page*FLASH_PAGE_SIZE:(page+1)*FLASH_PAGE_SIZE]
                        if args.verify_only:
                            # Don't wait for flash to be ready. Reads can always be used
                            cmd_str = f"flash_read {args.spi_index} {page} 0{ENTER}"
                            com_port.write(cmd_str.encode())
                            _print(com_port.read_until(), debug=True, pbar=pbar)
                            readback = com_port.read(FLASH_PAGE_SIZE)
                            if (readback != page_bytes):
                                _print(f"--- page {page} of {n_pages} ---", pbar=pbar)
                                _print(page_bytes, pbar=pbar)
                                _print(readback, pbar=pbar)
                                page_errors['spi page errors'] += 1
                            _print(com_port.read_until(), debug=True, pbar=pbar)
                        else:
                            cmd_str = "flash_upload" + (" r" if repeat_page else " i") + ENTER
                            com_port.write(cmd_str.encode())
                            _print(com_port.read_until(), debug=True, pbar=pbar)
                            com_port.write(page_bytes)
                            readback = com_port.read(FLASH_PAGE_SIZE)
                            msg = com_port.read_until()
                            _print(msg, debug=True, pbar=pbar)
                            # More than just newline reply, assume timeout
                            if (len(msg) > 3):
                                timeout_ctr += 1
                                if (readback != page_bytes):
                                    _print(page_bytes, debug=True, pbar=pbar)
                                    _print(readback, debug=True, pbar=pbar)
                                    page_errors['uart page errors'] += 1
                                _print("Wait for 1 second", pbar=pbar)
                                time.sleep(1)
                                com_port.reset_input_buffer()
                            else:
                                timeout_ctr = 0
                                if (readback != page_bytes):
                                    repeat_page = True
                                    _print(page_bytes, debug=True, pbar=pbar)
                                    _print(readback, debug=True, pbar=pbar)
                                    page_errors['uart page errors'] += 1
                                else:
                                    # Wait for flash write to finish
                                    cmd_str = f"flash_read {args.spi_index} {page} 1{ENTER}"
                                    com_port.write(cmd_str.encode())
                                    _print(com_port.read_until(), debug=True, pbar=pbar)
                                    readback = com_port.read(FLASH_PAGE_SIZE)
                                    if (readback != page_bytes):
                                        repeat_page = True
                                        _print(page_bytes, debug=True, pbar=pbar)
                                        _print(readback, debug=True, pbar=pbar)
                                        page_errors['spi page errors'] += 1
                                    else:
                                        repeat_page = False
                                    _print(com_port.read_until(), debug=True, pbar=pbar)

                        if timeout_ctr > 100:
                            _print(f"--- Timeout at page {page} of {n_pages} ---", pbar=pbar)
                            _print(page_bytes, pbar=pbar)
                            _print(readback, pbar=pbar)
                            break

                        if not repeat_page and timeout_ctr == 0:
                            page += 1
                            pbar.update()

            except Exception as e:
                print(e)
                exit_code = 1

            finally:
                if not args.verify_only:
                    # make sure to deallocate memory in openMMC
                    com_port.reset_input_buffer()
                    com_port.write(f"flash_final {n_pages}{ENTER}".encode())
                    _print(com_port.read_until(), debug=True)
                    if exit_code == 0:
                        _print(f"Flashing successful")

    if args.activate and exit_code == 0:
        com_port.reset_input_buffer()
        com_port.write(f"flash_activate_firmware {args.spi_index}{ENTER}".encode())
        # com_port.write(f"fpga_reset{ENTER}".encode())
        _print(com_port.read_until(), debug=True)
        msg = com_port.read_until()
        _print(msg, debug=True)
        print(f"Activated firmware on flash {args.spi_index}")

    sys.exit(exit_code)
