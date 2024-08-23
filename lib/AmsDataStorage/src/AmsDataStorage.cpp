#include "AmsDataStorage.h"
#include <lwip/apps/sntp.h>
#include "LittleFS.h"
#include "AmsStorage.h"
#include "FirmwareVersion.h"

AmsDataStorage::AmsDataStorage(RemoteDebug* debugger) 
{
    day.version = 5;
    day.accuracy = 1;
    week.version = 6;
    week.accuracy = 1;
    month.version = 6;
    month.accuracy = 1;
    this->debugger = debugger;
}


void AmsDataStorage::setTimezone(Timezone* tz) 
{
    this->tz = tz;
}


/***********************************************************************************************************************************************************
 * Purpose: Update day, week and month plot
 * History: D.Geisenhoff 21-DEC-2023 Week plot added
 * *********************************************************************************************************************************************************/
bool AmsDataStorage::update(AmsData* data) 
{
    if(isHappy()) 
    {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(AmsDataStorage) Data is up to date\n"));
        return false;
    }

    time_t now = time(nullptr);
    if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(AmsDataStorage) Time is: %lu\n"), (int32_t) now);
    if(tz == NULL) 
    {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(AmsDataStorage) Timezone is missing\n"));
        return false;
    }
    if(now < FirmwareVersion::BuildEpoch) 
    {
        if(data->getMeterTimestamp() > FirmwareVersion::BuildEpoch) 
        {
            now = data->getMeterTimestamp();
            if(debugger->isActive(RemoteDebug::DEBUG)) 
            {
                debugger->printf_P(PSTR("(AmsDataStorage) Using meter timestamp, which is: %lu\n"), (int32_t) now);
            }
        } 
        else if(data->getPackageTimestamp() > FirmwareVersion::BuildEpoch) 
        {
            now = data->getPackageTimestamp();
            if(debugger->isActive(RemoteDebug::DEBUG)) 
            {
                debugger->printf_P(PSTR("(AmsDataStorage) Using package timestamp, which is: %lu\n"), (int32_t) now);
            }
        }
    }
    if(now < FirmwareVersion::BuildEpoch) 
    {
        if(debugger->isActive(RemoteDebug::VERBOSE)) {
            debugger->printf_P(PSTR("(AmsDataStorage) Invalid time: %lu\n"), (int32_t) now);
        }
        return false;
    }

    tmElements_t utc, ltz, utcYesterday, ltzYesterDay;
    breakTime(now, utc);
    breakTime(tz->toLocal(now), ltz);
    breakTime(now-3600, utcYesterday);
    breakTime(tz->toLocal(now-3600), ltzYesterDay);

    uint32_t importCounterTotal = data->getActiveImportCounterTotal() * 1000;  //DGF changed to total
    uint32_t exportCounterTotal = data->getActiveExportCounterTotal() * 1000;  //DGF changed to total

    // Clear hours between last update and now
    if(day.lastMeterReadTime > now) 
    {
        if(debugger->isActive(RemoteDebug::WARNING)) 
        {
            debugger->printf_P(PSTR("(AmsDataStorage) Invalid future timestamp for day plot, resetting\n"));
        }
        day.activeImportTotal = importCounterTotal; //DGF changed to total
        day.activeExportTotal = exportCounterTotal; //DGF changed to total
        day.lastMeterReadTime = now;
        } 
        else if(day.activeImportTotal == 0 || now - day.lastMeterReadTime > 86400)  //DGF changed to total
        {
            if(debugger->isActive(RemoteDebug::VERBOSE)) 
            {
                debugger->printf_P(PSTR("(AmsDataStorage) %lu == 0 || %lu - %lu > 86400"), day.activeImportTotal, now, day.lastMeterReadTime); //DGF changed to total
            }
        day.activeImportTotal = importCounterTotal; //DGF changed to total
        day.activeExportTotal = exportCounterTotal; //DGF changed to total
        day.lastMeterReadTime = now;
        if(debugger->isActive(RemoteDebug::WARNING)) 
        {
            debugger->printf_P(PSTR("(AmsDataStorage) Too long since last day update, clearing data\n"));
        }
        for(int i = 0; i<24; i++) 
        {
            setHourImport(i, 0);
            setHourExport(i, 0);
        }
    } 
    else 
    {
        if(debugger->isActive(RemoteDebug::DEBUG)) 
        {
            debugger->printf_P(PSTR("(AmsDataStorage) Last day update: %lu\n"), (int32_t) day.lastMeterReadTime);
        }
        tmElements_t last;
        breakTime(day.lastMeterReadTime, last);
        uint8_t endHour = utc.Hour;
        if(last.Hour > utc.Hour){
            for(int i = 0; i < utc.Hour; i++) 
            {
                if(debugger->isActive(RemoteDebug::VERBOSE)) 
                {
                    debugger->printf_P(PSTR("(AmsDataStorage) Clearing hour: %d\n"), i);
                }
                setHourImport(i, 0);
                setHourExport(i, 0);
            }
            endHour = 24;
        }
        for(int i = last.Hour; i < endHour; i++) {
            if(debugger->isActive(RemoteDebug::VERBOSE)) 
            {
                debugger->printf_P(PSTR("(AmsDataStorage) Clearing hour: %d\n"), i);
            }
            setHourImport(i, 0);
            setHourExport(i, 0);
        }
    }

    //DGF week plot added
    // Clear week days between last update and now
    if(week.lastMeterReadTime > now) 
    {
        if(debugger->isActive(RemoteDebug::WARNING)) 
        {
            debugger->printf_P(PSTR("(AmsDataStorage) Invalid future timestamp for week plot, resetting\n"));
        }
        week.activeImportTotal = importCounterTotal; 
        week.activeExportTotal = exportCounterTotal; 
        week.lastMeterReadTime = now;
    } 
    else if(week.activeImportTotal == 0 || now - week.lastMeterReadTime > 604800+3600) 
    {
        if(debugger->isActive(RemoteDebug::VERBOSE)) 
        {
            debugger->printf_P(PSTR("(AmsDataStorage) %lu == 0 || %lu - %lu > 608400"), week.activeImportTotal, now, week.lastMeterReadTime); //DGF changed to total
        }
        week.activeImportTotal = importCounterTotal; 
        week.activeExportTotal = exportCounterTotal; 
        week.lastMeterReadTime = now;
        if(debugger->isActive(RemoteDebug::WARNING)) 
        {
            debugger->printf_P(PSTR("(AmsDataStorage) Too long since last week update, clearing data\n"));
        }
        clearWeekPlot();
    } 
    else 
    {
        if(debugger->isActive(RemoteDebug::DEBUG)) 
        {
            debugger->printf_P(PSTR("(AmsDataStorage) Last week update: %lu\n"), (int32_t) week.lastMeterReadTime);
        }
        tmElements_t last;
        breakTime(tz->toLocal(week.lastMeterReadTime), last);
        uint8_t endDay = ltz.Wday;
        if(last.Wday > ltz.Wday) 
        {
            for(int i = 1; i < ltz.Wday; i++) 
            {
                if(debugger->isActive(RemoteDebug::VERBOSE)) 
                {
                    debugger->printf_P(PSTR("(AmsDataStorage) Clearing week day: %d\n"), i);
                }
                setWeekDayImport(i, 0);
                setWeekDayExport(i, 0);
            }
            endDay = 7;
        }
        for(int i = last.Wday; i < endDay; i++) 
        {
            if(debugger->isActive(RemoteDebug::VERBOSE)) 
            {
                debugger->printf_P(PSTR("(AmsDataStorage) Clearing week day: %d\n"), i);
            }
            setWeekDayImport(i, 0);
            setWeekDayExport(i, 0);
        }
    }
    //DGF

    // Clear days between last update and now (month plot)
    if(month.lastMeterReadTime > now) 
    {
        if(debugger->isActive(RemoteDebug::WARNING)) 
        {
            debugger->printf_P(PSTR("(AmsDataStorage) Invalid future timestamp for month plot, resetting\n"));
        }
        month.activeImportTotal = importCounterTotal; //DGF changed to total
        month.activeExportTotal = exportCounterTotal; //DGF changed to total
        month.lastMeterReadTime = now;
    } 
    else if(month.activeImportTotal == 0 || now - month.lastMeterReadTime > 2682000) //DGF changed to total, 2682000 = 31 days + 1hr?
    {
        if(debugger->isActive(RemoteDebug::VERBOSE)) 
        {
            debugger->printf_P(PSTR("(AmsDataStorage) %lu == 0 || %lu - %lu > 2682000"), month.activeImportTotal, now, month.lastMeterReadTime); //DGF changed to total
        }
        month.activeImportTotal = importCounterTotal; //DGF changed to total
        month.activeExportTotal = exportCounterTotal; //DGF changed to total
        month.lastMeterReadTime = now;
        if(debugger->isActive(RemoteDebug::WARNING)) 
        {
            debugger->printf_P(PSTR("(AmsDataStorage) Too long since last month update, clearing data\n"));
        }
        for(int i = 1; i<=31; i++) 
        {
            setDayImport(i, 0);
            setDayExport(i, 0);
        }
    } 
    else 
    {
        if(debugger->isActive(RemoteDebug::DEBUG)) 
        {
            debugger->printf_P(PSTR("(AmsDataStorage) Last month update: %lu\n"), (int32_t) month.lastMeterReadTime);
        }
        tmElements_t last;
        breakTime(tz->toLocal(month.lastMeterReadTime), last);
        uint8_t endDay = ltz.Day;
        if(last.Day > ltz.Day) {
            for(int i = 1; i < ltz.Day; i++) 
            {
                if(debugger->isActive(RemoteDebug::VERBOSE)) 
                {
                    debugger->printf_P(PSTR("(AmsDataStorage) Clearing day: %d\n"), i);
                }
                setDayImport(i, 0);
                setDayExport(i, 0);
            }
            endDay = 31;
        }
        for(int i = last.Day; i < endDay; i++) 
        {
            if(debugger->isActive(RemoteDebug::VERBOSE)) 
            {
                debugger->printf_P(PSTR("(AmsDataStorage) Clearing day: %d\n"), i);
            }
            setDayImport(i, 0);
            setDayExport(i, 0);
        }
    }

    if(data->getListType() < 3) 
    {
        debugger->printf_P(PSTR("(AmsDataStorage) Not enough data in list type: %d\n"), data->getListType());
        return false;
    }

    bool ret = false;

    // Update day plot
    if(!isDayHappy()) 
    {
        if(day.activeImportTotal > importCounterTotal || day.activeExportTotal > exportCounterTotal) //DGF changed to total
        {
            day.activeImportTotal = importCounterTotal; //DGF changed to total
            day.activeExportTotal = exportCounterTotal; //DGF changed to total
            day.lastMeterReadTime = now;
            setHourImport(utcYesterday.Hour, 0);
            setHourExport(utcYesterday.Hour, 0);
        } 
        else if(now - day.lastMeterReadTime < 4000) 
        {
            uint32_t imp = importCounterTotal - day.activeImportTotal; //DGF changed to total
            uint32_t exp = exportCounterTotal - day.activeExportTotal; //DGF changed to total
            setHourImport(utcYesterday.Hour, imp);
            setHourExport(utcYesterday.Hour, exp);

            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("(AmsDataStorage) Usage for hour %d: %d - %d\n"), ltzYesterDay.Hour, imp, exp);
            day.activeImportTotal = importCounterTotal; //DGF changed to total
            day.activeExportTotal = exportCounterTotal; //DGF changed to total
            day.lastMeterReadTime = now;
        } 
        else 
        {
            float mins = (now - day.lastMeterReadTime) / 60.0;
            uint32_t im = importCounterTotal - day.activeImportTotal; //DGF changed to total
            uint32_t ex = exportCounterTotal - day.activeExportTotal; //DGF changed to total
            float ipm = im / mins;
            float epm = ex / mins;

            if(debugger->isActive(RemoteDebug::DEBUG)) {
                debugger->printf_P(PSTR("(AmsDataStorage) Since last day update, minutes: %.1f, import: %d (%.2f/min), export: %d (%.2f/min)\n"), mins, im, ipm, ex, epm);
            }

            tmElements_t last;
            breakTime(day.lastMeterReadTime, last);
            day.lastMeterReadTime = day.lastMeterReadTime - (last.Minute * 60) - last.Second;
            time_t stopAt = now - (utc.Minute * 60) - utc.Second;
            while(day.lastMeterReadTime < stopAt) {
                time_t cur = min(day.lastMeterReadTime + 3600, stopAt);
                uint8_t minutes = round((cur - day.lastMeterReadTime) / 60.0);
                if(minutes < 1) break;

                breakTime(day.lastMeterReadTime, last);
                float imp = (ipm * minutes);
                float exp = (epm * minutes);
                setHourImport(last.Hour, imp);
                setHourExport(last.Hour, exp);

                if(debugger->isActive(RemoteDebug::INFO)) {
                    debugger->printf_P(PSTR("(AmsDataStorage) Estimated usage for hour %u: %.1f - %.1f (%lu)\n"), last.Hour, imp, exp, (int32_t) cur);
                }

                day.activeImportTotal += imp; //DGF changed to total
                day.activeExportTotal += exp; //DGF changed to total
                day.lastMeterReadTime = cur;
            }
        }
        ret = true;
    }

    //DGF week plot added
    // Update month plot
    if(ltz.Hour == 0 && !isWeekHappy()) 
    {
        if(week.activeImportTotal > importCounterTotal || week.activeExportTotal > exportCounterTotal) 
        {
            week.activeImportTotal = importCounterTotal;
            week.activeExportTotal = exportCounterTotal;
            week.lastMeterReadTime = now;
            setWeekDayImport(ltzYesterDay.Wday, 0);
            setWeekDayExport(ltzYesterDay.Wday, 0);
        } 
        else if(now - week.lastMeterReadTime < 90100 && now - week.lastMeterReadTime > 82700) 
        { // DST days are 23h (82800s) and 25h (90000)
            int32_t imp = importCounterTotal - week.activeImportTotal; 
            int32_t exp = exportCounterTotal - week.activeExportTotal; 

            if(debugger->isActive(RemoteDebug::INFO)) 
            {
                debugger->printf_P(PSTR("(AmsDataStorage) Usage for day %d: %d - %d\n"), ltzYesterDay.Wday, imp, exp);
            }

            setWeekDayImport(ltzYesterDay.Wday, imp);
            setWeekDayExport(ltzYesterDay.Wday, exp);
            week.activeImportTotal = importCounterTotal; 
            week.activeExportTotal = exportCounterTotal; 
            week.lastMeterReadTime = now;
        } 
        else 
        {
            // Make sure last week read is at midnight
            tmElements_t last;
            breakTime(tz->toLocal(week.lastMeterReadTime), last);
            week.lastMeterReadTime = week.lastMeterReadTime - (last.Hour * 3600) - (last.Minute * 60) - last.Second;
            if(debugger->isActive(RemoteDebug::DEBUG)) 
            {
                debugger->printf_P(PSTR("(AmsDataStorage) Last week read after resetting to midnight: %lu\n"), (int32_t) week.lastMeterReadTime);
            }

            float hrs = (now - week.lastMeterReadTime) / 3600.0;
            uint32_t im = importCounterTotal - week.activeImportTotal; 
            uint32_t ex = exportCounterTotal - week.activeExportTotal; 
            float iph = im / hrs;
            float eph = ex / hrs;

            if(debugger->isActive(RemoteDebug::DEBUG)) 
            {
                debugger->printf_P(PSTR("(AmsDataStorage) Since last week update, hours: %.1f, import: %d (%.2f/hr), export: %d (%.2f/hr)\n"), hrs, im, iph, ex, eph);
            }

            time_t stopAt = now - (ltz.Hour * 3600) - (ltz.Minute * 60) - ltz.Second;
            while(week.lastMeterReadTime < stopAt) 
            {
                time_t cur = min(week.lastMeterReadTime + 86400, stopAt);
                uint8_t hours = round((cur - week.lastMeterReadTime) / 3600.0);

                breakTime(tz->toLocal(week.lastMeterReadTime), last);
                float imp = (iph * hours);
                float exp = (eph * hours);
                setWeekDayImport(last.Wday, imp);
                setWeekDayExport(last.Wday, exp);

                if(debugger->isActive(RemoteDebug::INFO)) 
                {
                    debugger->printf_P(PSTR("(AmsDataStorage) Estimated usage for week day %u: %.1f - %.1f (%lu)\n"), last.Wday, imp, exp, (int32_t) cur);
                }

                week.activeImportTotal += imp; 
                week.activeExportTotal += exp;
                week.lastMeterReadTime = cur;
            }
        }
        ret = true;
    }
    //DGF

    // Update month plot
    if(ltz.Hour == 0 && !isMonthHappy()) 
    {
        if(month.activeImportTotal > importCounterTotal || month.activeExportTotal > exportCounterTotal) //DGF changed to total
        {
            month.activeImportTotal = importCounterTotal; //DGF changed to total
            month.activeExportTotal = exportCounterTotal; //DGF changed to total
            month.lastMeterReadTime = now;
            setDayImport(ltzYesterDay.Day, 0);
            setDayExport(ltzYesterDay.Day, 0);
        } 
        else if(now - month.lastMeterReadTime < 90100 && now - month.lastMeterReadTime > 82700) 
        { // DST days are 23h (82800s) and 25h (90000)
            int32_t imp = importCounterTotal - month.activeImportTotal; //DGF changed to total
            int32_t exp = exportCounterTotal - month.activeExportTotal; //DGF changed to total

            if(debugger->isActive(RemoteDebug::INFO)) {
                debugger->printf_P(PSTR("(AmsDataStorage) Usage for day %d: %d - %d\n"), ltzYesterDay.Day, imp, exp);
            }

            setDayImport(ltzYesterDay.Day, imp);
            setDayExport(ltzYesterDay.Day, exp);
            month.activeImportTotal = importCounterTotal; //DGF changed to total
            month.activeExportTotal = exportCounterTotal; //DGF changed to total
            month.lastMeterReadTime = now;
        } 
        else 
        {
            // Make sure last month read is at midnight
            tmElements_t last;
            breakTime(tz->toLocal(month.lastMeterReadTime), last);
            month.lastMeterReadTime = month.lastMeterReadTime - (last.Hour * 3600) - (last.Minute * 60) - last.Second;
            if(debugger->isActive(RemoteDebug::DEBUG)) 
            {
                debugger->printf_P(PSTR("(AmsDataStorage) Last month read after resetting to midnight: %lu\n"), (int32_t) month.lastMeterReadTime);
            }

            float hrs = (now - month.lastMeterReadTime) / 3600.0;
            uint32_t im = importCounterTotal - month.activeImportTotal; //DGF changed to total
            uint32_t ex = exportCounterTotal - month.activeExportTotal; //DGF changed to total
            float iph = im / hrs;
            float eph = ex / hrs;

            if(debugger->isActive(RemoteDebug::DEBUG)) 
            {
                debugger->printf_P(PSTR("(AmsDataStorage) Since last month update, hours: %.1f, import: %d (%.2f/hr), export: %d (%.2f/hr)\n"), hrs, im, iph, ex, eph);
            }

            time_t stopAt = now - (ltz.Hour * 3600) - (ltz.Minute * 60) - ltz.Second;
            while(month.lastMeterReadTime < stopAt) 
            {
                time_t cur = min(month.lastMeterReadTime + 86400, stopAt);
                uint8_t hours = round((cur - month.lastMeterReadTime) / 3600.0);

                breakTime(tz->toLocal(month.lastMeterReadTime), last);
                float imp = (iph * hours);
                float exp = (eph * hours);
                setDayImport(last.Day, imp);
                setDayExport(last.Day, exp);

                if(debugger->isActive(RemoteDebug::INFO)) 
                {
                    debugger->printf_P(PSTR("(AmsDataStorage) Estimated usage for day %u: %.1f - %.1f (%lu)\n"), last.Day, imp, exp, (int32_t) cur);
                }

                month.activeImportTotal += imp; //DGF changed to total
                month.activeExportTotal += exp; //DGF changed to total
                month.lastMeterReadTime = cur;
            }
        }
        ret = true;
    }
    return ret;
}


