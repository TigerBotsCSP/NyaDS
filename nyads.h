#pragma once
#include <string>
#include <boost/asio.hpp>

class nyads {
public:
    // Data
    uint8_t packets = 0;
    uint8_t mode = 0x00;
    uint8_t alliance = 0x00;


    /// <summary>
    ///  Connect to a robot using its IP and port.
    /// </summary>
    /// <param name="url">IP address of the robot (10.TE.AM.2 if wireless)</param>
    /// <param name="port">Port of the robot's driver station listener, default is 1110</param>
    /// <returns>True if success, false if not</returns>
    int connect(std::string _url, int _port = 1110) {
        if (connected) return true;

        url = _url;
        port = _port;
        //socket.host_ = url;
        //socket.port_ = port;

        if (update()) {
            connected = true;
            return true;
        }
        else {
            return false;
        }
    }

    
    /// <summary>
    ///  Send current data (mode, alliance, etc.) to the robot to update it.
    /// </summary>
    /// <returns>True if success, false if not</returns>
    int update() {
        if (!connected && packets) return false;

        try {
            boost::asio::io_service io_service;
            boost::asio::ip::udp::socket socket(io_service, boost::asio::ip::udp::v4());

            std::array<char, 6> message = { packets >> 8, packets, 0x01, mode, 0x10, alliance };
            boost::asio::ip::udp::endpoint endpoint(boost::asio::ip::address::from_string(url), port);
            socket.send_to(boost::asio::buffer(message), endpoint);

            packets++;
            return true;
        }
        catch (std::exception& e) {
            return false;
        }
    }

    /// <summary>
    /// Set the mode of the robot.
    /// </summary>
    /// <param name="_mode">Mode of the robot (off, teleop, auto, match)</param>
    void set_mode(std::string _mode) {
        if (_mode == "off") mode = 0x00;
        if (_mode == "teleop") mode = 0x04;
        if (_mode == "auto") mode = 0x06;
    }

    /// <summary>
    /// Set the alliance of the robot.
    /// </summary>
    /// <param name="_alliance">Alliance of the robot (red, blue)</param>
    void set_alliance(std::string _alliance) {
        if (_alliance == "red") alliance = 0x00;
        if (_alliance == "blue") alliance = 0x03;
    }
public:
    // Data for connecting and sending
    bool connected;
    std::string url;
    int port;
};
