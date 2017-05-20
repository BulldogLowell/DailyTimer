/*
 * DailyTimer Library
   Copyright(c) 2017, James Brower,  BulldogLowell@gmail.com
   
   Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
   to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
   and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
   IN THE SOFTWARE.
   
   Library provides tools to set daily timers for control of devices such as lamps, appliances, etc. Developed primarilary for houshold presence simulation.
   Allows for setting ON and OFF times, days of the week (i.e. weekends, weekdays, Fridays) and the use of random on/off times 
   Timers may bridge midnight, simply enter times accordingly:
   Automatically sets correct timer state on powerup, see example below.
   Randomize start and/or end times using this member function:   
   Random days of week using setRandomDays(4); // will select four days of the week, randomly.  Useful to run this member function once a week, for example.
   Select custom days of the week using setDaysActive(0b10101010);  // e.g.Sunday, Tuesday, Thursday and Saturday Note: Days in this order:  0bSMTWTFS0 <- LSB is zero 
   Set a timed event with just the start time as a trigger:
   you can return the active days using getDays();  // returns a byte... Sunday is the MSB, Saturday is the LSB << 1
   Dynamically set your start or end time (i.e. using some type of celestial function or web call to determine Sunrise or Sunset times)
   myTimer.setStartTime(byte hour, byte minute);
   myTimer.setEndTime(byte hour, byte minute);
   
*/

#include "DailyTimer.h"

int DailyTimer::instanceCount = 0;
DailyTimer* instances[MAX_TIMER_INSTANCES] = {nullptr};

//DailyTimer::DailyTimer(bool syncOnPowerup, byte StartHour, byte StartMinute, EventDays DaysOfTheWeek, void(*StartTimeCallback)())
//{
//  DailyTimer(syncOnPowerup, StartHour, StartMinute, 0, 0, DaysOfTheWeek, FIXED, StartTimeCallback, NULL);
//}

//DailyTimer::DailyTimer(bool syncOnPowerup, byte StartHour, byte StartMinute, EventDays DaysOfTheWeek, RandomTime randomTime, void(*StartTimeCallback)())
//{
//  DailyTimer(syncOnPowerup, StartHour, StartMinute, 0, 0, DaysOfTheWeek, randomTime, StartTimeCallback, NULL);
//}

DailyTimer::DailyTimer(bool syncOnPowerup, byte StartHour, byte StartMinute, byte EndHour, byte EndMinute, EventDays DaysOfTheWeek, void(*StartTimeCallback)(), void(*EndTimeCallback)())
{
  autoSync = syncOnPowerup;
  startTime.hour = constrain(StartHour, 0, 23);
  startTime.minute = constrain(StartMinute, 0, 59);
  endTime.hour = constrain(EndHour, 0, 23);
  endTime.minute = constrain(EndMinute, 0, 59);
  setDaysActive(dayTemplate[DaysOfTheWeek]);
  randomType = FIXED;
  offset = 0;
  startTimeCallback = StartTimeCallback;
  endTimeCallback = EndTimeCallback;
  instances[instanceCount++] = this;
  //DailyTimer(syncOnPowerup, StartHour, StartMinute, EndHour, EndMinute, DaysOfTheWeek, FIXED, StartTimeCallback, EndTimeCallback);
}

DailyTimer::DailyTimer(bool syncOnPowerup, byte StartHour, byte StartMinute, byte EndHour, byte EndMinute, EventDays DaysOfTheWeek, RandomTime randomTime, void(*StartTimeCallback)(), void(*EndTimeCallback)())
{
  autoSync = syncOnPowerup;
  startTime.hour = constrain(StartHour, 0, 23);
  startTime.minute = constrain(StartMinute, 0, 59);
  endTime.hour = constrain(EndHour, 0, 23);
  endTime.minute = constrain(EndMinute, 0, 59);
  setDaysActive(dayTemplate[DaysOfTheWeek]);
  randomType = randomTime;
  offset = 15;
  startTimeCallback = StartTimeCallback;
  endTimeCallback = EndTimeCallback;
  instances[instanceCount++] = this;
}

bool DailyTimer::begin()
{
  return sync();
}

void DailyTimer::setDaysActive(EventDays days)
{
  onMask = dayTemplate[days];
  time_t now_time = now();
  if(tmConvert_t(year(now_time), month(now_time), day(now_time), startTime.hour, startTime.minute, 0) > tmConvert_t(year(now_time), month(now_time), day(now_time), endTime.hour, endTime.minute, 0))
  {
    offMask = onMask >> 1;
  }
  else
  {
    offMask = onMask;
  }
  (void)sync();
}

void DailyTimer::setDaysActive(byte activeDays)
{
  onMask = activeDays;
  time_t now_time = now();
  if(tmConvert_t(year(now_time), month(now_time), day(now_time), startTime.hour, startTime.minute, 0) > tmConvert_t(year(now_time), month(now_time), day(now_time), endTime.hour, endTime.minute, 0))
  {
    offMask = onMask >> 1;
  }
  else
  {
    offMask = onMask;
  }
  (void)sync();
}

