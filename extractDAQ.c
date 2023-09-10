#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>

#define RET(i) (edges[i][0] & TMCcountMask)
#define REN(i) (edges[i][0] & NewEventMask) >> 7
#define REV(i) (edges[i][0] & Val_EdgeMask) >> 5

#define FET(i) (edges[i][1] & TMCcountMask)
#define FEN(i) (edges[i][1] & NewEventMask) >> 7
#define FEV(i) (edges[i][1] & Val_EdgeMask) >> 5

// #define CPLDns_RES 24.0d //  CPLD Clock Resolution:  ns between clock "ticks" on Old DAQ board
#define CPLDns_RES 40.0d //  CPLD Clock Resolution:  ns between clock "ticks" on New DAQ board

 #define D 40    //  Trigger Delay in ns
 #define W 10240 //  Trigger Width in ns
// #define D 60    //  Trigger Delay in ns used in Exanple D of Appendix E
// #define W 160   //  Trigger Width in ns used in Exanple D of Appendix E

#define MAXPPC 10 //  Maximum possible Pulses per Channel per Event

char DAQfilename[256], inputline[128];
FILE *DAQfile;
int stillreading=1;

void ReadLine(), DumpEventPulses(), dateof();
int utimeof();

uint32_t ClockCount;
uint8_t edges[4][2];
uint32_t lastGPS, EventStartime, Pulse_ns[MAXPPC][4][2];
int hh, mm, day, mon, year;
int ehh, emm, eday, emon, eyear;
double sec, esec;
char GPSok[2];
int satcount;
uint8_t DAQstatus;
int GPSoffset, np[4], nev=0;

uint8_t TMCcountMask=0x1F, \
        NewEventMask=0x80, \
        Val_EdgeMask=0x20;

int main(int argc, char **argv)
{
  int i, j, npf, FirstEvent=1, FirstPulse;
  uint32_t EventCntSt, Fall_ns;
  double TMCns_Res;
  int Eventsec, isec, UTofEvent;

  TMCns_Res=CPLDns_RES/32.0d; //  Note:  TMC "edge rise/fall counter" has 5 bits
                              //         and 32 = 2^5.

  DAQfile=stdin;
  if(argc > 1) //  Use filename supploed as command line argument instead of stdon
  {
    sscanf(argv[1],"%s", DAQfilename);
    DAQfile=fopen(DAQfilename,"r");
  }
  ReadLine();
  while(!REN(0) && stillreading)ReadLine();
  while(stillreading)
  {
    if(!FirstEvent)ReadLine();
    if(REN(0))
    {
      if(!FirstEvent || !stillreading) DumpEventPulses();
      Eventsec = (int) (sec + (double)GPSoffset / 1000.0d + 0.5d);
      UTofEvent=utimeof(2000+year, mon, day, hh, mm, Eventsec); // Unix(UTC) Time of start of Event 
      dateof(UTofEvent, &eyear, &emon, &eday, \
                        &ehh, &emm, &isec); //  Compute correct year/month/day/hour/minute
      esec=(double)isec;                    //  in case Eventsec < 0 or >= 60.
      EventCntSt=ClockCount;
      for(i=0;i<4;i++)
      {
        np[i]=-1;
        for(j=0;j<MAXPPC;j++)Pulse_ns[j][i][1]=0;
      }
      FirstPulse=1;
    }
    for(i=0;i<4;i++)
    {
      if(REV(i))
      {
        np[i]++;
        if(np[i] == MAXPPC)
        {
          printf("\nMore than %d(=MAXPPC) pulses for channel %d\n\n",MAXPPC,i);
          exit(1);
        }
        Pulse_ns[np[i]][i][0] = (uint32_t)((double)(ClockCount - EventCntSt))*CPLDns_RES + \
                                (uint32_t)((double)RET(i)*TMCns_Res + 0.5d);
        if(FirstPulse) EventStartime=(uint32_t)((double)(ClockCount - lastGPS))*CPLDns_RES;
        FirstPulse=0;
      }
      if(FEV(i))
      {
        Fall_ns = (uint32_t)((double)(ClockCount - EventCntSt))*CPLDns_RES + \
                  (uint32_t)((double)FET(i) * TMCns_Res + 0.5d);
        npf=np[i];
        if(np[i]>0)if(Pulse_ns[np[i]-1][i][1] == 0) npf=np[i]-1;
        Pulse_ns[npf][i][1] = Fall_ns;
      }
    }
    if(!stillreading) DumpEventPulses();
    FirstEvent=0;
  }
  exit(0);
}

