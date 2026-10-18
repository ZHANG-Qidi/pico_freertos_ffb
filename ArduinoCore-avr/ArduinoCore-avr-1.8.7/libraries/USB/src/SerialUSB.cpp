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

    taskENTER_CRITICAL();
    const int c = _rx_buffer_head == _rx_buffer_tail ? -1 : _rx_buffer[_rx_buffer_tail];
    taskEXIT_CRITICAL();
    return c;
}

int _SerialUSB::read() {
    if (!_running) {
        return -1;
    }

    if (!connected()) return -1;

    taskENTER_CRITICAL();
    if (_rx_buffer_head == _rx_buffer_tail) {
        taskEXIT_CRITICAL();
        return -1;
    }
    const int c = _rx_buffer[_rx_buffer_tail];
    _rx_buffer_tail = (_rx_buffer_tail + 1) % SERIAL_USB_RX_BUFFER_SIZE;
    taskEXIT_CRITICAL();
    return c;
}

int _SerialUSB::available() {
    if (!_running) {
        return 0;
    }

    taskENTER_CRITICAL();
    const size_t count = (SERIAL_USB_RX_BUFFER_SIZE + _rx_buffer_head - _rx_buffer_tail) % SERIAL_USB_RX_BUFFER_SIZE;
    taskEXIT_CRITICAL();
    return count;
}

int _SerialUSB::availableForWrite() {
    if (!_running || !tud_cdc_connected()) {
        return 0;
    }

    taskENTER_CRITICAL();
    const size_t head = _tx_buffer_head;
    const size_t tail = _tx_buffer_tail;
    const size_t space = head >= tail ? SERIAL_USB_TX_BUFFER_SIZE - 1 - (head - tail) : (tail - head) - 1;
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
    size_t direct = 0;
    if (_tx_buffer_head == _tx_buffer_tail) {
        const size_t available = tud_cdc_write_available();
        direct = tud_cdc_write(buf, size < available ? size : available);
    }
    buf += direct;
    size -= direct;
    const size_t head = _tx_buffer_head;
    const size_t tail = _tx_buffer_tail;
    const size_t space = head >= tail ? SERIAL_USB_TX_BUFFER_SIZE - 1 - (head - tail) : (tail - head) - 1;
    const size_t accepted = size < space ? size : space;
    const size_t contiguous = SERIAL_USB_TX_BUFFER_SIZE - head;
    const size_t first = accepted < contiguous ? accepted : contiguous;
    memcpy(_tx_buffer + head, buf, first);
    memcpy(_tx_buffer, buf + first, accepted - first);
    _tx_buffer_head = (head + accepted) % SERIAL_USB_TX_BUFFER_SIZE;
    taskEXIT_CRITICAL();
    if (direct) tud_cdc_write_flush();
    return direct + accepted;
}

void _SerialUSB::clearTx() {
    taskENTER_CRITICAL();
    _tx_buffer_head = _tx_buffer_tail = 0;
    taskEXIT_CRITICAL();
    tud_cdc_write_clear();
}

void _SerialUSB::clearRx() {
    taskENTER_CRITICAL();
    _rx_buffer_head = _rx_buffer_tail = 0;
    taskEXIT_CRITICAL();
    tud_cdc_read_flush();
}

void _SerialUSB::on_cdc_rx_complete() {
    uint8_t data[CFG_TUD_CDC_RX_BUFSIZE];
    const size_t received = tud_cdc_read(data, sizeof(data));

    taskENTER_CRITICAL();
    const size_t head = _rx_buffer_head;
    const size_t tail = _rx_buffer_tail;
    const size_t space = head >= tail ? SERIAL_USB_RX_BUFFER_SIZE - 1 - (head - tail) : (tail - head) - 1;
    const size_t accepted = received < space ? received : space;
    const size_t contiguous = SERIAL_USB_RX_BUFFER_SIZE - head;
    const size_t first = accepted < contiguous ? accepted : contiguous;
    memcpy(_rx_buffer + head, data, first);
    memcpy(_rx_buffer, data + first, accepted - first);
    _rx_buffer_head = (head + accepted) % SERIAL_USB_RX_BUFFER_SIZE;
    taskEXIT_CRITICAL();
}

void _SerialUSB::on_cdc_tx_complete() {
    if (!_running || !tud_cdc_connected()) {
        clearTx();
        return;
    }

    // TinyUSB flushes its FIFO after this callback returns.
    taskENTER_CRITICAL();
    if (_tx_buffer_head != _tx_buffer_tail) {
        const size_t space = tud_cdc_write_available();
        size_t count = _tx_buffer_head > _tx_buffer_tail ? _tx_buffer_head - _tx_buffer_tail : SERIAL_USB_TX_BUFFER_SIZE - _tx_buffer_tail;
        count = count > space ? space : count;
        const size_t sent = tud_cdc_write(_tx_buffer + _tx_buffer_tail, count);
        _tx_buffer_tail = (_tx_buffer_tail + sent) % SERIAL_USB_TX_BUFFER_SIZE;
    }
    taskEXIT_CRITICAL();
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
    if (!dtr) {
        SerialUSB.clearTx();
        SerialUSB.clearRx();
    }
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

extern "C" void tud_cdc_rx_cb(uint8_t itf) {
    if (itf == 0) SerialUSB.on_cdc_rx_complete();
}

_SerialUSB SerialUSB;

void arduino::serialEventRun(void) {
    if (serialEvent && SerialUSB.available()) {
        serialEvent();
    }
}