void DailyTimer::setRandomOffset(int random_minutes, RandomTime randomSetting)
{
  offset = constrain(random_minutes, 0, 59);
  if (offset == 0)
  {
    randomType = FIXED;
  }
  else
  {
    randomType = randomSetting;
  }
}

void DailyTimer::setStartTime(byte hour, byte minute)
{
  startTime.hour = constrain(hour, 0, 23);
  startTime.minute = constrain(minute, 0, 59);
  setDaysActive(onMask);
  (void)sync();
}

void DailyTimer::setEndTime(byte hour, byte minute)
{
  endTime.hour = constrain(hour, 0, 23);
  endTime.minute = constrain(minute, 0, 59);
  setDaysActive(onMask);
  (void)sync();
}


byte DailyTimer::setRandomDays(byte number_Days)
{
  randomSeed(now() + micros());
  byte mask = 0;
  byte array[8] = {0};
  for (int i = 0; i < number_Days; i++)
  {
    array[i] = 1;
  }
  for(int i = 0; i < 7; i++)
  {
    byte index = random(i, 7);
    byte temp = array[i];
    array[i] = array[index];
    array[index] = temp;
  }
  for (int i = 0; i < 7; i++)
  {
    mask |= (array[i] << i);
  }
  onMask = mask << 1;
  (void)sync();
  return onMask;
}

byte DailyTimer::getDays()
{
  return onMask;
}

bool DailyTimer::sync()
{
  bool currentState = isActive(this);
  if(currentState && autoSync)
  {
    startTimeCallback();
  }
  return state = currentState;
}

void DailyTimer::update()
{
  for(int i = 0; i < instanceCount; i++)
  {
    bool lastState = instances[i]->state;
    instances[i]->state = isActive(instances[i]);
    if(lastState != instances[i]->state)
    {
      if(instances[i]->state == true)
      {
        instances[i]->startTimeCallback();
      }
      else
      {
        instances[i]->endTimeCallback();
      }
    }
  }
}

bool DailyTimer::isActive(DailyTimer* instance)
{
  if (instance->currentDay != weekday() && instance->randomType) // once a day, generate new random offsets
  {
    randomSeed(now() + micros());
    if (instance->randomType == RANDOM  || instance->randomType == RANDOM_START)
    {
      int hrs = instance->startTime.hour * 60 + instance->startTime.minute;
      hrs += constrain(random(-1 * instance->offset, instance->offset), 1, (24 * 60) - 1);
      instance->randomStartTime.minute = hrs % 60;
      instance->randomStartTime.hour = hrs / 60;
    }
    if (instance->randomType == RANDOM || instance->randomType == RANDOM_END)
    {
      int hrs = instance->endTime.hour * 60 + instance->endTime.minute;
      hrs += constrain(random(-1 * instance->offset, instance->offset), 1, (24 * 60) - 1);
      instance->randomEndTime.minute = hrs % 60;
      instance->randomEndTime.hour = hrs / 60;
    }
    instance->currentDay = weekday();
  }
  time_t now_time = now();
  time_t on_time = tmConvert_t(year(now_time), month(now_time), day(now_time), (instance->randomType == RANDOM || instance->randomType == RANDOM_START) ? instance->randomStartTime.hour : instance->startTime.hour, (instance->randomType == RANDOM || instance->randomType == RANDOM_START) ? instance->randomStartTime.minute : instance->startTime.minute, /*second(now_time)*/ 0);
  time_t off_time = tmConvert_t(year(now_time), month(now_time), day(now_time), (instance->randomType == RANDOM || instance->randomType == RANDOM_END) ? instance->randomEndTime.hour : instance->endTime.hour, (instance->randomType == RANDOM || instance->randomType == RANDOM_END) ? instance->randomEndTime.minute : instance->endTime.minute, /*second(now_time));*/ 0);
  byte weekDay = weekday(now_time);
  byte today = 0b00000001 << (8 - weekDay);
  if (today & dayTemplate[SUNDAYS])
  {
    today |= 0b00000001;
  }
  if ((today & instance->onMask) && (today & instance->offMask))  // if it is supposed to turn both on and off today
  {
    if (on_time < off_time)
    {
      return (now_time > on_time && now_time < off_time);
    }
    else if (off_time < on_time)
    {
      return (now_time > on_time || now_time < off_time);
    }
    else // if both on and off are set to the same time, I'm confused... so let's do nothing
    {
      return false;
    }
  }
  else if (today & instance->onMask) // if it is supposed to turn only on today
  {
    if (on_time < off_time)
    {
      return (now_time > on_time && now_time < off_time);
    }
    else
    {
      return (now_time > on_time);
    }
  }
  else if (today & instance->offMask)  // if it is supposed to turn only off today
  {
    return now_time < off_time;
  }
  else // if 
  {
    return false;
  }
}

time_t DailyTimer::tmConvert_t(int YYYY, byte MM, byte DD, byte hh, byte mm, byte ss)
{
  tmElements_t tmSet;
  tmSet.Year = YYYY - 1970;
  tmSet.Month = MM;
  tmSet.Day = DD;
  tmSet.Hour = hh;
  tmSet.Minute = mm;
  tmSet.Second = ss;
  return makeTime(tmSet);
}
