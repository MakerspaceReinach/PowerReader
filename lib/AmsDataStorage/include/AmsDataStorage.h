#ifndef _AMSDATASTORAGE_H
#define _AMSDATASTORAGE_H
#include "Arduino.h"
#include "AmsData.h"
#include "RemoteDebug.h"
#include "Timezone.h"

struct DayDataPoints 
{
    uint8_t version;
    uint16_t hImport[24];
    time_t lastMeterReadTime;
    uint32_t activeImportTotal;  //DGF changed to total
    uint32_t activeExportTotal; //DGF changed to total
    uint16_t hExport[24];
    uint8_t accuracy;
}; // 114 bytes  //DGF was 113


/***********************************************************************************************************************************************************
 * Purpose: Data points for week plot
 * History: D.Geisenhoff 21-DEC-2023 Added
 * *********************************************************************************************************************************************************/
struct WeekDataPoints 
{
    uint8_t version;
    uint16_t wdImport[7];
    time_t lastMeterReadTime;
    uint32_t activeImportTotal; 
    uint32_t activeExportTotal; 
    uint16_t wdExport[7];
    uint8_t accuracy;
}; // 46 bytes


struct MonthDataPoints 
{
    uint8_t version;
    uint16_t dImport[31];
    time_t lastMeterReadTime;
    uint32_t activeImportTotal; //DGF changed to total
    uint32_t activeExportTotal; //DGF changed to total
    uint16_t dExport[31];
    uint8_t accuracy;
}; // 142 bytes

//EHorvat new minute plot.
struct MinuteDataPoints 
{
    uint8_t version;
    int32_t mWatt[150];
}; // 121 bytes
//EHorvat new minute plot END

//EHorvat new hour plot.
struct HourDataPoints {
    uint8_t version;
    int32_t mWatt[180];
}; // 121 bytes
//EHorvat new hour plot END

class AmsDataStorage 
{
public:
    AmsDataStorage(RemoteDebug*);
    void setTimezone(Timezone*);
    bool update(AmsData*);
    int32_t getMinuteWatt(uint8_t);        //EHorvat new minute plot.
    void updateMinute(int32_t);            //EHorvat new minute plot.
    int32_t getHourWatt(uint8_t);        //EHorvat new hour plot.
    void updateHour(int32_t);            //EHorvat new hour plot.
    uint32_t getHourImport(uint8_t);
    uint32_t getHourExport(uint8_t);
    uint32_t getWeekDayImport(uint8_t);  //DGF added for week plot
    uint32_t getWeekDayExport(uint8_t);  //DGF added for week plot
    uint32_t getDayImport(uint8_t);
    uint32_t getDayExport(uint8_t);
    bool load();
    bool save();

    DayDataPoints getDayData();
    bool setDayData(DayDataPoints&);
    WeekDataPoints getWeekData();         //DGF added for week plot
    bool setWeekData(WeekDataPoints&);    //DGF added for week plot
    MonthDataPoints getMonthData();
    bool setMonthData(MonthDataPoints&);

    uint8_t getDayAccuracy();
    void setDayAccuracy(uint8_t);
    uint8_t getWeekAccuracy();          //DGF added for week plot
    void setWeekAccuracy(uint8_t);      //DGF added for week plot
    void clearWeekPlot();               //DGF added for week plot
    uint8_t getMonthAccuracy();
    void setMonthAccuracy(uint8_t);

    bool isHappy();
    bool isDayHappy();
    bool isWeekHappy();  //DGF added for week plot
    bool isMonthHappy();

private:
    Timezone* tz;
    DayDataPoints day = {
        0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        10
    };

    //DGF week plot added
    WeekDataPoints week = 
    {
        0, 
        0, 0, 0, 0, 0, 0, 0,
        0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 
        10
    };
    //DGF

    MonthDataPoints month = {
        0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        10
    };
            //EHorvat new 6 Minute plot.
    MinuteDataPoints minute = {
        0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
         //EHorvat new hour plot now
    HourDataPoints hour = {
        0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    RemoteDebug* debugger;
    void setHourImport(uint8_t, uint32_t);
    void setHourExport(uint8_t, uint32_t);
    void setWeekDayImport(uint8_t, uint32_t);   //DGF added for week plot
    void setWeekDayExport(uint8_t, uint32_t);   //DGF added for week plot
    void setDayImport(uint8_t, uint32_t);
    void setDayExport(uint8_t, uint32_t);
};

#endif
