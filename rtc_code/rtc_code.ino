#include <DS3231.h>

// Code from the Demo Example of the DS3231 Library

DS3231  rtc(SDA, SCL);

void setup()
{
  // Setup Serial connection
  Serial.begin(115200);
  // Uncomment the next line if you are using an Arduino Leonardo
  //while (!Serial) {}
  
  // Initialize the rtc object
  rtc.begin();
  
//   The following lines can be uncommented to set the date and time
  rtc.setDOW(TUESDAY);     // Set Day-of-Week to SUNDAY
  rtc.setTime(4, 0, 0);     // Set the time to 12:00:00 (24hr format)
  rtc.setDate(15, 02, 2021);   // Set the date to January 1st, 2014
}

// Code from the Demo Example of the DS3231 Library
void loop()
{
  // Send Day-of-Week
  Serial.print(rtc.getDOWStr());
  Serial.print(" ");
  
  // Send date
  Serial.print(rtc.getDateStr());
  Serial.print(" -- ");
  // Send time
  Serial.println(rtc.getTimeStr());

  String hh = getValue(rtc.getTimeStr(), ':', 0);
  String mm = getValue(rtc.getTimeStr(), ':', 1);
  String ss = getValue(rtc.getTimeStr(), ':', 2);
  Serial.print(hh);
  Serial.print(mm);
  Serial.println(ss);
  
  // Wait one second before repeating
  delay (1000);
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