//EHorvat new minute plot
void AmsDataStorage::updateMinute(int32_t wattimpexp) 
{
    for(int i = 149; i>0; i--) 
    {
        minute.mWatt[i] = minute.mWatt[i-1] ;   
    }
    minute.mWatt[0] = wattimpexp;
}


int32_t AmsDataStorage::getMinuteWatt(uint8_t element) 
{
    if(element < 0 || element > 149) return 0;
    return minute.mWatt[element];
}
//EHorvat new minute plot END


//EHorvat new hour plot
void AmsDataStorage::updateHour(int32_t wattimpexp) 
{
    for(int i = 149; i>0; i--) {
        hour.mWatt[i] = hour.mWatt[i-1] ;   
    }
    hour.mWatt[0] = wattimpexp;
}


int32_t AmsDataStorage::getHourWatt(uint8_t element) 
{
    if(element < 0 || element > 149) return 0;
    return hour.mWatt[element];
}
//EHorvat new hour plot END


/***********************************************************************************************************************************************************
 * Purpose: Set import power for hour 
 * History: D.Geisenhoff 21-DEC-2023 This comment added
 * *********************************************************************************************************************************************************/
void AmsDataStorage::setHourImport(uint8_t hour, uint32_t val) 
{
    if(hour < 0 || hour > 24) return;
    
    uint8_t accuracy = day.accuracy;
    uint32_t update = val / pow(10, accuracy);
    while(update > UINT16_MAX) 
    {
        accuracy++;
        update = val / pow(10, accuracy);
    }
   
    if(accuracy != day.accuracy) 
    {
        setDayAccuracy(accuracy);
    }
    
    day.hImport[hour] = update;

    uint32_t max = 0;
    for(uint8_t i = 0; i < 24; i++) 
    {
        if(day.hImport[i] > max)
            max = day.hImport[i];
        if(day.hExport[i] > max)
            max = day.hExport[i];
    }

    while(max < UINT16_MAX/10 && accuracy > 0) 
    {
        accuracy--;
        max = max*10;
    }
    
    if(accuracy != day.accuracy) 
    {
        setDayAccuracy(accuracy);
    }
}


