#include "EnergyAccounting.h"
#include "LittleFS.h"
#include "AmsStorage.h"
#include "FirmwareVersion.h"


EnergyAccounting::EnergyAccounting(RemoteDebug* debugger) 
{
    data.version = 1;
    this->debugger = debugger;
}

void EnergyAccounting::setup(AmsDataStorage *ds, EnergyAccountingConfig *config) {
    this->ds = ds;
    this->config = config;
    this->currentThresholdIdx = 0;
}

void EnergyAccounting::setEapi(EntsoeApi *eapi) {
    this->eapi = eapi;
}

EnergyAccountingConfig* EnergyAccounting::getConfig() {
    return config;
}

void EnergyAccounting::setTimezone(Timezone* tz) {
    this->tz = tz;
}

bool EnergyAccounting::isInitialized() {
    return this->init;
}


/***********************************************************************************************************************************************************
 * Purpose: Update energy accounting data
 * History: D.Geisenhoff 22-DEC-2023 Week added
 * *********************************************************************************************************************************************************/
bool EnergyAccounting::update(AmsData* amsData) 
{

    if(config == NULL) return false;
    time_t now = time(nullptr);
    if(now < FirmwareVersion::BuildEpoch) return false;
    if(tz == NULL) {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(EnergyAccounting) Timezone is missing\n"));
        return false;
    }

    bool ret = false;
    tmElements_t local;
    breakTime(tz->toLocal(now), local);

    if(!init) 
    {
        currentHour = local.Hour;
        currentDay = local.Day;
        currentWeekDay = local.Wday;  // DGF Week added
        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf_P(PSTR("(EnergyAccounting) Initializing data at %lu\n"), (int32_t) now);
        if(!load()) 
        {
            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("(EnergyAccounting) Unable to load existing data\n"));
            data = 
            { 1, local.Month, 
                0, 0, 0, 0, // Cost yesterday, this week, this month, last month, DGF week added
                0, 0, 0, 0, // Income yesterday, this week, this month, last month, DGF week added
                0, 0, 0, // Last month import, export and accuracy
                0, 0, // Peak 1
                0, 0, // Peak 2
                0, 0, // Peak 3
                0, 0, // Peak 4
                0, 0  // Peak 5
            };
        } 
        else if(debugger->isActive(RemoteDebug::DEBUG)) 
        {
            for(uint8_t i = 0; i < 5; i++) 
            {
                debugger->printf_P(PSTR("(EnergyAccounting) Peak hour from day %d: %d\n"), data.peaks[i].day, data.peaks[i].value*10);
            }
            debugger->printf_P(PSTR("(EnergyAccounting) Loaded cost yesterday: %.2f, this month: %d, last month: %d\n"), data.costYesterday / 100.0, data.costThisMonth / 100.0, data.costLastMonth / 100.0);
            debugger->printf_P(PSTR("(EnergyAccounting) Loaded income yesterday: %.2f, this month: %d, last month: %d\n"), data.incomeYesterday / 100.0, data.incomeThisMonth / 100.0, data.incomeLastMonth / 100.0);        }
        init = true;
    }

    float price = getPriceForHour(0);
    float incomerefund = getIncomeRefundForHour(0);  //EHorvat new fixed income price / refund
    if(!initPrice && price != ENTSOE_NO_VALUE) 
    {
        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf_P(PSTR("(EnergyAccounting) Initializing prices at %lu\n"), (int32_t) now);
        calcDayCost();
    }

    if(local.Hour != currentHour && (amsData->getListType() >= 3 || local.Minute == 1)) 
    {
        if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("(EnergyAccounting) New local hour %d\n"), local.Hour);
        
        //current day
        tmElements_t oneHrAgo, oneHrAgoLocal;
        breakTime(now-3600, oneHrAgo);
        uint16_t val = ds->getHourImport(oneHrAgo.Hour) / 10;

        breakTime(tz->toLocal(now-3600), oneHrAgoLocal);
        ret |= updateMax(val, oneHrAgoLocal.Day);

        currentHour = local.Hour; // Need to be defined here so that day cost is correctly calculated
        if(local.Hour > 0) 
        {
            calcDayCost();
        }

        use = 0;
        produce = 0;
        costHour = 0;
        incomeHour = 0;

        // week and month
        uint8_t prevDay = currentDay;

        if(local.Day != currentDay) 
        {
            // DGF week added
            // current week
            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("(EnergyAccounting) New day %d\n"), local.Wday);
            data.costThisWeek += costDay * 100;
            data.incomeThisWeek += incomeDay * 100;
            currentWeekDay = local.Wday;
            ret = true;
            //DGF

            // current month
            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("(EnergyAccounting) New day %d\n"), local.Day);
            data.costYesterday = costDay * 100;
            data.costThisMonth += costDay * 100;
            costDay = 0;

            data.incomeYesterday = incomeDay * 100;
            data.incomeThisMonth += incomeDay * 100;
            incomeDay = 0;

            currentDay = local.Day;
            ret = true;
        }

        // Last month
        if(local.Month != data.month) 
        {
            if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("(EnergyAccounting) New month %d\n"), local.Month);
            data.costLastMonth = data.costThisMonth;
            data.costThisMonth = 0;
            data.incomeLastMonth = data.incomeThisMonth;
            data.incomeThisMonth = 0;
            for(uint8_t i = 0; i < 5; i++) 
            {
                data.peaks[i] = { 0, 0 };
            }

            uint64_t totalImport = 0, totalExport = 0;
            for(uint8_t i = 1; i <= prevDay; i++) 
            {
                totalImport += ds->getDayImport(i);
                totalExport += ds->getDayExport(i);
            }
            uint8_t accuracy = 0;
            uint64_t importUpdate = totalImport, exportUpdate = totalExport;
            while(importUpdate > UINT32_MAX || exportUpdate > UINT32_MAX) 
            {
                accuracy++;
                importUpdate = totalImport / pow(10, accuracy);
                exportUpdate = totalExport / pow(10, accuracy);
            }
            data.lastMonthImport = importUpdate;
            data.lastMonthExport = exportUpdate;
            data.lastMonthAccuracy = accuracy;

            data.month = local.Month;
            currentThresholdIdx = 0;
            ret = true;
        }
    }

    unsigned long ms = this->lastUpdateMillis > amsData->getLastUpdateMillis() ? 0 : amsData->getLastUpdateMillis() - this->lastUpdateMillis;
    float kwhi = (amsData->getActiveImportPower() * (((float)ms) / 3600000.0)) / 1000.0;
    float kwhe = (amsData->getActiveExportPower() * (((float)ms) / 3600000.0)) / 1000.0;
    lastUpdateMillis = amsData->getLastUpdateMillis();
    if(kwhi > 0) 
    {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(EnergyAccounting) Adding %.8f kWh import\n", kwhi);  //EHorvat NES-MEP was .4f
        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf("(EHo) The now time variable is  %ld\n", (int64_t) now); //EHorvat NES-MEP want to see this
        
        use += kwhi;
        if(price != ENTSOE_NO_VALUE) 
        {
            float cost = price * kwhi;
            if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(EnergyAccounting)  and %.4f %s\n"), cost / 100.0, currency.c_str());            
            costHour += cost;
            costDay += cost;
        }
    }
    //Serial.printf("kwhi: %f, use: %f\n",kwhi,use);
    if(kwhe > 0) 
    {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf("(EnergyAccounting) Adding %.8f kWh export\n", kwhe); //EHorvat NES-MEP was .4f
        produce += kwhe;
        if(incomerefund != ENTSOE_NO_VALUE) 
        { //EHorvat new fixed income price / refund was...
            float income = incomerefund * kwhe; //EHorvat new fixed income price / refund (was float income = price * kwhe;)
            if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(EnergyAccounting)  and %.4f %s\n"), income / 100.0, currency.c_str());
            incomeHour += income;
            incomeDay += income;
        }
    }

    if(config != NULL) 
    {
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(EnergyAccounting)  calculating threshold, currently at %d\n"), currentThresholdIdx);
        while(getMonthMax() > config->thresholds[currentThresholdIdx] && currentThresholdIdx < 10) currentThresholdIdx++;
        if(debugger->isActive(RemoteDebug::VERBOSE)) debugger->printf_P(PSTR("(EnergyAccounting)  new threshold %d\n"), currentThresholdIdx);
    }

    return ret;
}

