#include "mbed.h"
#include <climits>

#if !defined(DEVICE_SERIAL_ASYNCH)
    #error "Device doesn't support Async serial"
#endif

static const uint16_t crc_table[] = {
    0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
    0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
    0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
    0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
    0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
    0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
    0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
    0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
    0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
    0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
    0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
    0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
    0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
    0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
    0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
    0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
    0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
    0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
    0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
    0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
    0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
    0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
    0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
    0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
    0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
    0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
    0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
    0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
    0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
    0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
    0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
    0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040
};

template <uint8_t N>
class ModbusSlave {
  public:
    enum Exception {
        illegalFunction = 1,
        illegalDataAddress,
        illegalDataValue,
        slaveDeviceFailure
    };

    enum FunctionCode {
        fc03 = 3
    };

    ModbusSlave(Serial * serial_obj, PinName de = NC, PinName re = NC);
    void init(uint8_t slave_id);
    void sendPacket(const uint8_t* data, uint8_t len);
    void sleep(bool state);
    void write(uint8_t index, const uint8_t * data, uint8_t len);
    void write(uint8_t index, uint16_t data);
    uint16_t crc(const uint8_t *data, uint8_t len);

  private:
    void rx_cb(int event);
    void tx_done_cb(int event);
    bool _sleep;
    uint8_t _slave_id;
    uint8_t test[7];
    uint8_t _rx_buffer[8];
    uint8_t _store_buffer[(N * 2)];
#if N > 125
    uint8_t _tx_buffer[255];
#else
    uint8_t _tx_buffer[(N * 2) + 5];
#endif

    void exceptionResponse(Exception type, FunctionCode fn);

  protected:
    Serial * _serial;
    DigitalOut _de;
    DigitalOut _re;
    Mutex _mutex;
};

template <uint8_t N>
ModbusSlave<N>::ModbusSlave(Serial * serial_obj, PinName de, PinName re):
    _sleep(true),
    _de(de),
    _re(re) {
    _serial = serial_obj;
    sleep(true);

    memset(_rx_buffer, UCHAR_MAX, 8);
}

template <uint8_t N>
void ModbusSlave<N>::init(uint8_t slave_id) {
    sleep(false);
    _slave_id = slave_id;
}

template <uint8_t N>
void ModbusSlave<N>::rx_cb(int event) {
    if (event & SERIAL_EVENT_RX_COMPLETE) {
        printf("\nrx: %02X %02X %02X %02X %02X %02X %02X %02X\n", _rx_buffer[0], _rx_buffer[1], _rx_buffer[2], _rx_buffer[3], _rx_buffer[4],
               _rx_buffer[5], _rx_buffer[6], _rx_buffer[7]);

        if (crc(_rx_buffer, 6) == ((_rx_buffer[7] << 8) | _rx_buffer[6])) { // if the calculated crc matches the received crc
            if (_rx_buffer[0] == _slave_id || _rx_buffer[0] == 0) {
                uint16_t starting_address = ((_rx_buffer[2] << 8) | _rx_buffer[3]); // combine the starting address bytes
                uint16_t no_of_registers = ((_rx_buffer[4] << 8) | _rx_buffer[5]); // combine the number of register bytes

                if (_rx_buffer[1] == fc03) {
                    if (starting_address < N) { // check exception 2 ILLEGAL DATA ADDRESS
                        if ((starting_address + N) <= N) { // check exception 3 ILLEGAL DATA VALUE
                            uint8_t no_of_bytes = no_of_registers * 2;
                            printf("no of regiters: %u bytes: %u\n", no_of_registers, no_of_bytes);
                            printf("starting address: %u\n", starting_address);

                            uint8_t response_frame_size = 5 + no_of_bytes;

                            _tx_buffer[0] = _slave_id;
                            _tx_buffer[1] = fc03;
                            _tx_buffer[2] = no_of_bytes;

                            printf("frame_size: %u\n", response_frame_size);

                            _mutex.lock();
                            memcpy(_tx_buffer + 3, _store_buffer, no_of_bytes);
                            _mutex.unlock();

                            uint16_t crc16 = crc(_tx_buffer, no_of_bytes + 3);
                            _tx_buffer[response_frame_size - 2] = crc16 & UCHAR_MAX;
                            _tx_buffer[response_frame_size - 1] = crc16 >> 8;
                            sendPacket(_tx_buffer, response_frame_size);

                        } else {
                            exceptionResponse(illegalDataValue, fc03);
                        }

                    } else {
                        exceptionResponse(illegalDataAddress, fc03);
                    }

                } else {
                    printf("different function:%u\n", _rx_buffer[1]);
                }

            } else {
                printf("different slave\n");
            }

        } else {
            uint16_t c = crc(_rx_buffer, 6);
            printf("CRC failed %u != %u\n", ((_rx_buffer[7] << 8) | _rx_buffer[6]), c);
        }

    } else {
        printf("RX error: %i\n", event);
    }

    _serial->read(_rx_buffer, 8, event_callback_t(this, &ModbusSlave::rx_cb), SERIAL_EVENT_RX_ALL);
}

template <uint8_t N>
void ModbusSlave<N>::tx_done_cb(int event) {
    if (_de != NC) _de.write(0);

    if (_re != NC) _re.write(0);
}

template <uint8_t N>
void ModbusSlave<N>::sendPacket(const uint8_t* data, uint8_t len) {
    if (!_sleep) {
        if (_de != NC) _de.write(1);

        if (_re != NC) _re.write(1);

        _serial->write(_tx_buffer, len, event_callback_t(this, &ModbusSlave::tx_done_cb));
    }

}

template <uint8_t N>
void ModbusSlave<N>::sleep(bool state) {
    if (state) {
        _serial->read(_rx_buffer, 0, 0, 0);

        if (_de != NC) _de.write(0); //shutdown mode

        if (_de != NC) _re.write(1); //shutdown mode

    } else {
        _serial->read(_rx_buffer, 8, event_callback_t(this, &ModbusSlave::rx_cb), SERIAL_EVENT_RX_ALL);

        if (_de != NC) _de.write(0);  // enable receiving

        if (_re != NC) _re.write(0);  // enable receiving
    }

    _sleep = state;
}

template <uint8_t N>
void ModbusSlave<N>::write(uint8_t index, const uint8_t * data, uint8_t len) {
    _mutex.lock();
    memcpy(_store_buffer + index, data, len);
    _mutex.unlock();
}

template <uint8_t N>
void ModbusSlave<N>::write(uint8_t index, uint16_t data) {
    _mutex.lock();
    _store_buffer[index] = (data >> 8);
    _store_buffer[index + 1] = (data & UCHAR_MAX);
    _mutex.unlock();
}

template <uint8_t N>
void ModbusSlave<N>::exceptionResponse(Exception type, FunctionCode fn) {
    _tx_buffer[0] = _slave_id;
    _tx_buffer[1] = (fn | 0x80);
    _tx_buffer[2] = type;

    uint16_t crc16 = crc(_tx_buffer, 3);

    _tx_buffer[3] = crc16 >> 8;
    _tx_buffer[4] = crc16 & 0xFF;

    sendPacket(_tx_buffer, 5);
}

template <uint8_t N>
uint16_t ModbusSlave<N>::crc(const uint8_t *data, uint8_t len) {
    uint8_t temp;
    uint16_t result = 0xFFFF;

    while (len--) {
        temp = *data++ ^ result;
        result >>= 8;
        result ^= crc_table[temp];
    }

    return result;
}