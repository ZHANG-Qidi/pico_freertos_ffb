#include "SerialUSB.h"

#include "tusb.h"

#ifndef DISABLE_USB_SERIAL
// Ensure we are installed in the USB chain
void __USBInstallSerial() { /* noop */ }
#endif

// SerialEvent functions are weak, so when the user doesn't define them,
// the linker just sets their address to 0 (which is checked below).
// The Serialx_available is just a wrapper around Serialx.available(),
// but we can refer to it weakly so we don't pull in the entire
// HardwareSerial instance if the user doesn't also refer to it.
extern void serialEvent() __attribute__((weak));

void _SerialUSB::begin(unsigned long baud) {
    (void)baud;  // ignored

    if (_running) {
        return;
    }

    _running = true;
}

bool _SerialUSB::connected() { return (tud_cdc_connected() || ignore_dtr); }

void _SerialUSB::end() {
    // TODO
}

int _SerialUSB::peek() {
    if (!_running) {
        return 0;
    }

    uint8_t c;
    return tud_cdc_peek(&c) ? (int)c : -1;
}

int _SerialUSB::read() {
    if (!_running) {
        return -1;
    }

    if (connected() && tud_cdc_available()) {
        return tud_cdc_read_char();
    }
    return -1;
}

int _SerialUSB::available() {
    if (!_running) {
        return 0;
    }

    return tud_cdc_available();
}

int _SerialUSB::availableForWrite() {
    if (!_running) {
        return 0;
    }

    return tud_cdc_write_available();
}

void _SerialUSB::flush() {
    if (!_running) {
        return;
    }

    tud_cdc_write_flush();
}

size_t _SerialUSB::write(uint8_t c) { return write(&c, 1); }

#include "device/usbd_pvt.h"

size_t _SerialUSB::write(const uint8_t *buf, size_t size) {
    if (!_running) {
        return 0;
    }

    if (!connected()) {
        return 0;
    }
    usbd_int_set(false);
    size_t to_send = size, so_far = 0;
    while (to_send) {
        size_t space = tud_cdc_write_available();
        if (!space) {
            usbd_int_set(true);
            tud_cdc_write_flush();
            continue;
        }
        if (space > to_send) {
            space = to_send;
        }
        size_t sent = tud_cdc_write(buf + so_far, space);
        usbd_int_set(true);
        if (sent) {
            so_far += sent;
            to_send -= sent;
            tud_cdc_write_flush();
        } else {
            size = so_far;
            break;
        }
    }
    return size;
}

_SerialUSB::operator bool() {
    static bool first_call = true;

    if (!_running) {
        return false;
    }

    bool rv = connected();

    if (rv && first_call) {
        delay(10);
        first_call = false;
    }
    return rv;
}

int _SerialUSB::_bps, _SerialUSB::_bits, _SerialUSB::_parity, _SerialUSB::_stop;
bool _SerialUSB::_dtr, _SerialUSB::_rts;

static void CheckSerialReset() {
#if NO_1200_BPS_RESET
    return;
#endif

    if ((_SerialUSB::_bps == 1200) && (!_SerialUSB::_dtr)) {
        // goBootloader();
    }
}

extern "C" void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
    (void)itf;
    _SerialUSB::_dtr = dtr ? true : false;
    _SerialUSB::_rts = rts ? true : false;
    CheckSerialReset();
}

extern "C" void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const *p_line_coding) {
    (void)itf;
    _SerialUSB::_bps = p_line_coding->bit_rate;
    _SerialUSB::_parity = p_line_coding->parity;
    _SerialUSB::_stop = p_line_coding->stop_bits;
    _SerialUSB::_bits = p_line_coding->data_bits;
    CheckSerialReset();
}

_SerialUSB SerialUSB;

void arduino::serialEventRun(void) {
    if (serialEvent && SerialUSB.available()) {
        serialEvent();
    }
}