void EnergyAccounting::calcDayCost() 
{
    time_t now = time(nullptr);
    tmElements_t local, utc;
    breakTime(tz->toLocal(now), local);

    if(getPriceForHour(0) != ENTSOE_NO_VALUE) 
    {
        if(initPrice) 
        {
            costDay = 0;
            incomeDay = 0;
        }
        for(uint8_t i = 0; i < currentHour; i++) 
        {
            float price = getPriceForHour(i - local.Hour);
            if(price == ENTSOE_NO_VALUE) break;
            breakTime(now - ((local.Hour - i) * 3600), utc);
            int16_t wh = ds->getHourImport(utc.Hour);
            costDay += price * (wh / 1000.0);

//            wh = ds->getHourExport(utc.Hour);              // EHorvat Moved to next if getIncomeRefundForHour
//            incomeDay += price * (wh / 1000.0);            // EHorvat Moved to next if getIncomeRefundForHour
        }
        initPrice = true;
    }
    if(getIncomeRefundForHour(0) != ENTSOE_NO_VALUE) 
    {       //EHorvat new fixed income price / refund
        for(uint8_t i = 0; i < currentHour; i++) 
        {
            float fixedRefund = getIncomeRefundForHour(0); //EHorvat new for income pricing calc ... always use fixed price and not Ensoe
            if(fixedRefund == ENTSOE_NO_VALUE) break;
            breakTime(now - ((local.Hour - i) * 3600), utc);
            int16_t wh = ds->getHourExport(utc.Hour);  // EHorvat Moved from prev if getPriceForHour
            incomeDay += fixedRefund * (wh / 1000.0);// EHorvat Moved from prev if getPriceForHour
        }
    }
}

