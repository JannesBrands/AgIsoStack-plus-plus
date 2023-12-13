#include "isobus/hardware_integration/available_can_drivers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/socket_can_interface.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_configuration.hpp"
#include "isobus/isobus/can_network_manager.hpp"

#include <atomic>
#include <csignal>
#include <memory>
#include <iostream>

static std::atomic_bool running = { true };
static constexpr std::uint32_t CAN_CHANNEL = 0;
static constexpr std::uint16_t DELAY_MS = 2000;
static constexpr std::uint16_t MANUFACTURER_ID = 94u;  //CNH Industrial N.V. (formerly CNH Global N.V.)

void signal_handler(int)
{
	running = false;
}

int main() {
    std::signal(SIGINT, signal_handler);

    std::signal(SIGINT, signal_handler);

	std::shared_ptr<isobus::CANHardwarePlugin> canDriver = nullptr;
#if defined(ISOBUS_SOCKETCAN_AVAILABLE)
	canDriver = std::make_shared<isobus::SocketCANInterface>("vcan0");
#elif defined(ISOBUS_WINDOWSPCANBASIC_AVAILABLE)
	canDriver = std::make_shared<isobus::PCANBasicWindowsPlugin>(PCAN_USBBUS1);
#elif defined(ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE)
	canDriver = std::make_shared<isobus::InnoMakerUSB2CANWindowsPlugin>(0); // CAN0
#elif defined(ISOBUS_MACCANPCAN_AVAILABLE)
	canDriver = std::make_shared<isobus::MacCANPCANPlugin>(PCAN_USBBUS1);
#elif defined(ISOBUS_SYS_TEC_AVAILABLE)
	canDriver = std::make_shared<isobus::SysTecWindowsPlugin>();
#endif
	if (nullptr == canDriver)
	{
		std::cout << "Unable to find a CAN driver. Please make sure you have one of the above drivers installed with the library." << std::endl;
		std::cout << "If you want to use a different driver, please add it to the list above." << std::endl;
		return -1;
	}

	isobus::CANHardwareInterface::set_number_of_can_channels(1);
	isobus::CANHardwareInterface::assign_can_channel_frame_handler(CAN_CHANNEL, canDriver);

	if ((!isobus::CANHardwareInterface::start()) || (!canDriver->get_is_valid()))
	{
		std::cout << "Failed to start hardware interface. The CAN driver might be invalid." << std::endl;
		return -2;
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(250));

    //example data
    std::array<std::uint8_t, 8> data {0, 1, 2, 3, 4, 5, 6, 7};

    //construct CBFF
    auto classic_raw_frame = isobus::CANMessageFrame();
    classic_raw_frame.channel = CAN_CHANNEL;
    std::copy(data.begin(), data.end(), classic_raw_frame.data);
    classic_raw_frame.dataLength = data.size();
    classic_raw_frame.identifier = 0x123 & 0x1FFFFFFF;
    classic_raw_frame.isExtendedFrame = false;

    //construct CEFF
    auto extended_raw_frame = isobus::CANMessageFrame();
    extended_raw_frame.channel = CAN_CHANNEL;
    std::copy(data.begin(), data.end(), extended_raw_frame.data);
    extended_raw_frame.dataLength = data.size();
    extended_raw_frame.identifier = 0x12345678 & 0x1FFFFFFF;
    extended_raw_frame.isExtendedFrame = true;

    //construct j1939 identifier
    std::array<std::uint8_t, 11> big_data {0, 1, 2, 3, 4, 5, 6, 7, 8 , 9, 10};
    auto j1939_src_addr = 0x80u;
    auto j1939_dst_addr = isobus::BROADCAST_CAN_ADDRESS;
    auto j1939_pgn = static_cast<uint32_t>(isobus::CANLibParameterGroupNumber::Any); 
    auto j1939_prio = static_cast<uint8_t>(isobus::CANIdentifier::CANPriority::PriorityDefault6);

    //prepare ISOBUS CFs
    auto ownName = isobus::NAME();
    ownName.set_manufacturer_code(MANUFACTURER_ID);
    ownName.set_arbitrary_address_capable(true);
    ownName.set_industry_group(2);
    ownName.set_device_class(25); //Slurry/Manure Applicators
    ownName.set_function_instance(16); //TIM Client


    auto interalCf = isobus::InternalControlFunction::create(ownName, 0x80, CAN_CHANNEL);
    auto externalCf = isobus::ControlFunction::create(isobus::NAME(isobus::DEFAULT_NAME), 0x90, CAN_CHANNEL);
    auto defaultPrio = isobus::CANIdentifier::CANPriority::PriorityDefault6;

    while (1) {
        std::string sentMsg = isobus::CANHardwareInterface::transmit_can_frame(classic_raw_frame)
        ? "success" : "fail";
        std::cout << "Sent classic can message! " << sentMsg << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

        sentMsg = isobus::CANHardwareInterface::transmit_can_frame(extended_raw_frame)
        ? "success" : "fail";
        std::cout << "Sent extended can message! " << sentMsg << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

        sentMsg = isobus::CANNetworkManager::CANNetwork.send_can_message_raw(
            CAN_CHANNEL,
            j1939_src_addr,
            j1939_dst_addr,
            j1939_pgn,
            j1939_prio,
            data.data(),
            data.size()
        ) ? "success" : "fail";
        std::cout << "Sent 'j1939' 8 byte message! " << sentMsg << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

        isobus::CANNetworkManager::CANNetwork.send_can_message(
            j1939_pgn,
            data.data(),
            data.size(),
            interalCf,
            externalCf,
            defaultPrio,
            nullptr,
            nullptr,
            nullptr
        ) ? "success" : "fail";
        std::cout << "Sent 'ISOBUS' 8 byte message! " << sentMsg << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));

        isobus::CANNetworkManager::CANNetwork.send_can_message(
            j1939_pgn,
            big_data.data(),
            big_data.size(),
            interalCf,
            externalCf,
            defaultPrio,
            nullptr,
            nullptr,
            nullptr
        ) ? "success" : "fail";
        std::cout << "Sent 'ISOBUS' 11 byte message! " << sentMsg << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(DELAY_MS));
    }
}