/***********************************************************************************************************************************************************
 * Purpose: Get import power of hour 
 * History: D.Geisenhoff 21-DEC-2023 This comment added
 * *********************************************************************************************************************************************************/
uint32_t AmsDataStorage::getHourImport(uint8_t hour) 
{
    if(hour < 0 || hour > 24) return 0;
    return day.hImport[hour] * pow(10, day.accuracy);
}


/***********************************************************************************************************************************************************
 * Purpose: Set export power for hour 
 * History: D.Geisenhoff 21-DEC-2023 This comment added
 * *********************************************************************************************************************************************************/
void AmsDataStorage::setHourExport(uint8_t hour, uint32_t val) 
{
    if(hour < 0 || hour > 24) return;
    
    uint8_t accuracy = day.accuracy;
    uint32_t update = val / pow(10, accuracy);
    while(update > UINT16_MAX)
    {
        accuracy++;
        update = val / pow(10, accuracy);
    }
    
    if(accuracy != day.accuracy) 
    {
        setDayAccuracy(accuracy);
    }

    day.hExport[hour] = update;

    uint32_t max = 0;
    for(uint8_t i = 0; i < 24; i++) 
    {
        if(day.hImport[i] > max)
            max = day.hImport[i];
        if(day.hExport[i] > max)
            max = day.hExport[i];
    }

    while(max < UINT16_MAX/10 && accuracy > 0) 
    {
        accuracy--;
        max = max*10;
    }
    
    if(accuracy != day.accuracy) 
    {
        setDayAccuracy(accuracy);
    }
}