float EnergyAccounting::getUseThisHour() 
{
    return use;
}

float EnergyAccounting::getUseToday() 
{
    float ret = 0.0;
    time_t now = time(nullptr);
    if(now < FirmwareVersion::BuildEpoch) return 0.0;
    if(tz == NULL) return 0.0;
    tmElements_t utc, local;
    breakTime(tz->toLocal(now), local);
    for(uint8_t i = 0; i < currentHour; i++) 
    {
        breakTime(now - ((local.Hour - i) * 3600), utc);
        ret += ds->getHourImport(utc.Hour) / 1000.0;
    }
    return ret + getUseThisHour();
}


/***********************************************************************************************************************************************************
 * Purpose: Get energy used this week, so far
 * History: D.Geisenhoff 22-DEC-2023 Week added
 * *********************************************************************************************************************************************************/
float EnergyAccounting::getUseThisWeek() 
{
    time_t now = time(nullptr);
    if(now < FirmwareVersion::BuildEpoch) return 0.0;
    float ret = 0;
    for(uint8_t i = 1; i < currentWeekDay; i++) 
    {
        ret += ds->getWeekDayImport(i) / 1000.0;
    }
    return ret + getUseToday();
}


float EnergyAccounting::getUseThisMonth() {
    time_t now = time(nullptr);
    if(now < FirmwareVersion::BuildEpoch) return 0.0;
    float ret = 0;
    for(uint8_t i = 1; i < currentDay; i++) {
        ret += ds->getDayImport(i) / 1000.0;
    }
    return ret + getUseToday();
}


float EnergyAccounting::getUseLastMonth() {
    return (data.lastMonthImport * pow(10, data.lastMonthAccuracy)) / 1000;
}

float EnergyAccounting::getProducedThisHour() {
    return produce;
}

float EnergyAccounting::getProducedToday() {
    float ret = 0.0;
    time_t now = time(nullptr);
    if(now < FirmwareVersion::BuildEpoch) return 0.0;
    tmElements_t utc, local;
    breakTime(tz->toLocal(now), local);
    for(uint8_t i = 0; i < currentHour; i++) {
        breakTime(now - ((local.Hour - i) * 3600), utc);
        ret += ds->getHourExport(utc.Hour) / 1000.0;
    }
    return ret + getProducedThisHour();
}


