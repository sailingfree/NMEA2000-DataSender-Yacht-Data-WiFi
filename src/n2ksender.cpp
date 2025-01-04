// send various n2k messages
#define ESP32_CAN_TX_PIN GPIO_NUM_5  // Set CAN TX port to 5 (Caution!!! Pin 2 before)
#define ESP32_CAN_RX_PIN GPIO_NUM_4  // Set CAN RX port to 4

#include <NMEA2000.h>
#include <NMEA2000_CAN.h>  // This will automatically choose right CAN library and create suitable NMEA2000 object
#include <N2kMessages.h>
#include <N2kMsg.h>
#include <N2kMaretron.h>
#include <Arduino.h>
#include <Seasmart.h>
#include <N2kDeviceList.h>
#include <n2ksender.h>
#include <N2ktoYD.h>
#include <GwShell.h>
#include <GwPrefs.h>


// List of n2k devices for the device scanner
tN2kDeviceList *pN2kDeviceList;

// Forward declarations
void OnN2kOpen();
void CheckLoopTime();
using tN2kSendFunction = void (*)();
void addCommands();

// Structure for holding message sending information
struct tN2kSendMessage {
    tN2kSendFunction SendFunction;
    const char *const Description;
    tN2kSyncScheduler Scheduler;

    tN2kSendMessage(tN2kSendFunction _SendFunction, const char *const _Description, uint32_t /* _NextTime */
                    ,
                    uint32_t _Period, uint32_t _Offset, bool _Enabled) : SendFunction(_SendFunction),
                                                                         Description(_Description),
                                                                         Scheduler(_Enabled, _Period, _Offset) {}
    void Enable(bool state);
};

extern tN2kSendMessage N2kSendMessages[];
extern size_t nN2kSendMessages;

static unsigned long N2kMsgSentCount = 0;
static unsigned long N2kMsgFailCount = 0;

// Toggles for sending and disagnostics
// These are saved in the preferences
static bool enableCAN = false;
static bool enableYD = false;

// Other toggles are reset at startup
static bool ShowSentMessages = false;
static bool ShowStatistics = false;
static bool EnableForward = false;

static tN2kScheduler NextStatsTime;
static unsigned long StatisticsPeriod = 2000;

// Convert the list of message enable flags into a binary format
// For saving into the preferences
// This is done to cut down the number of keys needed.
// there afewer than 32 messages so we use an unsigned 32 bit value
uint32_t enableMap;

// Create the map and save it in enableMap
void msgToMap() {
    enableMap = 0;
    for (size_t i = 0; i < nN2kSendMessages; i++) {
        enableMap |= N2kSendMessages[i].Scheduler.IsEnabled() << i;
    }
}

// Use the map to set the enable flags in the messages
void mapToMsg() {
    for (size_t i = 0; i < nN2kSendMessages; i++) {
        N2kSendMessages[i].Enable(enableMap & 1 <<i);
    }
}

// read the map from the preferences and upadte the messages
void readMap() {
    enableMap = GwGetInt(N2KMAP);
    mapToMsg();
}

// save the map as a preference
void saveMap() {
    msgToMap();
    GwSetInt(N2KMAP, enableMap);
}

// Handle some other gw messages from other gw nodes
static void handle_gw_msgs(const tN2kMsg &N2kMsg) {
    ulong PGN = N2kMsg.PGN;
    switch (PGN) {
        case 127508L:  // Battery
        case 127488:   // Engine rapid
        case 130306:   // Wind
        case 129026:   // SOG/COG
        case 128267:   // Depth
        case 129029:   // GNSS
        case 129540:   // GNSS sats in view
        case 130310:   // Outside environment
        case 130312:   // Temperature
        case 130313:   // Humidity
        case 130314:   // Pressure
        default:
            Serial.printf("Handling PGN %ld\n", PGN);
            break;
    }
}

// Set the information for other bus devices, which messages we support
static const unsigned long TransmitMessages[] PROGMEM = {127489L,  // Engine dynamic
                                                         127488L,  // Engine fast dynamic
                                                         127250,   // Vessel heading
                                                         130577,   // Direction data
                                                         130310,   // Outside environmental
                                                         0};
static const unsigned long ReceiveMessages[] PROGMEM = {/*126992L,*/  // System time
                                                        127250L,      // Heading
                                                        127258L,      // Magnetic variation
                                                        128259UL,     // Boat speed
                                                        128267UL,     // Depth
                                                        129025UL,     // Position
                                                        129026L,      // COG and SOG
                                                        129029L,      // GNSS
                                                        130306L,      // Wind
                                                        128275UL,     // Log
                                                        127245UL,     // Rudder
                                                        0};