/***********************************************************************************************************************************************************
 * Purpose: Get import power of hour 
 * History: D.Geisenhoff 21-DEC-2023 This comment added
 * *********************************************************************************************************************************************************/
uint32_t AmsDataStorage::getHourExport(uint8_t hour) 
{
    if(hour < 0 || hour > 24) return 0;
    return day.hExport[hour] * pow(10, day.accuracy);
}


/***********************************************************************************************************************************************************
 * Purpose: Set import power for weekday 
 * History: D.Geisenhoff 21-DEC-2023 Created
 * *********************************************************************************************************************************************************/
void AmsDataStorage::setWeekDayImport(uint8_t wday, uint32_t val) 
{
    if(wday < 1 || wday > 7) return;
    
    uint8_t accuracy = week.accuracy;
    uint32_t update = val / pow(10, accuracy);
    while(update > UINT16_MAX) 
    {
        accuracy++;
        update = val / pow(10, accuracy);
    }
    
    if(accuracy != week.accuracy) 
    {
        setWeekAccuracy(accuracy);
    }

    week.wdImport[wday-1] = update;

    uint32_t max = 0;
    for(uint8_t i = 0; i < 7; i++) 
    {
        if(week.wdImport[i] > max)
            max = week.wdImport[i];
        if(week.wdExport[i] > max)
            max = week.wdExport[i];
    }

    while(max < UINT16_MAX/10 && accuracy > 0) 
    {
        accuracy--;
        max = max*10;
    }
    
    if(accuracy != week.accuracy) 
    {
        setWeekAccuracy(accuracy);
    }
}