/***********************************************************************************************************************************************************
 * Purpose: Get energy producedthis week, so far
 * History: D.Geisenhoff 22-DEC-2023 Week added
 * *********************************************************************************************************************************************************/
float EnergyAccounting::getProducedThisWeek() 
{
    time_t now = time(nullptr);
    if(now < FirmwareVersion::BuildEpoch) return 0.0;
    float ret = 0;
    for(uint8_t i = 1; i < currentWeekDay; i++) 
    {
        ret += ds->getWeekDayExport(i) / 1000.0;
    }
    return ret + getProducedToday();
}


float EnergyAccounting::getProducedThisMonth() 
{
    time_t now = time(nullptr);
    if(now < FirmwareVersion::BuildEpoch) return 0.0;
    float ret = 0;
    for(uint8_t i = 1; i < currentDay; i++) {
        ret += ds->getDayExport(i) / 1000.0;
    }
    return ret + getProducedToday();
}


float EnergyAccounting::getProducedLastMonth() 
{
    return (data.lastMonthExport * pow(10, data.lastMonthAccuracy)) / 1000;
}

float EnergyAccounting::getCostThisHour() {
    return costHour;
}



/***********************************************************************************************************************************************************
 * Purpose: Get today costs
 * History: D.Geisenhoff 22-DEC-2023 This comment added
 * *********************************************************************************************************************************************************/
float EnergyAccounting::getCostToday() 
{
    return costDay;
}


float EnergyAccounting::getCostYesterday() {
    return data.costYesterday / 100.0;
}


/***********************************************************************************************************************************************************
 * Purpose: Get costs this week, so far
 * History: D.Geisenhoff 22-DEC-2023 Week added
 * *********************************************************************************************************************************************************/
float EnergyAccounting::getCostThisWeek() 
{
    return (data.costThisWeek / 100.0) + getCostToday();
}


float EnergyAccounting::getCostThisMonth() {
    return (data.costThisMonth / 100.0) + getCostToday();
}


float EnergyAccounting::getCostLastMonth() {
    return data.costLastMonth / 100.0;
}

float EnergyAccounting::getIncomeThisHour() {
    return incomeHour;
}

float EnergyAccounting::getIncomeToday() {
    return incomeDay;
}

float EnergyAccounting::getIncomeYesterday() {
    return data.incomeYesterday / 100.0;
}


/***********************************************************************************************************************************************************
 * Purpose: Get income this week, so far
 * History: D.Geisenhoff 22-DEC-2023 Week added
 * *********************************************************************************************************************************************************/
float EnergyAccounting::getIncomeThisWeek() 
{
    return (data.incomeThisWeek / 100.0) + getIncomeToday();
}

float EnergyAccounting::getIncomeThisMonth() {
    return (data.incomeThisMonth / 100.0) + getIncomeToday();
}


float EnergyAccounting::getIncomeLastMonth() {
    return data.incomeLastMonth / 100.0;
}

uint8_t EnergyAccounting::getCurrentThreshold() {
    if(config == NULL)
        return 0;
    return config->thresholds[currentThresholdIdx];
}

float EnergyAccounting::getMonthMax() 
{
    if(config == NULL)
        return 0.0;
    uint8_t count = 0;
    uint32_t maxHour = 0.0;
    bool included[5] = { false, false, false, false, false };

    for(uint8_t x = 0; x < min((uint8_t) 5, config->hours); x++) 
    {
        uint8_t maxIdx = 0;
        uint16_t maxVal = 0;
        for(uint8_t i = 0; i < 5; i++) {
            if(included[i]) continue;
            if(data.peaks[i].day == 0) continue;
            if(data.peaks[i].value > maxVal) {
                maxVal = data.peaks[i].value;
                maxIdx = i;
            }
        }
        if(maxVal > 0) {
            included[maxIdx] = true;
            count++;
        }
    }

    for(uint8_t i = 0; i < 5; i++) {
        if(!included[i]) continue;
        maxHour += data.peaks[i].value;
    }
    return maxHour > 0 ? maxHour / count / 100.0 : 0.0;
}