void setupN2ksender() {
    // Add the n2k commands to the shell
    addCommands();

    // Add sender specific preference keys
    GwAddReg(YDENABLE);
    GwAddReg(CANENBALE);
    GwAddReg(N2KMAP);

    // Get the values of the toggles from the preferences
    enableYD = GwGetInt(YDENABLE);
    enableCAN = GwGetInt(CANENBALE);

    // Read the map of enabled messages and set their send state
    readMap();

    // Reserve enough buffer for sending all messages.
    // This does not work on small memory devices like Uno or Mega
    // This version should work with smaller buffer, since message sending has been offsetted.
    NMEA2000.SetN2kCANSendFrameBufSize(250);
    // Set Product information
    NMEA2000.SetProductInformation("200",                                   // Manufacturer's Model serial code
                                   2047,                                    // Manufacturer's product code
                                   "Naiad NMEA2000 and YD message sender",  // Manufacturer's Model ID
                                   "1.0.0 Oct 2024",                        // Manufacturer's Software version code
                                   "1.1.0"                                  // Manufacturer's Model version
    );
    // Set device information
    NMEA2000.SetDeviceInformation(1,    // Unique number. Use e.g. Serial number.
                                  132,  // Device function=Analog to NMEA 2000 Gateway. See codes on https://web.archive.org/web/20190531120557/https://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                  25,   // Device class=Inter/Intranetwork Device. See codes on  https://web.archive.org/web/20190531120557/https://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                  2047  // Just choosen free from code list on https://web.archive.org/web/20190529161431/http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
    );
    // Uncomment 3 rows below to see, what device will send to bus
    NMEA2000.SetForwardStream(&Serial);             // PC output to default serial port
    NMEA2000.SetForwardType(tNMEA2000::fwdt_Text);  // Show in clear text. Leave uncommented for default Actisense format.
    NMEA2000.SetForwardOwnMessages(false);          // Do not print own messages.

    // We act as real node on bus. Some devices does not show messages, if they can not request information.
    NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode, 22);
    // NMEA2000.SetDebugMode(tNMEA2000::dm_ClearText); // Uncomment this, so you can test code without CAN bus chips on Arduino Mega
    NMEA2000.EnableForward(EnableForward);  // Disable all msg forwarding to USB (=Serial)
    NMEA2000.SetOnOpen(OnN2kOpen);
    NMEA2000.SetMsgHandler(handle_gw_msgs);
    NMEA2000.ExtendTransmitMessages(TransmitMessages);
    NMEA2000.ExtendReceiveMessages(ReceiveMessages);
    NMEA2000.Open();
}

// *****************************************************************************
void SendN2kMsg(const tN2kMsg &N2kMsg) {
    if (enableYD) {
        GwSendYD(N2kMsg);
    }

    if (enableCAN) {
        if (NMEA2000.SendMsg(N2kMsg)) {
            N2kMsgSentCount++;
        } else {
            N2kMsgFailCount++;
        }
    }

    if (ShowSentMessages) N2kMsg.Print(&Serial);
}

// *****************************************************************************
// Function check is it time to send message. If it is, message will be sent and
// next send time will be updated.
// Function always returns next time it should be handled.
int64_t CheckSendMessage(tN2kSendMessage &N2kSendMessage) {
    if (N2kSendMessage.Scheduler.IsDisabled()) return N2kSendMessage.Scheduler.GetNextTime();

    if (N2kSendMessage.Scheduler.IsTime()) {
        N2kSendMessage.Scheduler.UpdateNextTime();
        N2kSendMessage.SendFunction();
    }

    return N2kSendMessage.Scheduler.GetNextTime();
}

// *****************************************************************************
// Function send enabled messages from tN2kSendMessage structure according to their
// period+offset.
void SendN2kMessages() {
    static uint64_t NextSend = 0;
    uint64_t Now = N2kMillis64();

    if (NextSend < Now) {
        uint64_t NextMsgSend;
        NextSend = Now + 2000;
        for (size_t i = 0; i < nN2kSendMessages; i++) {
            NextMsgSend = CheckSendMessage(N2kSendMessages[i]);
            if (NextMsgSend < NextSend) NextSend = NextMsgSend;
        }
    }
}

void handleN2kMessages() {
    if (enableCAN || enableYD) SendN2kMessages();
    NMEA2000.ParseMessages();
    if (ShowStatistics) {
        if (NextStatsTime.IsTime()) {
            NextStatsTime.FromNow(StatisticsPeriod);
            char buf[200];
            snprintf(buf, 200, "\nMessages sent:%lu, sent failed:%lu\n", N2kMsgSentCount, N2kMsgFailCount);
            Serial.print(buf);
        }

        CheckLoopTime();
    }
}

// *****************************************************************************
double ReadCabinTemp() {
    return CToKelvin(21.11 + random(-5, 5));  // Read here the true temperature e.g. from analog input
}

// *****************************************************************************
double ReadWaterTemp() {
    return CToKelvin(15.5 + random(-3, 3));  // Read here the true temperature e.g. from analog input
}

