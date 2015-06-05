// MAVRIC Brain
// Author: Team Goose
// Requires WiringPi http://wiringpi.com/

#include <iostream>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <time.h> //For sleeping
#include <math.h>

//MySQL Includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <mysql.h>

#define DATABASE_NAME  "mymavricdb"
#define DATABASE_USERNAME "mavric"
#define DATABASE_PASSWORD "mavric"

//*****************************************************************************
//
// Data type macros to send to Tiva before requesting data.
//
//*****************************************************************************
#define TEMPERATUREDATA             0x1

#define PROX1DATA                   0x2
#define PROX2DATA                   0x3
#define PROX3DATA                   0x4
#define PROX4DATA                   0x5
#define PROX5DATA                   0x6
#define PROX6DATA                   0x7
#define PROX7DATA                   0x8
#define PROX8DATA                   0x9

#define BATTDATA                    0xA

#define LEFTSMELLDATA               0xB
#define RIGHTSMELLDATA              0xC

#define LEFTSOUNDDATA               0xD
#define RIGHTSOUNDDATA              0xE

#define MOTORDATA                   0xF

//****************************************************************************
//
// Math Constants for Thermistor
//
//****************************************************************************
float DENOMINATOR = 0.000001790830963;

int B = 3950;

double VIN = 3.3;


MYSQL *mysql1;

const int address = 0x04; // This is the address we setup in the Tiva Program
int fd;
time_t now;
char date[13]; 
char current_time[50];

//*****************************************
//*****************************************
//********** CONNECT TO DATABASE **********
//*****************************************
//*****************************************
void mysql_connect (void)
{
     //initialize MYSQL object for connections
	mysql1 = mysql_init(NULL);

    if(mysql1 == NULL)
    {
		fprintf(stderr, "ABB : %s\n", mysql_error(mysql1));
        return;
    }

    //Connect to the database
    if(mysql_real_connect(mysql1, "mymavricdb.cqgj91he9tya.us-west-2.rds.amazonaws.com", DATABASE_USERNAME, DATABASE_PASSWORD, DATABASE_NAME, 3306, NULL, 0) == NULL)
    {
		fprintf(stderr, "%s\n", mysql_error(mysql1));
    }
    else
    {
		printf("Database connection successful.\r\n");
    }
	mysql_select_db(mysql1, DATABASE_NAME); //Select mavricdb as default database
	mysql_query(mysql1, "SET time_zone = 'America/Los_Angeles'"); //Set CURTIME to system time
}

//**********************************************
//**********************************************
//********** DISCONNECT FROM DATABASE **********
//**********************************************
//**********************************************
void mysql_disconnect (void)
{
    mysql_close(mysql1);
    printf( "Disconnected from database.\r\n");
}

void mysql_create_table(void)
{
	strftime(date, 13, "%d_%b_%G", localtime(&now)); //Grab date

	if(mysql1 != NULL)
	{	
		char final_query[1024];
		sprintf(final_query, "CREATE TABLE %s (Time TIME, Temperature Double, Prox1 int, Prox2 int, Prox3 int, Prox4 int, Prox5 int, Prox6 int, Prox7 int, LSmellGood int, LSmellBad int, RSmellGood int, RSmellBad int, LSoundGood int, LSoundBad int, RSoundGood int, RSoundBad int, BattLevel int)", date);
		if (mysql_query(mysql1, final_query)) {
			fprintf(stderr, "%s\n", mysql_error(mysql1));
			return;
		}
	}
}

void mysql_write_data(double temperature, int prox1, int prox2, int prox3, int prox4, int prox5, int prox6, int prox7, int LSmellGood, int LSmellBad, int RSmellGood, int RSmellBad, int LSoundGood, int LSoundBad, int RSoundGood, int RSoundBad, int battery)
{
	if(mysql1 != NULL)
	{
		char final_query[1024];
		sprintf(final_query, "INSERT INTO %s (Time, Temperature, Prox1, Prox2, Prox3, Prox4, Prox5, Prox6, Prox7, LSmellGood, LSmellBad, RSmellGood, RSmellBad, LSoundGood, LSoundBad, RSoundGood, RSoundBad, BattLevel) VALUES(CURTIME(), %f, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)", date, temperature, prox1, prox2, prox3, prox4, prox5, prox6, prox7, LSmellGood, LSmellBad, RSmellGood, RSmellBad, LSoundGood, LSoundBad, RSoundGood, RSoundBad, battery);
		if(mysql_query(mysql1, final_query))
		{
			fprintf(stderr, "%s\n", mysql_error(mysql1));
			return;
		}
	}
}