/***********************************************************************************************************************************************************
 * Purpose: Get import power of weekday 
 * History: D.Geisenhoff 21-DEC-2023 Created
 * *********************************************************************************************************************************************************/
uint32_t AmsDataStorage::getWeekDayImport(uint8_t wday) 
{
    if(wday < 1 || wday > 7) return 0;
    return (week.wdImport[wday-1] * pow(10, week.accuracy));
}


/***********************************************************************************************************************************************************
 * Purpose: Set export power for weekday 
 * History: D.Geisenhoff 21-DEC-2023 Created
 * *********************************************************************************************************************************************************/
void AmsDataStorage::setWeekDayExport(uint8_t wday, uint32_t val) 
{
    if(wday < 1 || wday > 7) return;
    
    uint8_t accuracy = week.accuracy;
    uint32_t update = val / pow(10, accuracy);
    while(update > UINT16_MAX) 
    {
        accuracy++;
        update = val / pow(10, accuracy);
    }
    
    if(accuracy != week.accuracy) 
    {
        setWeekAccuracy(accuracy);
    }

    week.wdExport[wday-1] = update;

    uint32_t max = 0;
    for(uint8_t i = 0; i < 7; i++) 
    {
        if(week.wdImport[i] > max)
            max = week.wdImport[i];
        if(week.wdExport[i] > max)
            max = week.wdExport[i];
    }

    while(max < UINT16_MAX/10 && accuracy > 0) 
    {
        accuracy--;
        max = max*10;
    }
    
    if(accuracy != week.accuracy) 
    {
        setWeekAccuracy(accuracy);
    }
}