EnergyAccountingPeak EnergyAccounting::getPeak(uint8_t num) {
    if(config == NULL)
        return EnergyAccountingPeak({0,0});
    if(num < 1 || num > 5) return EnergyAccountingPeak({0,0});

    uint8_t count = 0;
    bool included[5] = { false, false, false, false, false };

    for(uint8_t x = 0;x < min((uint8_t) 5, config->hours); x++) 
    {
        uint8_t maxIdx = 0;
        uint16_t maxVal = 0;
        for(uint8_t i = 0; i < 5; i++) 
        {
            if(included[i]) continue;
            if(data.peaks[i].value > maxVal) 
            {
                maxVal = data.peaks[i].value;
                maxIdx = i;
            }
        }
        if(maxVal > 0) {
            included[maxIdx] = true;
            count++;
        }
    }

    uint8_t pos = 0;
    for(uint8_t i = 0; i < 5; i++) 
    {
        if(!included[i]) continue;
        pos++;
        if(pos == num) {
            return data.peaks[i];
        }
    }
    return EnergyAccountingPeak({0,0});
}


/***********************************************************************************************************************************************************
 * Purpose: Load energy accounting data
 * History: D.Geisenhoff 22-DEC-2023 This comment added
 * *********************************************************************************************************************************************************/
bool EnergyAccounting::load() 
{
    if(!LittleFS.begin()) 
    {
        if(debugger->isActive(RemoteDebug::ERROR)) 
        {
            debugger->printf_P(PSTR("(EnergyAccounting) Unable to load LittleFS\n"));
        }
        return false;
    }

    bool ret = false;
    if(LittleFS.exists(FILE_ENERGYACCOUNTING)) 
    {
        File file = LittleFS.open(FILE_ENERGYACCOUNTING, "r");
        char buf[file.size()];
        file.readBytes(buf, file.size());

        if(debugger->isActive(RemoteDebug::DEBUG)) debugger->printf_P(PSTR("(EnergyAccounting) Data version %d\n"), buf[0]);
        if (buf[0] == 1)
        {
            EnergyAccountingData* data = (EnergyAccountingData*) buf;
            memcpy(&this->data, data, sizeof(this->data));
            ret = true;
        }
        // DGF removed (no older versions of data) 
       /*  else if(buf[0] == 5) 
        {
            EnergyAccountingData5* data = (EnergyAccountingData5*) buf;
            this->data = { 6, data->month, 
                ((uint32_t) data->costYesterday) * 10,
                ((uint32_t) data->costThisMonth) * 100,
                ((uint32_t) data->costLastMonth) * 100,
                ((uint32_t) data->incomeYesterday) * 10,
                ((uint32_t) data->incomeThisMonth) * 100,
                ((uint32_t) data->incomeLastMonth) * 100,
                0,0,0, // Last month import, export and accuracy
                data->peaks[0].day, data->peaks[0].value,
                data->peaks[1].day, data->peaks[1].value,
                data->peaks[2].day, data->peaks[2].value,
                data->peaks[3].day, data->peaks[3].value,
                data->peaks[4].day, data->peaks[4].value
            };
            ret = true;
        } 
        else if(buf[0] == 4) 
        {
            EnergyAccountingData4* data = (EnergyAccountingData4*) buf;
            this->data = { 5, data->month, 
                ((uint32_t) data->costYesterday) * 10,
                ((uint32_t) data->costThisMonth) * 100,
                ((uint32_t) data->costLastMonth) * 100,
                0,0,0, // Income from production
                0,0,0, // Last month import, export and accuracy
                data->peaks[0].day, data->peaks[0].value,
                data->peaks[1].day, data->peaks[1].value,
                data->peaks[2].day, data->peaks[2].value,
                data->peaks[3].day, data->peaks[3].value,
                data->peaks[4].day, data->peaks[4].value
            };
            ret = true;
        } 
        else if(buf[0] == 3) 
        {
            EnergyAccountingData* data = (EnergyAccountingData*) buf;
            this->data = { 5, data->month, 
                data->costYesterday * 10,
                data->costThisMonth,
                data->costLastMonth,
                0,0,0, // Income from production
                0,0,0, // Last month import, export and accuracy
                data->peaks[0].day, data->peaks[0].value,
                data->peaks[1].day, data->peaks[1].value,
                data->peaks[2].day, data->peaks[2].value,
                data->peaks[3].day, data->peaks[3].value,
                data->peaks[4].day, data->peaks[4].value
            };
            ret = true;
        }  */
        //DGF
        else 
        {
            data = 
            {   1, 0, 
                0, 0, 0, 0, // Costyesterday, this week, this month, last month, DGF Week added
                0, 0, 0, 0, // Income yesterday, this week, this month, last month from production, DGF week added
                0, 0, 0, // Last month import, export and accuracy
                0, 0, // Peak 1
                0, 0, // Peak 2
                0, 0, // Peak 3
                0, 0, // Peak 4
                0, 0  // Peak 5
            };
            // DGF removed, no older version od data
            /*
            if(buf[0] == 2) {
                EnergyAccountingData2* data = (EnergyAccountingData2*) buf;
                this->data.month = data->month;
                this->data.costYesterday = data->costYesterday * 10;
                this->data.costThisMonth = data->costThisMonth;
                this->data.costLastMonth = data->costLastMonth;
                uint8_t b = 0;
                for(uint8_t i = sizeof(this->data); i < file.size(); i+=2) {
                    this->data.peaks[b].day = b;
                    memcpy(&this->data.peaks[b].value, buf+i, 2);
                    b++;
                    if(b >= config->hours || b >= 5) break;
                }
                ret = true;
            }
            else
            {
                */
            if (debugger->isActive(RemoteDebug::WARNING))
                debugger->printf_P(PSTR("(EnergyAccounting) Unknown version\n"));
            ret = false;
            //}
            //DGF
        }
        file.close();
    } 
    else 
    {
        if(debugger->isActive(RemoteDebug::WARNING)) debugger->printf_P(PSTR("(EnergyAccounting) File not found\n"));
    }

    return ret;
}