void ReadLine()
{
  int i, nl, tnl;

  fscanf(DAQfile,"%[^\r\n]\n",inputline); //  Handle lines with and wothout "return" character (\r)
  sscanf(inputline,"%X%n", &ClockCount, &tnl);
  for(i=0; i<4; i++)
  {
    sscanf(&inputline[tnl],"%hhX %hhX%n",&edges[i][0],&edges[i][1],&nl); //  Note "hh" for 8-bit integers
    tnl+=nl;
  } 
  sscanf(&inputline[tnl],"%X %2d%2d%6lf %2d%2d%2d %s %d %hhX %d", \
         &lastGPS, &hh, &mm, &sec, &day, &mon, &year, GPSok, &satcount, &DAQstatus, &GPSoffset);

  stillreading=!(feof(DAQfile) || ferror(DAQfile));
}

void DumpEventPulses()
{
  int i, j;
  for(i=0;i<4 && Pulse_ns[0][i][1] == 0;i++);
  if(i < 4)
  {
    nev++; //  Event count used at this stage, but it's made here just in case . . .
    j=0;
    printf("Pulses of Event: %02d-%02d-%02d %02d:%02d:%02.0f.%09u Ch-%d Pls-%d %10u %10u %5u\n", \
      eday, emon, eyear, ehh, emm, esec, EventStartime, \
      i, j, Pulse_ns[j][i][0], Pulse_ns[j][i][1], Pulse_ns[j][i][1] - Pulse_ns[j][i][0]);
    for(j=1;j<np[i]+1;j++)
    {
      if(Pulse_ns[j][i][1] > 0) printf( \
         "                                               Ch-%d Pls-%d %10u %10u %5u\n", \
         i, j, Pulse_ns[j][i][0], Pulse_ns[j][i][1], Pulse_ns[j][i][1] - Pulse_ns[j][i][0]);
    }
    for(i=i++;i<4;i++)
    {
      for(j=0;j<np[i]+1;j++)
      {
        if(Pulse_ns[j][i][1] > 0) printf( \
           "                                               Ch-%d Pls-%d %10u %10u %5u\n", \
           i, j, Pulse_ns[j][i][0], Pulse_ns[j][i][1], Pulse_ns[j][i][1] - Pulse_ns[j][i][0]);
      }
    }
  }
}

// returns unix time for parameters year, month, day, hour, min, sec
int utimeof(year, month, day, hour, min, sec)
int year, month, day, hour, min, sec;
{
  static time_t unixtime_t,zerotime_t=(time_t)0;
  struct tm *p_tm,time_buf;
  p_tm = &time_buf;
  p_tm->tm_year=year-1900;
  p_tm->tm_mon=month-1;
  p_tm->tm_mday=day;
  p_tm->tm_hour=hour;
  p_tm->tm_min=min;
  p_tm->tm_sec=sec;
  p_tm->tm_isdst=-1;
  unixtime_t=mktime(p_tm)-mktime(gmtime(&zerotime_t));
  return (int)unixtime_t+(p_tm->tm_isdst+hour-p_tm->tm_hour)*3600;  // Note prior call to mktime curcial to set p_tm->tm_isdst
                                                                    // However Spring DST jump causes headaches !!
}

// Updates *year, *month, *day, *hour, *min, *sec with correct values corresponding to unix time parameter
void dateof(unixtime, year, month, day, hour, min, sec)
int unixtime, *year, *month, *day, *hour, *min, *sec;
{
  time_t unixtime_t;
  struct tm *p_tm,time_buf;
  p_tm = &time_buf;
  unixtime_t=(time_t)unixtime;
  p_tm=gmtime(&unixtime_t);
  year[0]=1900+p_tm->tm_year;
  month[0]=1+p_tm->tm_mon;
  day[0]=p_tm->tm_mday;
  hour[0]=p_tm->tm_hour;
  min[0]=p_tm->tm_min;
  sec[0]=p_tm->tm_sec;
}