/***********************************************************************************************************************************************************
 * Purpose: Get export power of weekday 
 * History: D.Geisenhoff 21-DEC-2023 Created
 * *********************************************************************************************************************************************************/
uint32_t AmsDataStorage::getWeekDayExport(uint8_t wday) 
{
    if(wday < 1 || wday > 7) return 0;
    return (week.wdExport[wday-1] * pow(10, week.accuracy));
}


/***********************************************************************************************************************************************************
 * Purpose: Set export power for day 
 * History: D.Geisenhoff 21-DEC-2023 This comment added
 * *********************************************************************************************************************************************************/
void AmsDataStorage::setDayImport(uint8_t day, uint32_t val) 
{
    if(day < 1 || day > 31) return;
    
    uint8_t accuracy = month.accuracy;
    uint32_t update = val / pow(10, accuracy);
    while(update > UINT16_MAX) 
    {
        accuracy++;
        update = val / pow(10, accuracy);
    }
    
    if(accuracy != month.accuracy) 
    {
        setMonthAccuracy(accuracy);
    }

    month.dImport[day-1] = update;

    uint32_t max = 0;
    for(uint8_t i = 0; i < 31; i++) 
    {
        if(month.dImport[i] > max)
            max = month.dImport[i];
        if(month.dExport[i] > max)
            max = month.dExport[i];
    }

    while(max < UINT16_MAX/10 && accuracy > 0) 
    {
        accuracy--;
        max = max*10;
    }
    
    if(accuracy != month.accuracy) 
    {
        setMonthAccuracy(accuracy);
    }
}

uint32_t AmsDataStorage::getDayImport(uint8_t day) 
{
    if(day < 1 || day > 31) return 0;
    return (month.dImport[day-1] * pow(10, month.accuracy));
}


void AmsDataStorage::setDayExport(uint8_t day, uint32_t val) 
{
    if(day < 1 || day > 31) return;
    
    uint8_t accuracy = month.accuracy;
    uint32_t update = val / pow(10, accuracy);
    while(update > UINT16_MAX) 
    {
        accuracy++;
        update = val / pow(10, accuracy);
    }
    
    if(accuracy != month.accuracy) 
    {
        setMonthAccuracy(accuracy);
    }

    month.dExport[day-1] = update;

    uint32_t max = 0;
    for(uint8_t i = 0; i < 31; i++) 
    {
        if(month.dImport[i] > max)
            max = month.dImport[i];
        if(month.dExport[i] > max)
            max = month.dExport[i];
    }

    while(max < UINT16_MAX/10 && accuracy > 0) 
    {
        accuracy--;
        max = max*10;
    }
    
    if(accuracy != month.accuracy) 
    {
        setMonthAccuracy(accuracy);
    }
}


uint32_t AmsDataStorage::getDayExport(uint8_t day) 
{
    if(day < 1 || day > 31) return 0;
    return (month.dExport[day-1] * pow(10, month.accuracy));
}


/***********************************************************************************************************************************************************
 * Purpose: Load day and month plot files 
 * History: D.Geisenhoff 21-DEC-2023 Week plot added
 * *********************************************************************************************************************************************************/
bool AmsDataStorage::load() 
{
    if(!LittleFS.begin()) 
    {
        if(debugger->isActive(RemoteDebug::ERROR)) 
        {
            debugger->printf_P(PSTR("(AmsDataStorage) Unable to load LittleFS\n"));
        }
        return false;
    }

    bool ret = false;
    if(LittleFS.exists(FILE_DAYPLOT)) 
    {
        File file = LittleFS.open(FILE_DAYPLOT, "r");
        char buf[file.size()];
        file.readBytes(buf, file.size());
        DayDataPoints* day = (DayDataPoints*) buf;
        file.close();
        ret = setDayData(*day);
    }

    //DGF week plot added
    if(LittleFS.exists(FILE_WEEKPLOT)) 
    {
        File file = LittleFS.open(FILE_WEEKPLOT, "r");
        char buf[file.size()];
        file.readBytes(buf, file.size());
        WeekDataPoints* week = (WeekDataPoints*) buf;
        file.close();
        ret = ret && setWeekData(*week);
    }
    // else
    // {
    //     clearWeekPlot();
    // }
    //DGF

    if(LittleFS.exists(FILE_MONTHPLOT)) 
    {
        File file = LittleFS.open(FILE_MONTHPLOT, "r");
        char buf[file.size()];
        file.readBytes(buf, file.size());
        MonthDataPoints* month = (MonthDataPoints*) buf;
        file.close();
        ret = ret && setMonthData(*month);
    }

    return ret;
}