int writeNumber(unsigned value) 
{
	return wiringPiI2CWrite(fd, value);
}

int readNumber(void) 
{
	return wiringPiI2CRead(fd);
}

double requestTemperature(void) 
{
	int number;
	double temp;
	
	writeNumber(TEMPERATUREDATA); //Request temperature data
	number = readNumber();
	number = number << 8; //Bit shift number by one byte
	number |= readNumber(); //Or number with new value
	temp = (double) B / log((((double) VIN / (number * 3.3 / 4096)) - 1) / DENOMINATOR); //Temp in Kelvin
	temp = (temp - 273.15) * 1.8 + 32;
	printf("\nTemp is %f", temp);
}

int requestBattData()
{
	int battery;
	
	writeNumber(BATTDATA); //Request battery data
	battery = readNumber();
	battery = battery << 8;
	battery |= readNumber();
	printf("\nBatt is %d", battery);
	return battery;	
}

int requestProx(int sensor) 
{
	int proximity;
	
	writeNumber(sensor); //Send the appropriate proximity constant
	proximity = readNumber();
	proximity = proximity << 8; //Bit shift number by one byte
	proximity |= readNumber(); //Or proximity value with new value
	printf("\nProx is %d", proximity);
	return proximity;
}

int requestGoodAudio(int sensor)
{
	int goodAudio; //Bucket 1 or 3
	writeNumber(sensor); //Send the appropriate audio constant
	
	goodAudio = (readNumber() << 24);
	goodAudio |= (readNumber() << 16);
	goodAudio |= (readNumber() << 8);
	goodAudio |= readNumber();
	
	printf("\nGood Audio is %d", goodAudio);
	
	return goodAudio;
}

int requestBadAudio()
{
	int badAudio; //Bucket 2 or 4
	
	badAudio = (readNumber() << 24);
	badAudio |= (readNumber() << 16);
	badAudio |= (readNumber() << 8);
	badAudio |= readNumber();
	
	printf("\nBad Audio is %d", badAudio);
	
	return badAudio;
	
}

int main(int argc, const char * argv[]) 
{
	now = time((time_t*)NULL);
	mysql_connect();
	mysql_create_table(); //Create the mysql table for this test	
	
	fd = wiringPiI2CSetup(address); // Automatically selects i2c on GPIO
	int proximity[8] = {0}; //Make array of proximity values initialized to 0
	int audioData[8] = {0}; //Make array of audio values initialized to 0
	
	double temp;
	int batterylevel;
	
	int sendData = 0;

	while(1) {
		temp = requestTemperature(); //Request the current Tiva temperature (2 bytes)
		proximity[0] = requestProx(PROX1DATA);
		proximity[1] = requestProx(PROX2DATA);
		proximity[2] = requestProx(PROX3DATA);
		proximity[3] = requestProx(PROX4DATA);
		proximity[4] = requestProx(PROX5DATA);
		proximity[5] = requestProx(PROX6DATA);
		proximity[6] = requestProx(PROX7DATA);
		
		audioData[0] = requestGoodAudio(LEFTSMELLDATA);
		audioData[1] = requestBadAudio();
		
		audioData[2] = requestGoodAudio(RIGHTSMELLDATA);
		audioData[3] = requestBadAudio();
		
		audioData[4] = requestGoodAudio(LEFTSOUNDDATA);
		audioData[5] = requestBadAudio();
		
		audioData[6] = requestGoodAudio(RIGHTSOUNDDATA);
		audioData[7] = requestBadAudio();
		
		batterylevel = requestBattData();
		sendData++;
		
		if (sendData >= 50){
			sendData = 0;
			//mysql_write_data(temp, proximity[0], proximity[1], proximity[2], proximity[3], proximity[4], proximity[5], proximity[6], batterylevel);
			mysql_write_data(temp, proximity[0], proximity[1], proximity[2], proximity[3], proximity[4], proximity[5], proximity[6], audioData[0], audioData[1], audioData[2], audioData[3], audioData[4], audioData[5], audioData[6], audioData[7], batterylevel);
		}
		usleep(100000); //Sleep for 0.1 seconds
	}
	mysql_disconnect(); //Will never reach actually
	
	return 0;
}