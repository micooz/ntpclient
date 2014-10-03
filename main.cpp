#include <iostream>
#include "boost/asio.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

using namespace boost::posix_time;
using namespace boost::asio::ip;

boost::asio::io_service io;

static void reverseByteOrder(uint64_t &in) {
    uint64_t rs = 0;
    int len = sizeof(uint64_t);
    for (int i = 0; i < len; i++) {
        std::memset(reinterpret_cast<uint8_t*>(&rs) + len - 1 - i
                    , static_cast<uint8_t> ((in & 0xFFLL << (i * 8)) >> i * 8)
                    , 1);
    }
    in = rs;
}

class NtpPacket {
public:
    NtpPacket() {
        _rep._flags = 0xdb;
        //    11.. ....    Leap Indicator: unknown
        //    ..01 1...    NTP Version 3
        //    .... .011    Mode: client
        _rep._pcs = 0x00;//unspecified
        _rep._ppt = 0x01;
        _rep._pcp = 0x01;
        _rep._rdy = 0x01000000;//big-endian
        _rep._rdn = 0x01000000;
        _rep._rid = 0x00000000;
        _rep._ret = 0x0;
        _rep._ort = 0x0;
        _rep._rct = 0x0;
        _rep._trt = 0x0;
    }

    friend std::ostream& operator<<(std::ostream& os, const NtpPacket& ntpacket) {
        return os.write(reinterpret_cast<const char *>(&ntpacket._rep), sizeof(ntpacket._rep));
    }

    friend std::istream& operator>>(std::istream& is, NtpPacket& ntpacket) {
        return is.read(reinterpret_cast<char*>(&ntpacket._rep), sizeof(ntpacket._rep));
    }

public:
#pragma pack(1)
    struct NtpHeader {
        uint8_t _flags;//Flags
        uint8_t _pcs;//Peer Clock Stratum
        uint8_t _ppt;//Peer Polling Interval
        uint8_t _pcp;//Peer Clock Precision
        uint32_t _rdy;//Root Delay
        uint32_t _rdn;//Root Dispersion
        uint32_t _rid;//Reference ID
        uint64_t _ret;//Reference Timestamp
        uint64_t _ort;//Origin Timestamp
        uint64_t _rct;//Receive Timestamp
        uint64_t _trt;//Transmit Timestamp
    };
#pragma pack()
    NtpHeader _rep;
};

class NtpClient {
public:
    NtpClient(const std::string& serverIp)
        :_socket(io), _serverIp(serverIp) {
    }

    time_t getTime() {
        if (_socket.is_open()) {
            _socket.shutdown(udp::socket::shutdown_both, _ec);
            if (_ec) {
                std::cout << _ec.message() << std::endl;
                _socket.close();
                return 0;
            }
            _socket.close();
        }
        udp::endpoint ep(boost::asio::ip::address_v4::from_string(_serverIp), NTP_PORT);
        NtpPacket request;
        std::stringstream ss;
        std::string buf;
        ss << request;
        ss >> buf;
        _socket.open(udp::v4());
        _socket.send_to(boost::asio::buffer(buf), ep);
        std::array<uint8_t, 128> recv;
        size_t len = _socket.receive_from(boost::asio::buffer(recv), ep);
        uint8_t* pBytes = recv.data();
        /****dump hex data
        for (size_t i = 0; i < len; i++) {
        if (i % 16 == 0) {
        std::cout << std::endl;
        }
        else {
        std::cout << std::setw(2) << std::setfill('0')
        << std::hex << (uint32_t) pBytes[i];
        std::cout << ' ';
        }
        }
        ****/
        time_t tt;
        uint64_t last;
        uint32_t seconds;
        /****get the last 8 bytes(Transmit Timestamp) from received packet.
        std::memcpy(&last, pBytes + len - 8, sizeof(last));
        ****create a NtpPacket*/
        NtpPacket resonpse;
        std::stringstream rss;
        rss.write(reinterpret_cast<const char*>(pBytes), len);
        rss >> resonpse;
        last = resonpse._rep._trt;
        //
        reverseByteOrder(last);
        seconds = (last & 0x7FFFFFFF00000000) >> 32;
        tt = seconds + 8 * 3600 * 2 - 61533950;
        return tt;
    }

private:
    const uint16_t NTP_PORT = 123;
    udp::socket _socket;
    std::string _serverIp;
    boost::system::error_code _ec;
};

int main(int argc, char* agrv[]) {
    NtpClient ntp("129.6.15.28");
    int n = 5;
    while (n--) {
        time_t tt = ntp.getTime();
        boost::posix_time::ptime utc = from_time_t(tt);
        std::cout << "Local Timestamp:" << time(0) << '\t' << "NTP Server:" << tt << "(" << to_simple_string(utc) << ")" << std::endl;
        Sleep(10);
    }
    return 0;
}