/***********************************************************************************************************************************************************
 * Purpose: Save day and month plot to files 
 * History: D.Geisenhoff 21-DEC-2023 Week plot added
 * *********************************************************************************************************************************************************/
bool AmsDataStorage::save() 
{
    if(!LittleFS.begin()) 
    {
        if(debugger->isActive(RemoteDebug::ERROR)) 
        {
            debugger->printf_P(PSTR("(AmsDataStorage) Unable to load LittleFS\n"));
        }
        return false;
    }
    {
        File file = LittleFS.open(FILE_DAYPLOT, "w");
        char buf[sizeof(day)];
        memcpy(buf, &day, sizeof(day));
        for(unsigned long i = 0; i < sizeof(day); i++) 
        {
            file.write(buf[i]);
        }
        file.close();
    }
    //DGF week plot added
    {
        
        File file = LittleFS.open(FILE_WEEKPLOT, "w");
        char buf[sizeof(week)];
        memcpy(buf, &week, sizeof(week));
        for(unsigned long i = 0; i < sizeof(week); i++) 
        {
            file.write(buf[i]);
        }
        file.close();   
    }
    //DGF
    {
        File file = LittleFS.open(FILE_MONTHPLOT, "w");
        char buf[sizeof(month)];
        memcpy(buf, &month, sizeof(month));
        for(unsigned long i = 0; i < sizeof(month); i++) 
        {
            file.write(buf[i]);
        }
        file.close();
    }
    return true;
}


DayDataPoints AmsDataStorage::getDayData() 
{
    return day;
}


/***********************************************************************************************************************************************************
 * Purpose: Get week data
 * History: D.Geisenhoff 21-DEC-2023 Created
 * *********************************************************************************************************************************************************/
WeekDataPoints AmsDataStorage::getWeekData() 
{
    return week;
}


MonthDataPoints AmsDataStorage::getMonthData() 
{
    return month;
}


bool AmsDataStorage::setDayData(DayDataPoints& day) 
{
    if(day.version == 5) 
    {
        this->day = day;
        return true;
    } 
    else if(day.version == 4) 
    {
        this->day = day;
        this->day.accuracy = 1;
        this->day.version = 5;
        return true;
    } 
    else if(day.version == 3) 
    {
        this->day = day;
        for(uint8_t i = 0; i < 24; i++) this->day.hExport[i] = 0;
        this->day.accuracy = 1;
        this->day.version = 5;
        return true;
    }
    return false;
}


/***********************************************************************************************************************************************************
 * Purpose: Set week data
 * History: D.Geisenhoff 21-DEC-2023 Created
 * *********************************************************************************************************************************************************/
bool AmsDataStorage::setWeekData(WeekDataPoints& week) 
{
    if(week.version == 6) 
    {
        this->week = week;
        return true;
    } 
    else if(week.version == 5) 
    {
        this->week = week;
        this->week.accuracy = 1;
        this->week.version = 6;
        return true;
    } 
    else if(week.version == 4) 
    {
        this->week = week;
        for(uint8_t i = 0; i < 7; i++) this->week.wdExport[i] = 0;
        this->week.accuracy = 1;
        this->week.version = 6;
        return true;
    }
    return false;
}


bool AmsDataStorage::setMonthData(MonthDataPoints& month) 
{
    if(month.version == 6) 
    {
        this->month = month;
        return true;
    } 
    else if(month.version == 5) 
    {
        this->month = month;
        this->month.accuracy = 1;
        this->month.version = 6;
        return true;
    } 
    else if(month.version == 4) 
    {
        this->month = month;
        for(uint8_t i = 0; i < 31; i++) this->month.dExport[i] = 0;
        this->month.accuracy = 1;
        this->month.version = 6;
        return true;
    }
    return false;
}


uint8_t AmsDataStorage::getDayAccuracy() 
{
    return day.accuracy;
}


void AmsDataStorage::setDayAccuracy(uint8_t accuracy) 
{
    if(day.accuracy != accuracy) 
    {
        uint16_t multiplier = pow(10, day.accuracy)/pow(10, accuracy);
        for(uint8_t i = 0; i < 24; i++) 
        {
            day.hImport[i] = day.hImport[i] * multiplier;
            day.hExport[i] = day.hExport[i] * multiplier;
        }
        day.accuracy = accuracy;
    }
}


/***********************************************************************************************************************************************************
 * Purpose: Get week accuracy
 * History: D.Geisenhoff 21-DEC-2023 Created
 * *********************************************************************************************************************************************************/
uint8_t AmsDataStorage::getWeekAccuracy() 
{
    return week.accuracy;
}


/***********************************************************************************************************************************************************
 * Purpose: Set week accuracy
 * History: D.Geisenhoff 21-DEC-2023 Created
 * *********************************************************************************************************************************************************/