// *****************************************************************************
void SendN2kAttitude() {
    tN2kMsg N2kMsg;
    SetN2kAttitude(N2kMsg, 1, DegToRad(-3.1), DegToRad(2.4), DegToRad(-7.8));
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kSystemTime() {
    tN2kMsg N2kMsg;
    SetN2kSystemTime(N2kMsg, 1, 17555, 62000);
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kLocalOffset() {
    tN2kMsg N2kMsg;
    SetN2kLocalOffset(N2kMsg, 17555, 62000, 120);
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kRudder() {
    tN2kMsg N2kMsg;
    SetN2kRudder(N2kMsg, DegToRad(5), 1, N2kRDO_MoveToStarboard, DegToRad(-5));
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kGNSS() {
    tN2kMsg N2kMsg;
    static double lat = 50.676837,
                  lon = -2.092669;

    uint16_t daysSince1970;
    double secondsSinceMidnight;

    time_t now = time(NULL);
    struct tm *tm;
    tm = gmtime(&now);

    daysSince1970 = now / (24 * 60 * 60);
    secondsSinceMidnight = tm->tm_sec + tm->tm_min * 60 + tm->tm_hour * 60 * 60;

    SetN2kGNSS(N2kMsg, 1, daysSince1970, secondsSinceMidnight,
               lat,
               lon,
               10.5, N2kGNSSt_GPS, N2kGNSSm_GNSSfix, 12, 0.8, 0.5, 15, 1, N2kGNSSt_GPS, 15, 2);

    // 0.00001 degrees is 1.112m
    // 2.57222 m/s = 5knots
    // so adjust by 0.00002 to get a reasonable speed
    lat += 0.00002;
    lon += 0.00001;
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kGNSSDOPData() {
    tN2kMsg N2kMsg;
    SetN2kGNSSDOPData(N2kMsg, 1, N2kGNSSdm_Auto, N2kGNSSdm_Auto, 1.2 + random(-1, 1), -0.8, N2kDoubleNA);
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kGNSSSatsInView() {
    tN2kMsg N2kMsg;
    // Init message
    SetN2kGNSSSatellitesInView(N2kMsg, 0xff, N2kDD072_RangeResidualsWereUsedToCalculateData);
    // Add satellites

    // This one is always fixed for testing
    tSatelliteInfo SatelliteInfo;
    SatelliteInfo.PRN = 21;
    SatelliteInfo.Elevation = DegToRad(44.94);
    SatelliteInfo.Azimuth = DegToRad(162.67);
    SatelliteInfo.SNR = 47.6;
    SatelliteInfo.RangeResiduals = 7951.32;
    SatelliteInfo.UsageStatus = N2kDD124_UsedInSolutionWithoutDifferentialCorrections;
    AppendSatelliteInfo(N2kMsg, SatelliteInfo);

    // These have varying values to test displays
    for (int i = 0; i < random(0, 9); i++) {
        SatelliteInfo.PRN = random(1, 60);
        SatelliteInfo.Elevation = DegToRad(random(10, 80));
        SatelliteInfo.Azimuth = DegToRad(random(0, 360));
        SatelliteInfo.SNR = random(40, 50);
        SatelliteInfo.RangeResiduals = 10105.22;
        SatelliteInfo.UsageStatus = N2kDD124_UsedInSolutionWithoutDifferentialCorrections;
        AppendSatelliteInfo(N2kMsg, SatelliteInfo);
    }
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendUserDatumData() {
    tN2kMsg N2kMsg;
    N2kMsg.SetPGN(129045L);
    N2kMsg.Priority = 6;
    N2kMsg.Add4ByteDouble(3.25, 1e-2);         // Delta X
    N2kMsg.Add4ByteDouble(-3.19, 1e-2);        // Delta Y
    N2kMsg.Add4ByteDouble(1.1, 1e-2);          // Delta Z
    N2kMsg.AddFloat(DegToRad(0.123));          // Rotation in X
    N2kMsg.AddFloat(DegToRad(-0.0123));        // Rotation in Y
    N2kMsg.AddFloat(DegToRad(0.00123));        // Rotation in Z
    N2kMsg.AddFloat(1.001);                    // Scale
    N2kMsg.Add4ByteDouble(N2kDoubleNA, 1e-7);  // Ellipsoid Semi-major Axis
    N2kMsg.AddFloat(15.23456);                 // Ellipsoid Flattening Inverse
    N2kMsg.Add4ByteUInt(0xffffffff);
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendTideStationData() {
    tN2kMsg N2kMsg;
    N2kMsg.SetPGN(130320L);
    N2kMsg.Priority = 6;
    N2kMsg.AddByte(0xc0);                       // Mode+Tide Tendency+reserved
    N2kMsg.Add2ByteUInt(18498);                 // Measurement Date
    N2kMsg.Add4ByteUDouble(43210, 0.0001);      // Measurement Time
    N2kMsg.Add4ByteDouble(60.436616000, 1e-7);  // Station location, latitude
    N2kMsg.Add4ByteDouble(22.237820000, 1e-7);  // Station location, longitude
    N2kMsg.Add2ByteDouble(3.25, 1e-3);          // Tide level
    N2kMsg.Add2ByteUDouble(1.56, 1e-2);         // Tide level standard deviation
    N2kMsg.AddVarStr("00022");
    N2kMsg.AddVarStr("Test");
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendSalinity() {
    tN2kMsg N2kMsg;
    N2kMsg.SetPGN(130321L);
    N2kMsg.Priority = 6;
    N2kMsg.AddByte(0xf1);                       // Mode+reserved
    N2kMsg.Add2ByteUInt(18498);                 // Measurement Date
    N2kMsg.Add4ByteUDouble(43210, 0.0001);      // Measurement Time
    N2kMsg.Add4ByteDouble(60.436616000, 1e-7);  // Station location, latitude
    N2kMsg.Add4ByteDouble(22.237820000, 1e-7);  // Station location, longitude
                                                //  N2kMsg.Add4ByteDouble(N2kDoubleNA,1e-7); // Station location, latitude
                                                //  N2kMsg.Add4ByteDouble(N2kDoubleNA,1e-7); // Station location, longitude
    N2kMsg.AddFloat(34.56789);                  // Salinity
    N2kMsg.Add2ByteUDouble(293.15, 0.01);
    N2kMsg.AddVarStr("00022");
    N2kMsg.AddVarStr("Test");
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendHeave() {
    tN2kMsg N2kMsg;
    SetN2kHeave(N2kMsg, 0xff, 5.3, 2.1, N2kDD374_UserDefined);
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendRouteInfo() {
    tN2kMsg N2kMsg;
    SetN2kRouteWPInfo(N2kMsg, 0, 1, 7, N2kdir_forward, "Back to home");
    AppendN2kPGN129285(N2kMsg, 1, "Start point", 60.0, 21.0);
    AppendN2kPGN129285(N2kMsg, 2, "Turn before rock", 60.01, 21.01);
    AppendN2kPGN129285(N2kMsg, 3, "Home", 60.02, 21.02);
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kProductInformation() {
    NMEA2000.SendProductInformation();
    N2kMsgSentCount++;
}

// *****************************************************************************
void SendN2kIsoAddressClaim() {
    // Note that sometime NMEA Reader gets grazy, when ISO Address claim will be sent periodically.
    // If that happens, reopen NMEA Reader.
    NMEA2000.SendIsoAddressClaim();
    N2kMsgSentCount++;
}

// *****************************************************************************
void SendN2kPressure() {
    tN2kMsg N2kMsg;
    SetN2kPressure(N2kMsg, 0, 2, N2kps_Atmospheric, mBarToPascal(1024) + random(-5, 5));
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kSetPressure() {
    tN2kMsg N2kMsg;
    SetN2kSetPressure(N2kMsg, 0, 2, N2kps_CompressedAir, 1255);
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kHumidity() {
    tN2kMsg N2kMsg;
    SetN2kHumidity(N2kMsg, 0xff, 1, N2khs_InsideHumidity, 41.8);  //,43);
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kTemperatureExt() {
    tN2kMsg N2kMsg;
    SetN2kTemperatureExt(N2kMsg, 1, 1, N2kts_MainCabinTemperature, ReadCabinTemp(), CToKelvin(21.6 + random(-5, 5)));
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kTemperature() {
    tN2kMsg N2kMsg;
    SetN2kTemperature(N2kMsg, 1, 1, N2kts_MainCabinTemperature, ReadCabinTemp(), CToKelvin(21.6 + random(-5, 5)));
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendEnvironmentalParameters() {
    tN2kMsg N2kMsg;
    SetN2kEnvironmentalParameters(N2kMsg, 1, N2kts_MainCabinTemperature, ReadCabinTemp(), N2khs_InsideHumidity, 55, mBarToPascal(1013.5));
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendOutsideEnvironmentalParameters() {
    tN2kMsg N2kMsg;
    SetN2kOutsideEnvironmentalParameters(N2kMsg, 1, ReadWaterTemp(), CToKelvin(25.3), mBarToPascal(1013.5));
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kMaretronTempHR() {
    tN2kMsg N2kMsg;
    SetN2kMaretronTempHR(N2kMsg, 0xff, 1, N2kts_MainCabinTemperature, CToKelvin(24.7));
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kChargerStatus() {
    tN2kMsg N2kMsg;
    SetN2kChargerStatus(N2kMsg, 1, 2, N2kCS_Absorption, N2kCM_Primary, N2kOnOff_On, N2kOnOff_On, 180);
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kDCBatStatus1() {
    tN2kMsg N2kMsg;
    SetN2kDCBatStatus(N2kMsg, 1, 12.72 + random(-1, 1));
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kDCBatStatus2() {
    tN2kMsg N2kMsg;
    static unsigned char sid = 0;
    //    SetN2kDCBatStatus(N2kMsg, 0, 12.45, 5.08, CToKelvin(27.15));
    SetN2kPGN127508(N2kMsg, 0, 13.8 + random(-1, 1), 0.95 + random(-1, 1), N2kDoubleNA, sid++);
    if (sid > 252) sid = 0;
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kBatConf() {
    tN2kMsg N2kMsg;
    SetN2kBatConf(N2kMsg, 1, N2kDCbt_AGM, N2kDCES_Yes, N2kDCbnv_12v, N2kDCbc_LeadAcid, AhToCoulomb(410), 95, 1.26, 97);
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kDCStatus() {
    tN2kMsg N2kMsg;
    SetN2kDCStatus(N2kMsg, 1, 1, N2kDCt_Alternator, 86, 91, 1420, 0.215, N2kDoubleNA);
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kTransmissionParameters() {
    tN2kMsg N2kMsg;
    SetN2kTransmissionParameters(N2kMsg, 0, N2kTG_Forward, 750000, CToKelvin(65.5), true, false, true);
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kAISClassAPosition() {
    tN2kMsg N2kMsg;
    SetN2kAISClassAPosition(N2kMsg, 1, tN2kAISRepeat::N2kaisr_First, 123456789, 26.396, -80.075, 1, 1, 1, 20, 20, N2kaischannel_A_VDL_reception, 30, 0, tN2kAISNavStatus::N2kaisns_At_Anchor);
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kBinaryStatus1() {
    tN2kMsg N2kMsg;
    SetN2kBinaryStatus(N2kMsg, 2, N2kOnOff_On, N2kOnOff_Unavailable, N2kOnOff_Off);
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kBinaryStatus2() {
    tN2kMsg N2kMsg;
    tN2kBinaryStatus SwitchBoard;
    N2kResetBinaryStatus(SwitchBoard);
    N2kSetStatusBinaryOnStatus(SwitchBoard, N2kOnOff_On, 7);
    SetN2kBinaryStatus(N2kMsg, 3, SwitchBoard);
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kEngineTripParameters() {
    tN2kMsg N2kMsg;
    SetN2kEngineTripParameters(N2kMsg, 0, 237, 3.6, 4.5, 4.4);
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kEngineDynamicParam() {
    tN2kMsg N2kMsg;
    tN2kEngineDiscreteStatus1 Status1;
    Status1.Bits.LowOilLevel = 1;
    Status1.Bits.OverTemperature = 1;
    SetN2kEngineDynamicParam(N2kMsg, 0, 656000, CToKelvin(86.3), CToKelvin(82.1), 14.21, 5.67, hToSeconds(2137.55), N2kDoubleNA, N2kDoubleNA, N2kInt8NA, N2kInt8NA, Status1);
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kEngineRapid() {
    tN2kMsg N2kMsg;
    SetN2kEngineParamRapid(N2kMsg, 0, 2210 + random(-50, 50));  //,820000,48);
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kCOGSOGRapid() {
    tN2kMsg N2kMsg;
    SetN2kCOGSOGRapid(N2kMsg, 1, N2khr_true, DegToRad(115.6 + random(-10, 10)), 5.5 + random(-2, 2));
    SendN2kMsg(N2kMsg);
}

// *****************************************************************************
void SendN2kMagneticHeading() {
    static double Heading = 227.5;
    tN2kMsg N2kMsg;
    SetN2kMagneticHeading(N2kMsg, 0, DegToRad(Heading), DegToRad(-3.0 + random(-5, 5)), DegToRad(5.5 + random(-5, 5)));
    SendN2kMsg(N2kMsg);
}

// PGN 130306 "Wind Data"
void SendN2kWind() {
    tN2kMsg N2kMsg;
    SetN2kWindSpeed(N2kMsg, 0, 10.8033 + random(-5, 5), DegToRad(110 + random(-5, 5)), N2kWind_Apparent);
    SendN2kMsg(N2kMsg);
}

// PGN 128267 Depth
void SendN2kDepth() {
    tN2kMsg N2kMsg;
    SetN2kWaterDepth(N2kMsg, 0, 12.3 + random(-5, 5), 0.0, N2kDoubleNA);
    SendN2kMsg(N2kMsg);
}

// We add 300 ms as default for each offset to avoid failed sending at start.
// Message sending is synchronized to open. After open there is 250 ms address claiming time when
// message sending fails.
#define AddSendPGN(fn, NextSend, Period, Offset, Enabled) {fn, #fn, NextSend, Period, Offset + 300, Enabled}

tN2kSendMessage N2kSendMessages[] = {
    AddSendPGN(SendN2kIsoAddressClaim, 0, 5000, 0, false)  // 60928 Not periodic
    ,
    AddSendPGN(SendN2kSystemTime, 0, 1000, 0, true)  // 126992
    ,
    AddSendPGN(SendN2kProductInformation, 0, 5000, 60, false)  // 126996 (20) Not periodic
                                                               // ,AddSendPGN(SendN2kRudder,0,100,0,true) // 127245
    ,
    AddSendPGN(SendN2kMagneticHeading, 0, 100, 1, true)  // 127250
                                                         // ,AddSendPGN(SendN2kAttitude,0,1000,20,true) // 127257
    ,
    AddSendPGN(SendN2kEngineRapid, 0, 100, 2, true)  // 127488
    ,
    AddSendPGN(SendN2kEngineDynamicParam, 0, 500, 21, true)  // 127489 (4)
                                                             // ,AddSendPGN(SendN2kTransmissionParameters,0,100,3,true) // 127493
                                                             // ,AddSendPGN(SendN2kEngineTripParameters,0,1000,25,true) // 127497 (2)
                                                             //  ,AddSendPGN(SendN2kBinaryStatus1,0,2500,27,true) // 127501
                                                             //  ,AddSendPGN(SendN2kBinaryStatus2,0,2500,28,true) // 127501
                                                             //  ,AddSendPGN(SendN2kChargerStatus,0,1500,29,true) // 127507
    ,
    AddSendPGN(SendN2kDCBatStatus1, 0, 1500, 30, true)  // 127508
    ,
    AddSendPGN(SendN2kDCBatStatus2, 0, 1500, 31, true)  // 127508
                                                        //  ,AddSendPGN(SendN2kDCStatus,0,1500,32,true) // 127506 (2)
    ,
    AddSendPGN(SendN2kBatConf, 0, 5000, 34, false)  // 127513 Not periodic
    ,
    AddSendPGN(SendN2kCOGSOGRapid, 0, 250, 0, true)  // 129026
    ,
    AddSendPGN(SendN2kGNSS, 0, 1000, 75, true)  // 129029 (7)
    ,
    AddSendPGN(SendN2kLocalOffset, 0, 5000, 36, false)  // 129033 Not periodic
                                                        // ,AddSendPGN(SendN2kAISClassAPosition,0,5000,80,false) // 129038 (4) Not periodic
                                                        // ,AddSendPGN(SendUserDatumData,0,5000,82,false) // 129045 (6) Not periodic
    ,
    AddSendPGN(SendN2kGNSSDOPData, 0, 1000, 37, true)  // 129539
    ,
    AddSendPGN(SendN2kGNSSSatsInView, 0, 1000, 37, true)  // 129540 (max. 32)
    ,
    AddSendPGN(SendOutsideEnvironmentalParameters, 0, 500, 38, true)  // 130310
    ,
    AddSendPGN(SendEnvironmentalParameters, 0, 500, 39, true)  // 130311
    ,
    AddSendPGN(SendN2kTemperature, 0, 2000, 40, true)  // 130312
    ,
    AddSendPGN(SendN2kHumidity, 0, 2000, 41, true)  // 130313
    ,
    AddSendPGN(SendN2kPressure, 0, 2000, 42, true)  // 130314
                                                    //  ,AddSendPGN(SendN2kSetPressure,0,5000,43,true) // 130315 Not periodic
                                                    //  ,AddSendPGN(SendN2kTemperatureExt,0,2500,44,true) // 130316 Not periodic
    ,
    AddSendPGN(SendTideStationData, 0, 1000, 86, false)  // 130320 (8)
    ,
    AddSendPGN(SendSalinity, 0, 1000, 90, false)  // 130321 (8)
                                                  //  ,AddSendPGN(SendHeave,0,100,94,true) // 127252
                                                  //  ,AddSendPGN(SendRouteInfo,0,5000,94,true) // 129285 Not periodic
    ,
    AddSendPGN(SendN2kMaretronTempHR, 0, 2000, 45, false)  // 130823 (2)
    ,
    AddSendPGN(SendN2kWind, 0, 250, 46, true)  // PGN 130306 "Wind Data"
    ,
    AddSendPGN(SendN2kDepth, 0, 250, 46, true)  // PGN 128267 "Depth Data"
};

size_t nN2kSendMessages = sizeof(N2kSendMessages) / sizeof(tN2kSendMessage);

// *****************************************************************************
// Call back for NMEA2000 open. This will be called, when library starts bus communication.
// See NMEA2000.SetOnOpen(OnN2kOpen); on setup()
// We initialize here all messages next sending time. Since we use tN2kSyncScheduler all messages
// send offset will be synchronized to libary.
void OnN2kOpen() {
    for (size_t i = 0; i < nN2kSendMessages; i++) {
        if (N2kSendMessages[i].Scheduler.IsEnabled()) {
            N2kSendMessages[i].Scheduler.UpdateNextTime();
        }
    }
}

// *****************************************************************************
// Command handling functions. These are environment functions for example not
// for how to use library.

// *****************************************************************************
void CheckLoopTime() {
#define LoopTimePeriod 1000
    static unsigned long NextCheck = millis() + LoopTimePeriod;
    static unsigned long AvgCount = 0;
    static float AvgSum = 0;
    static unsigned long MaxLoopTime = 0;
    static unsigned long StartTime = micros();

    unsigned long UsedTime = micros() - StartTime;
    if (UsedTime > MaxLoopTime) MaxLoopTime = UsedTime;
    AvgCount++;
    AvgSum += UsedTime;

    if (NextCheck < millis()) {
        NextCheck = millis() + LoopTimePeriod;
        Serial.print("- Loop times max:");
        Serial.print(MaxLoopTime);
        Serial.print(" us, avg:");
        Serial.print(AvgSum / AvgCount);
        Serial.println(" us");
        MaxLoopTime = 0;
        AvgSum = 0;
        AvgCount = 0;
    }

    StartTime = micros();
}

// *****************************************************************************
void tN2kSendMessage::Enable(bool state) {
    if (Scheduler.IsEnabled() != state) {
        if (state) {
            Scheduler.UpdateNextTime();
        } else {
            Scheduler.Disable();
        }
    }
}

// *****************************************************************************
int ToggleStats(int argc, char ** argv) {
    ShowStatistics = !ShowStatistics;
    if (ShowStatistics) {
        NextStatsTime.FromNow(StatisticsPeriod);
    } else {
        NextStatsTime.Disable();
    }
    return 0;
}

// *****************************************************************************
int ToggleSending(int argc, char ** argv) {
    enableCAN = !enableCAN;
    GwSetInt(CANENBALE, enableCAN);
    Serial.print("Message sending ");
    Serial.println(enableCAN ? "enabled" : "disabled");
    return 0;
}

// *****************************************************************************
int ToggleShowSentMessages(int argc, char ** argv) {
    ShowSentMessages = !ShowSentMessages;
    return 0;
}

// *****************************************************************************
int ToggleForward(int argc, char ** argv) {
    EnableForward = !EnableForward;
    NMEA2000.EnableForward(EnableForward);

    Serial.print("Show bus messages ");
    Serial.println(EnableForward ? "enabled" : "disabled");
    return 0;
}

// *****************************************************************************
int ClearStatistics(int argc, char ** argv) {
    N2kMsgSentCount = 0;
    N2kMsgFailCount = 0;
    Serial.println("Message statistics cleared");
    return 0;
}

// *****************************************************************************
bool HasParam(int argc, char ** argv) {
    if (argc < 2) return false;
    return true;
}

// *****************************************************************************
template <typename T>
bool GetParam(int argc, char ** argv, T &Value, T Def = 0) {
    Value = Def;
    char *ParamEnd;
    Value = strtol(argv[argc], &ParamEnd, 10);
    return true;
}

// *****************************************************************************
int ToggleMsgSending(int argc, char ** argv) {
    if (!HasParam(argc, argv)) {
        Serial.println("Message index not defined");
        return 0;
    }

    size_t MsgIndex;
    if (!GetParam(1, argv, MsgIndex) || MsgIndex >= nN2kSendMessages) {
        Serial.println("Invalid message index");
        return 0;
    }

    N2kSendMessages[MsgIndex].Enable(!N2kSendMessages[MsgIndex].Scheduler.IsEnabled());
    Serial.print(N2kSendMessages[MsgIndex].Description);
    Serial.print("  ");
    Serial.println(N2kSendMessages[MsgIndex].Scheduler.IsEnabled() ? "enabled" : "disabled");
    // Update the saved map of enabled messages
    saveMap();
    return 0;
}

int ToggleYD(int argc, char ** argv) {
    enableYD = !enableYD;
    GwSetInt(YDENABLE, enableYD);
    Serial.printf("YD sending %s\n", enableYD ? "enabled" : "disabled");
    return 0;
}

// *****************************************************************************
int SendMessageNow(int argc, char ** argv) {
    if (!HasParam(argc, argv)) {
        Serial.println("Message index not defined");
        return 0;
    }

    size_t MsgIndex;
    if (!GetParam(1, argv, MsgIndex) || MsgIndex >= nN2kSendMessages) {
        Serial.println("Invalid message index");
        return 0;
    }

    N2kSendMessages[MsgIndex].SendFunction();
    Serial.print(N2kSendMessages[MsgIndex].Description);
    Serial.println("  sent");
   return 0;
}

// *****************************************************************************
int EnableAllMessages(int argc, char ** argv) {
    for (size_t i = 0; i < nN2kSendMessages; i++) {
        N2kSendMessages[i].Enable(true);
    }
    Serial.println("All messages enabled");
    return 0;
}

// *****************************************************************************
int DisableAllMessages(int argc, char ** argv) {
    Serial.printf("Disabling all messages\n");
    for (size_t i = 0; i < nN2kSendMessages; i++) {
        N2kSendMessages[i].Enable(false);
    }
    Serial.println("All messages disabled");
    return 0;
}

// *****************************************************************************
int ListMessages(int argc, char ** argv) {
    for (size_t i = 0; i < nN2kSendMessages; i++) {
        Serial.print("    ");
        Serial.print(i);
        Serial.print(" : ");
        Serial.print(N2kSendMessages[i].Scheduler.IsEnabled() ? "(enabled)  " : "(disabled) ");
        Serial.println(N2kSendMessages[i].Description);
    }
    return 0;
}

int printSummary(int argc, char ** argv) {
    Serial.printf("N2K %s YD %s\n", enableCAN == true ? "On" : "Off", enableYD ? "On" : "Off");
    return 0;
}

//*****************************************************************************
void PrintUlongList(const char *prefix, const unsigned long *List, Stream &stream) {
    uint8_t i;
    if (List != 0) {
        stream.printf(prefix);
        for (i = 0; List[i] != 0; i++) {
            if (i > 0) stream.print(", ");
            stream.printf("%lud", List[i]);
        }
        stream.println();
    }
}

//*****************************************************************************
void PrintText(const char *Text, bool AddLineFeed, Stream &stream) {
    if (Text != 0)
        stream.print(Text);
    if (AddLineFeed)
        stream.println();
}

//*****************************************************************************
void PrintDevice(const tNMEA2000::tDevice *pDevice, Stream &stream) {
    if (pDevice == 0) return;

    stream.printf("----------------------------------------------------------------------\n");
    stream.printf("%s\n", pDevice->GetModelID());
    stream.printf("  Source: %d\n", pDevice->GetSource());
    stream.printf("  Manufacturer code:        %d\n", pDevice->GetManufacturerCode());
    stream.printf("  Serial Code:              %s\n", pDevice->GetModelSerialCode());
    stream.printf("  Unique number:            %d\n", pDevice->GetUniqueNumber());
    stream.printf("  Software version:         %s\n", pDevice->GetSwCode());
    stream.printf("  Model version:            %s\n", pDevice->GetModelVersion());
    stream.printf("  Manufacturer Information: ");
    PrintText(pDevice->GetManufacturerInformation(), true, stream);
    stream.printf("  Installation description1: ");
    PrintText(pDevice->GetInstallationDescription1(), true, stream);
    stream.printf("  Installation description2: ");
    PrintText(pDevice->GetInstallationDescription2(), true, stream);
    PrintUlongList("  Transmit PGNs :", pDevice->GetTransmitPGNs(), stream);
    PrintUlongList("  Receive PGNs  :", pDevice->GetReceivePGNs(), stream);
    stream.printf("\n");
}

#define START_DELAY_IN_S 8
//*****************************************************************************
void ListDevices(Stream &stream, bool force = false) {
    static bool StartDelayDone = false;
    static int StartDelayCount = 0;
    static unsigned long NextStartDelay = 0;

    if (!StartDelayDone) {  // We let system first collect data to avoid printing all changes
        if (millis() > NextStartDelay) {
            if (StartDelayCount == 0) {
                stream.print("Reading device information from bus ");
                NextStartDelay = millis();
            }
            stream.print(".");
            NextStartDelay += 1000;
            StartDelayCount++;
            if (StartDelayCount > START_DELAY_IN_S) {
                StartDelayDone = true;
                stream.println();
            }
        }
        return;
    }
    if (!force && !pN2kDeviceList->ReadResetIsListUpdated()) return;

    // stream.println();
    //  stream.println("**********************************************************************");
    for (uint8_t i = 0; i < N2kMaxBusDevices; i++) {
        PrintDevice(pN2kDeviceList->FindDeviceBySource(i), stream);
    }
}

// add the commands to the shell
void addCommands() {
    Serial.printf("Adding N2k sender commands to shell\n");
    shell.addCommand(F("can \t\tToggle sending all N2k CAN messages"), ToggleSending);
    shell.addCommand(F("stats \t\tToggle displaying N2k stats"), ToggleStats);
    shell.addCommand(F("clear \tClear N2k statistics"), ClearStatistics);
    shell.addCommand(F("en \t\tEnable all messages"), EnableAllMessages);
    shell.addCommand(F("dis \t\tDisable all messages"), DisableAllMessages);
    shell.addCommand(F("msgx \t\tToggle the sending of message index x"), ToggleMsgSending);
    shell.addCommand(F("show \t\tToggle show sent messages"), ToggleShowSentMessages);
    shell.addCommand(F("bus \t\tToggle show other bus messages"), ToggleForward);
    shell.addCommand(F("listmsg \tList available messages"), ListMessages);
    shell.addCommand(F("sendx \t\Sends message x on request"), SendMessageNow);
    shell.addCommand(F("yd \t\tToggle sending YD broadcast UDP data"), ToggleYD);
    shell.addCommand(F("summary \tShow the state of the toggles"), printSummary);
}