/*
    Serial-over-USB for the Raspberry Pi Pico RP2040
    Copyright (c) 2021 Earle F. Philhower, III <earlephilhower@yahoo.com>
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include <Arduino.h>
#include <stdarg.h>

#ifndef SERIAL_USB_RX_BUFFER_SIZE
#define SERIAL_USB_RX_BUFFER_SIZE 512
#endif

#ifndef SERIAL_USB_TX_BUFFER_SIZE
#define SERIAL_USB_TX_BUFFER_SIZE 512
#endif

class _SerialUSB : public Stream {
   public:
    _SerialUSB() {}
    void begin(unsigned long baud = 115200);
    void begin(unsigned long baud, uint16_t config) {
        (void)config;
        begin(baud);
    };
    void end();

    virtual int peek() override;
    virtual int read() override;
    virtual int available() override;
    virtual int availableForWrite() override;
    virtual void flush() override;
    virtual size_t write(uint8_t c) override;
    virtual size_t write(const uint8_t *p, size_t len) override;
    using Print::write;
    operator bool();

    void clearTx();
    void clearRx();
    void on_cdc_rx_complete();
    void on_cdc_tx_complete();

    uint32_t baud() { return _bps; }
    uint8_t stopbits() { return _stop; }
    uint8_t paritytype() { return _parity; }
    uint8_t numbits() { return _bits; }
    bool dtr() {
        // this means we are actively using dtr signal, so ignore its meaning as "serial port open"
        ignore_dtr = true;
        return (_dtr != 0);
    }
    bool rts() { return (_rts != 0); }

    static int _bps, _bits, _parity, _stop;
    static bool _dtr, _rts;

   private:
    bool _running = false;
    bool ignore_dtr = false;
    bool connected();
    uint8_t _rx_buffer[SERIAL_USB_RX_BUFFER_SIZE];
    size_t _rx_buffer_head = 0;
    size_t _rx_buffer_tail = 0;
    uint8_t _tx_buffer[SERIAL_USB_TX_BUFFER_SIZE];
    size_t _tx_buffer_head = 0;
    size_t _tx_buffer_tail = 0;
};

extern _SerialUSB SerialUSB;

namespace arduino {
extern void serialEventRun(void) __attribute__((weak));
};