void AmsDataStorage::setWeekAccuracy(uint8_t accuracy) 
{
    if(week.accuracy != accuracy) {
        uint16_t multiplier = pow(10, week.accuracy)/pow(10, accuracy);
        for(uint8_t i = 0; i < 7; i++) {
            week.wdImport[i] = week.wdImport[i] * multiplier;
            week.wdExport[i] = week.wdExport[i] * multiplier;
        }
        week.accuracy = accuracy;
    }
    week.accuracy = accuracy;
}


/***********************************************************************************************************************************************************
 * Purpose: Clear week plot
 * History: D.Geisenhoff 21-DEC-2023 Created
 * *********************************************************************************************************************************************************/
void AmsDataStorage::clearWeekPlot()
{
    for (int i = 1; i <= 7; i++)
    {
        setWeekDayImport(i, 0);
        setWeekDayExport(i, 0);
    }
}


uint8_t AmsDataStorage::getMonthAccuracy() 
{
    return month.accuracy;
}


void AmsDataStorage::setMonthAccuracy(uint8_t accuracy) 
{
    if(month.accuracy != accuracy) {
        uint16_t multiplier = pow(10, month.accuracy)/pow(10, accuracy);
        for(uint8_t i = 0; i < 31; i++) {
            month.dImport[i] = month.dImport[i] * multiplier;
            month.dExport[i] = month.dExport[i] * multiplier;
        }
        month.accuracy = accuracy;
    }
    month.accuracy = accuracy;
}


/***********************************************************************************************************************************************************
 * Purpose: Check if plots can be updated
 * History: D.Geisenhoff 21-DEC-2023 Week plot added
 * *********************************************************************************************************************************************************/
bool AmsDataStorage::isHappy() 
{
    return isDayHappy() && isWeekHappy() && isMonthHappy();
}


bool AmsDataStorage::isDayHappy() 
{
    time_t now = time(nullptr);
    if(now < FirmwareVersion::BuildEpoch) return false;

    if(now < day.lastMeterReadTime) 
    {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(AmsDataStorage) Day data timestamp %lu < %lu\n"), (int32_t) now, (int32_t) day.lastMeterReadTime);
        return false;
    }
    if(now-day.lastMeterReadTime > 3600) 
    {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(AmsDataStorage) Day data timestamp age %lu - %lu > 3600\n"), (int32_t) now, (int32_t) day.lastMeterReadTime);
        return false;
    }
    tmElements_t tm, last;
    breakTime(tz->toLocal(now), tm);
    breakTime(tz->toLocal(day.lastMeterReadTime), last);
    if(tm.Hour != last.Hour) 
    {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(AmsDataStorage) Day data hour of last timestamp %d > %d\n"), tm.Hour, last.Hour);
        return false;
    }

    return true;
}


/***********************************************************************************************************************************************************
 * Purpose: Check if week plot can be updated
 * History: D.Geisenhoff 21-DEC-2023 Week plot added
 * *********************************************************************************************************************************************************/
bool AmsDataStorage::isWeekHappy() 
{
    if(tz == NULL) 
    {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(AmsDataStorage) Timezone is missing\n"));
        return false;
    }

    time_t now = time(nullptr);
    if(now < FirmwareVersion::BuildEpoch) return false;
    tmElements_t tm, last;
    
    if(now < week.lastMeterReadTime) 
    {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(AmsDataStorage) Week data timestamp %lu < %lu\n"), (int32_t) now, (int32_t) week.lastMeterReadTime);
        return false;
    }

    breakTime(tz->toLocal(now), tm);
    breakTime(tz->toLocal(week.lastMeterReadTime), last);
    if(tm.Wday != last.Wday) 
    {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(AmsDataStorage) Week data day of last timestamp %d > %d\n"), tm.Wday, last.Wday);
        return false;
    }
    if(now-week.lastMeterReadTime > 90100) // Seconds per day plus one hr? 
    {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(AmsDataStorage) Week %lu - %lu > 90100\n", (int32_t) now, (int32_t) week.lastMeterReadTime);
        return false;
    }

    return true;
}

bool AmsDataStorage::isMonthHappy() 
{
    if(tz == NULL) 
    {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(AmsDataStorage) Timezone is missing\n"));
        return false;
    }

    time_t now = time(nullptr);
    if(now < FirmwareVersion::BuildEpoch) return false;
    tmElements_t tm, last;
    
    if(now < month.lastMeterReadTime) 
    {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(AmsDataStorage) Month data timestamp %lu < %lu\n"), (int32_t) now, (int32_t) month.lastMeterReadTime);
        return false;
    }

    breakTime(tz->toLocal(now), tm);
    breakTime(tz->toLocal(month.lastMeterReadTime), last);
    if(tm.Day != last.Day) 
    {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(AmsDataStorage) Month data day of last timestamp %d > %d\n"), tm.Day, last.Day);
        return false;
    }
    if(now-month.lastMeterReadTime > 90100) // DGF Seconds per day plus 1 hr?
    {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(AmsDataStorage) Month %lu - %lu > 90100\n", (int32_t) now, (int32_t) month.lastMeterReadTime); //DGF Seconds per day plus one hr?
        return false;
    }

    return true;
}
