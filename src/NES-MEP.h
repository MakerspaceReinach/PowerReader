#pragma once

#include <Arduino.h>
#include <sys/time.h>
#include <time.h>

#define METER_ENABLE_PIN   5 // GPIO  5 / pin 29
// #define RS3232_ENABLE_PIN 32 // GPIO 32 / pin 8   // EHorvat disabled this pin & function
#define LED_PIN   2          // GPIO  2 /      LEDindicator GPIO output to a LED


#define MaxMEPRequestLength 512
#define MaxMEPReplyLength   512
#define MaxMEPBuffer        20
#define MaxSendAttempts     5   //EHorvat (was 20)

enum NextActionEnum
{
  None,
  AlertET11,
  AlertET13,
  AlertET50,
  AlertEP39BT08,
  AlertEP39ET58,
  AlertClear
};

typedef struct {
  byte i;
  byte j;
} DState;

typedef struct {
  unsigned long  Millis = 0;
  byte           Request[MaxMEPRequestLength];
  unsigned long  RequestLength = 0;
  byte           SendAttempts = 0;
  byte           Reply[MaxMEPReplyLength];
  unsigned long  ReplyLength = 0;
  NextActionEnum NextAction = None;
  //DGF 14-DEC-2023 register specific param added (introduced for tier id)
  byte           RegisterSpecificParam = 0;
  //DGF
} MEPQueueStruct;

struct BT23
  {
    long Fwd_Act_Wh = 0;
    long Rev_Act_Wh = 0;
    long Fwd_React_Wh = 0;
    long Rev_React_Wh = 0;
  };

typedef struct 
{
  BT23 bt23_total;
  BT23 bt23_tiers[8];  // DGF 14-DeC-2023 tiers added
  long BT28_Fwd_W = 0;
  long BT28_Rev_W = 0;
  long BT28_Fwd_VA = 0;
  long BT28_Rev_VA = 0;
  long BT28_RMS_mA_L1 = 0;
  long BT28_RMS_mA_L2 = 0;
  long BT28_RMS_mA_L3 = 0;    
  long BT28_RMS_mV_L1 = 0;
  long BT28_RMS_mV_L2 = 0;
  long BT28_RMS_mV_L3 = 0; 
  long BT28_Pwr_Factor_L1 = 0;
  long BT28_Freq_mHz = 0;    
  long BT28_VA_L1L2L3 = 0;  
  long BT28_Pwr_Factor_L2 = 0;
  long BT28_Pwr_Factor_L3 = 0;
  long BT28_Fwd_W_L1 = 0;
  long BT28_Fwd_W_L2 = 0;
  long BT28_Fwd_W_L3 = 0;
  long BT28_Rev_W_L1 = 0;
  long BT28_Rev_W_L2 = 0;
  long BT28_Rev_W_L3 = 0;
  long BT28_Fwd_Avg_W = 0;
  long BT28_Rev_Avg_W = 0;
  long BT28_Fwd_Avg_W_L1 = 0;
  long BT28_Fwd_Avg_W_L2 = 0;
  long BT28_Fwd_Avg_W_L3 = 0;
  long BT28_Rev_Avg_W_L1 = 0;
  long BT28_Rev_Avg_W_L2 = 0;
  long BT28_Rev_Avg_W_L3 = 0;
  long BT28_ReactivePower_Q1 = 0;
  long BT28_ReactivePower_Q2 = 0;
  long BT28_ReactivePower_Q3 = 0;
  long BT28_ReactivePower_Q4 = 0;
  byte BT55_Current_Tier_Id = 0; // DGF 04-DEC-2023 current tier id (tariff) added
} ConsumptionDataStruct;

typedef struct {
  char BT01_Manufacturer[5] = "";
  char BT01_Model[9] = "";
  byte BT01_MainFirmwareVersionNumber = 0;
  byte BT01_FirmwareRevisionNumber = 0;
  byte BT01_MainHardwareVersionNumber = 0;
  byte BT01_HardwareRevisionNumber = 0;
  char ET03_UtilitySerialNumber[31] = "";
} MeterInfoStruct;

