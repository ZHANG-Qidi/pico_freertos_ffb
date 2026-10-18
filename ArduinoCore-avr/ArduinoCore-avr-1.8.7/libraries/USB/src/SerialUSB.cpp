#include "SerialUSB.h"

#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
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
    if (!_running || !tud_cdc_connected()) {
        return 0;
    }

    taskENTER_CRITICAL();
    const size_t space = SERIAL_USB_TX_BUFFER_SIZE - _tx_buffer_count;
    taskEXIT_CRITICAL();
    return space;
}

void _SerialUSB::flush() {
    // Transmission is asynchronous; queued data is sent without waiting.
}

size_t _SerialUSB::write(uint8_t c) { return write(&c, 1); }

size_t _SerialUSB::write(const uint8_t *buf, size_t size) {
    if (!_running || !buf || !size || !tud_cdc_connected()) {
        return 0;
    }

    taskENTER_CRITICAL();
    const size_t space = SERIAL_USB_TX_BUFFER_SIZE - _tx_buffer_count;
    const size_t accepted = size < space ? size : space;
    const size_t contiguous = SERIAL_USB_TX_BUFFER_SIZE - _tx_buffer_head;
    const size_t first = accepted < contiguous ? accepted : contiguous;
    memcpy(_tx_buffer + _tx_buffer_head, buf, first);
    memcpy(_tx_buffer, buf + first, accepted - first);
    _tx_buffer_head = (_tx_buffer_head + accepted) % SERIAL_USB_TX_BUFFER_SIZE;
    _tx_buffer_count += accepted;
    const bool start = accepted && !_tx_active;
    if (start) _tx_active = true;
    taskEXIT_CRITICAL();
    if (start) transmit();
    return accepted;
}

void _SerialUSB::sendNext(bool flush) {
    if (!_running || !tud_cdc_connected()) {
        taskENTER_CRITICAL();
        _tx_buffer_head = _tx_buffer_tail = _tx_buffer_count = 0;
        _tx_active = false;
        taskEXIT_CRITICAL();
        tud_cdc_write_clear();
        return;
    }

    const size_t space = tud_cdc_write_available();
    taskENTER_CRITICAL();
    if (!_tx_buffer_count) {
        _tx_active = false;
        taskEXIT_CRITICAL();
        return;
    }
    const size_t contiguous = SERIAL_USB_TX_BUFFER_SIZE - _tx_buffer_tail;
    size_t count = _tx_buffer_count < contiguous ? _tx_buffer_count : contiguous;
    if (count > space) count = space;
    const size_t tail = _tx_buffer_tail;
    taskEXIT_CRITICAL();

    if (count) {
        const size_t sent = tud_cdc_write(_tx_buffer + tail, count);
        taskENTER_CRITICAL();
        _tx_buffer_tail = (_tx_buffer_tail + sent) % SERIAL_USB_TX_BUFFER_SIZE;
        _tx_buffer_count -= sent;
        taskEXIT_CRITICAL();
    }
    if (flush) tud_cdc_write_flush();
}

void _SerialUSB::transmit() { sendNext(true); }

void _SerialUSB::on_cdc_tx_complete() {
    // TinyUSB flushes its FIFO after this callback returns.
    sendNext(false);
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
    if (!dtr) SerialUSB.transmit();
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

extern "C" void tud_cdc_tx_complete_cb(uint8_t itf) {
    if (itf == 0) SerialUSB.on_cdc_tx_complete();
}

_SerialUSB SerialUSB;

void arduino::serialEventRun(void) {
    if (serialEvent && SerialUSB.available()) {
        serialEvent();
    }
}