/***********************************************************************************************************************************************************
 * Purpose: Save energy accounting data
 * History: D.Geisenhoff 22-DEC-2023 This comment added
 * *********************************************************************************************************************************************************/
bool EnergyAccounting::save() 
{
    if(!LittleFS.begin()) 
    {
        if(debugger->isActive(RemoteDebug::ERROR)) 
        {
            debugger->printf_P(PSTR("(EnergyAccounting) Unable to load LittleFS\n"));
        }
        return false;
    }
    {
        File file = LittleFS.open(FILE_ENERGYACCOUNTING, "w");
        char buf[sizeof(data)];
        memcpy(buf, &data, sizeof(data));
        for(uint8_t i = 0; i < sizeof(buf); i++) 
        {
            file.write(buf[i]);
        }
        file.close();
    }
    return true;
}


EnergyAccountingData EnergyAccounting::getData() 
{
    return this->data;
}


void EnergyAccounting::setData(EnergyAccountingData& data) 
{
    this->data = data;
}


bool EnergyAccounting::updateMax(uint16_t val, uint8_t day) 
{
    for(uint8_t i = 0; i < 5; i++) {
        if(data.peaks[i].day == day || data.peaks[i].day == 0) {
            if(val > data.peaks[i].value) {
                if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("(EnergyAccounting) Adding new max %d for day %d which is larger than %d\n"), val*10, day, data.peaks[i].value*10);
                data.peaks[i].day = day;
                data.peaks[i].value = val;
                return true;
            }
            return false;
        }
    }
    uint16_t test = 0;
    uint8_t idx = 255;
    for(uint8_t i = 0; i < 5; i++) {
        if(val > data.peaks[i].value) {
            if(test < data.peaks[i].value) {
                test = data.peaks[i].value;
                idx = i;
            }
        }
    }
    if(idx < 5) {
        if(debugger->isActive(RemoteDebug::INFO)) debugger->printf_P(PSTR("(EnergyAccounting) Adding new max %d for day %d\n"), val*10, day);
        data.peaks[idx].value = val;
        data.peaks[idx].day = day;
        return true;
    }
    return false;
}


/***********************************************************************************************************************************************************
 * Purpose: Set current price per imported kWh
 * History: D.Geisenhoff 22-DEC-2023 This comment added
 * *********************************************************************************************************************************************************/