typedef struct {
  byte ET13_MBus_0 = 0;
  byte ET13_MBus_1 = 0;
  byte ET13_MBus_2 = 0;
  byte ET13_MBus_3 = 0;
  word ET13_MBus_4 = 0;
  byte ET13_MBus_6 = 0;
  byte ET13_MBus_7 = 0;
  char ET13_MBus_8[21] = "";
  byte ET13_MBus_28 = 0;
  byte ET13_MBus_29 = 0;
  byte ET13_MBus_30[32];
  word ET13_MBus_62 = 0;
  word ET13_MBus_64 = 0;
} ET13_MBusInfoStruct;

typedef struct {
  byte BT21_0 = 0;
  byte BT21_1 = 0;
  byte BT21_2 = 0;
  byte BT21_3 = 0;
  byte BT21_4 = 0;
  byte BT21_5 = 0;
  byte BT21_6 = 0;
  byte BT21_7 = 0;
  byte BT21_8 = 0;
  byte BT21_9 = 0;
  byte ET11_0 = 0;
  byte ET11_1 = 0;
  byte ET11_2 = 0;
  byte ET11_3 = 0;
  byte ET11_4 = 0;
  word ET11_5 = 0;
  word ET11_7 = 0;
  word ET11_9 = 0;
  byte ET11_11 = 0;
  byte ET11_12 = 0;
  byte ET11_13 = 0;
  byte ET11_18 = 0;
  byte ET11_19 = 0;
  byte ET11_20 = 0;
  byte ET11_21 = 0;
  byte ET11_22 = 0;
  byte ET11_23 = 0;
  byte ET11_24 = 0;
  byte ET11_25 = 0;
  byte ET11_26 = 0;
  byte ET11_27 = 0;
  word ET11_28 = 0;
  word ET11_30 = 0;
  ET13_MBusInfoStruct MBusInfo[4];
  byte ET13_MEP_0 = 0;
  byte ET13_MEP_1 = 0;
  byte ET13_MEP_2 = 0;
  byte ET13_MEP_3 = 0;
  word ET13_MEP_4 = 0;
  byte ET13_MEP_6 = 0;
  byte ET13_MEP_7 = 0; 
  byte ET13_MEP_8 = 0;
  byte ET13_MEP_9[57];
  byte ET50_0[30];
  byte ET50_30 = 0;
  word ET50_31[6];
  long ET50_43[6];
  byte ET50_67 = 0;  
} MeterConfigStruct;

void Digest(const byte *apduBytes, unsigned long apduSize, const byte *key, byte *digestValueOut, DState *pState, byte bEnd, int keyLen);
boolean PackageIsValid(byte *Package, unsigned long PackageLength);
boolean DigestIsValidOnResponsePackage(byte *RequestPackage, unsigned long RequestPackageLength, byte *ResponsePackage, unsigned long ResponsePackageLength);
String MaxMEPReplyLengthAsHex();
unsigned long GetPackageLength(byte *Package);
String bytesToHexString(byte *byteArray, unsigned long byteArrayLen, boolean SpaceDelimiter);
void hexStringToBytes(String hexString, byte *byteArray, unsigned long *byteArrayLen);
void queueRequest(String request,String key,MEPQueueStruct *MEPQueue,byte *MEPQueueNextIndex,NextActionEnum NextAction);
void queueResponseWithNoRequest(byte *Response,unsigned long ResponseLength,MEPQueueStruct *MEPQueue,byte *MEPQueueNextIndex);
void queueProcedureRequest(word procedureNo,String parameters,String key,MEPQueueStruct *MEPQueue,byte *MEPQueueNextIndex,NextActionEnum NextAction);
void dumpByteArray(const byte *byteArray, const unsigned long arraySize);
String ReplyPackageToMessage(MEPQueueStruct *MEPQueue,byte MEPQueueReplyIndex);
void HandleInvalidSequenceNumber(String key,MEPQueueStruct *MEPQueue,byte MEPQueueReplyIndex, byte *MEPQueueNextIndex);
void HandleNextAction(String key,MEPQueueStruct *MEPQueue,byte MEPQueueReplyIndex, byte *MEPQueueNextIndex);
void HandleAlertSequence(byte *Response, unsigned long *ResponseLength,String key,MEPQueueStruct *MEPQueue,byte *MEPQueueNextIndex);
void MEPEnable(boolean State);
// void RS3232Enable(boolean State);  // EHorvat disabled
void LEDindicator(boolean State);   // LEDindicator
void IncreaseMEPQueueIndex(byte *MEPQueueNextIndex);
String ReplyData2String(MEPQueueStruct *MEPQueue,byte MEPQueueReplyIndex, boolean UpdateDataStructures);
