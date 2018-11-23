/*
ds3231 support module

Copyright (C) 2018 by Pavel Chauzov <poulch at mail dot ru>

*/
#define DS3231ADDR 0x68

#define _bcdToDec(val) ((uint8_t) ((val / 16 * 10) + (val % 16)))
#define _decToBcd(val) ((uint8_t) ((val / 10 * 16) + (val % 10)))

// fake millis syncprovider
time_t getTime_stub() {
//    uint8_t data[7];
    tmElements_t tm;

    tm.Second = 0;
    tm.Minute = 0;
    tm.Hour =   0;
    tm.Wday =   0;
    tm.Day =    1;
    tm.Month =  1;
    tm.Year = y2kYearToTm(17);
    time_t t = makeTime(tm);
    t += millis()/1000;
    return t;
}

time_t getTime_rtc() {
    uint8_t data[7];
    tmElements_t tm;

    i2c_write_uint8(DS3231ADDR,0);
    i2c_read_buffer(DS3231ADDR, data, 7);
    tm.Second = _bcdToDec(data[0]);
    tm.Minute = _bcdToDec(data[1]);
    tm.Hour =   _bcdToDec(data[2]);
    tm.Wday =   _bcdToDec(data[3]);
    tm.Day =    _bcdToDec(data[4]);
    tm.Month =  _bcdToDec(data[5]);
    tm.Year = y2kYearToTm(_bcdToDec(data[6]));

    return makeTime(tm);
}

uint8_t setTime_rtc(time_t nt) {
    uint8_t data[8];
    tmElements_t ct;
    breakTime(nt, ct);

    data[0] =  0;
    data[1] = _decToBcd(ct.Second);
    data[2] = _decToBcd(ct.Minute);
    data[3] = _decToBcd(ct.Hour);
    data[4] = _decToBcd(ct.Wday);
    data[5] = _decToBcd(ct.Day);
    data[6] = _decToBcd(ct.Month);
    data[7] = _decToBcd(tmYearToY2k(ct.Year));
    uint8_t s = i2c_write_buffer(DS3231ADDR, data, 8);
    return s;
}

String strDateTime(time_t t) {
    char buffer[30];
    const char* dow[] = {"Sun","Mon","Tue","Wen","Thu","Fri","Sat"};
    tmElements_t tm;
    breakTime(t,tm);
    snprintf_P(buffer, sizeof(buffer),
        PSTR("%04d-%02d-%02d %s %02d:%02d:%02d"),
        tmYearToCalendar(tm.Year),tm.Month,tm.Day,dow[tm.Wday-1],tm.Hour,tm.Minute,tm.Second
    );
    return String(buffer);
}

String strDate(time_t t) {
    char buffer[30];
    const char* dow[] = {"Sun","Mon","Tue","Wen","Thu","Fri","Sat"};
    tmElements_t tm;
    breakTime(t,tm);
    snprintf_P(buffer, sizeof(buffer), PSTR("%04d-%02d-%02d %s"), tmYearToCalendar(tm.Year),tm.Month,tm.Day,dow[tm.Wday-1]);
    return String(buffer);
}

String strTime(time_t t) {
    char buffer[30];
    tmElements_t tm;
    breakTime(t,tm);
    snprintf_P(buffer, sizeof(buffer),PSTR("%02d:%02d:%02d"),tm.Hour,tm.Minute,tm.Second);
    return String(buffer);
}