void EnergyAccounting::setFixedPrice(float price, String currency) 
{
    this->fixedPrice = price;
    this->currency = currency;
}


/***********************************************************************************************************************************************************
 * Purpose: Get price per kWh (for current import kW gauge)
 * History: D.Geisenhoff 18-DEC-2023 This comment added
 * *********************************************************************************************************************************************************/
float EnergyAccounting::getPriceForHour(uint8_t h) 
{
    if(this->fixedPrice > 0.0) return this->fixedPrice;   //DGF added this-> to fixedPrice
    if(eapi == NULL) return ENTSOE_NO_VALUE;
    return eapi->getValueForHour(h);
}


// EHorvat new fixed income price / refund
void EnergyAccounting::setFixedRefund(float refund, String currency) {
    this->fixedRefund = refund;
    this->currency = currency;
}
// EHorvat new fixed income price / refund
float EnergyAccounting::getIncomeRefundForHour(uint8_t h) {
    if(fixedRefund > 0.0) return fixedRefund;
    return 0.0;
}
//
// EHorvat new last_counter_value
void EnergyAccounting::setLast_Counter_Value(uint32_t last_counter_value) {
    this->last_counter_value = last_counter_value;
}
// EHorvat new last_counter_value
uint32_t EnergyAccounting::getLast_Counter_Value() {
    if(last_counter_value > 0) return last_counter_value;
    return 0;
}
// EHorvat new last_counter_value
void EnergyAccounting::setLast_Counter_Info(String last_counter_info) {
    this->last_counter_info = last_counter_info;
}
// EHorvat new last_counter_value
String EnergyAccounting::getLast_Counter_Info() {
    if(last_counter_info != "") return last_counter_info;
    return "";
}
// EHorvat new last_counter_value
void EnergyAccounting::setCostsLastCounterValue(float costs_last_counter_value) 
{
    this->costs_last_counter_value = costs_last_counter_value;
}
// EHorvat new last_counter_value
float EnergyAccounting::getCostsLastCounterValue() 
{
    if(costs_last_counter_value > 0) return costs_last_counter_value;
    return 0.0;
}
// EHorvat new last_counter_value
void EnergyAccounting::setLast_Counter_Diff(uint32_t last_counter_diff) 
{
    this->last_counter_diff = last_counter_diff;
}
// EHorvat new last_counter_value
uint32_t EnergyAccounting::getLast_Counter_Diff() 
{
    if(last_counter_diff > 0) return last_counter_diff;
    return 0;
}
//
//
// EHorvat new last_counter_value
void EnergyAccounting::setLast_expCounter_Value(uint32_t last_expcounter_value) {
    this->last_expcounter_value = last_expcounter_value;
}
// EHorvat new last_counter_value
uint32_t EnergyAccounting::getLast_expCounter_Value() {
    if(last_expcounter_value > 0) return last_expcounter_value;
    return 0;
}
// EHorvat new last_counter_value
void EnergyAccounting::setLast_expCounter_Info(String last_expcounter_info) {
    this->last_expcounter_info = last_expcounter_info;
}
// EHorvat new last_counter_value
String EnergyAccounting::getLast_expCounter_Info() 
{
    if(last_expcounter_info != "") return last_expcounter_info;
    return "";
}
// EHorvat new last_counter_value
void EnergyAccounting::setCostsLastexpCounterValue(float costs_last_expcounter_value) {
    this->costs_last_expcounter_value = costs_last_expcounter_value;
}
// EHorvat new last_counter_value
float EnergyAccounting::getCostsLastexpCounterValue() {
    if(costs_last_expcounter_value > 0) return costs_last_expcounter_value;
    return 0.0;
}
// EHorvat new last_counter_value
void EnergyAccounting::setLast_expCounter_Diff(uint32_t last_expcounter_diff) {
    this->last_expcounter_diff = last_expcounter_diff;
}
// EHorvat new last_counter_value
uint32_t EnergyAccounting::getLast_expCounter_Diff() {
    if(last_expcounter_diff > 0) return last_expcounter_diff;
    return 0;
}
