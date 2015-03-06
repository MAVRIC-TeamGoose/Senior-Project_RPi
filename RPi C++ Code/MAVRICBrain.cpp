// MAVRIC Brain
// Author: Team Goose
// Requires WiringPi http://wiringpi.com/

/*
 * Todo
 * Expand table to include all 14 data fields
 * Request proximity data
 * Request battery data
 * Request smell data
 * Request sound data
 */

#include <iostream>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <time.h> //For sleeping

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
		sprintf(final_query, "CREATE TABLE %s (Time TIME, Temperature Double)", date);
		if (mysql_query(mysql1, final_query)) {
			fprintf(stderr, "%s\n", mysql_error(mysql1));
			return;
		}
	}
}

void mysql_write_temp(double temperature)
{
     if(mysql1 != NULL)
     {
		char final_query[1024];
		sprintf(final_query, "INSERT INTO %s VALUES(CURTIME(), %f)", date, temperature);
		if (mysql_query(mysql1, final_query)) {
			fprintf(stderr, "%s\n", mysql_error(mysql1));
			return;
		}
     }
}

int writeNumber(unsigned value) {
	return wiringPiI2CWrite(fd, value);
}

int readNumber(void) {
	return wiringPiI2CRead(fd);
}

void requestTemperature(void) {
	int number;
	double temp;
	
	writeNumber(TEMPERATUREDATA); //Request temperature data
	number = readNumber();
	number = number << 8; //Bit shift number by one byte
	number |= readNumber(); //Or number with new value
	temp = 147.5 - ((225.0 * number) / 4095.0);
	temp = ((temp * 9) + 160) / 5.0;
	strftime(current_time, 50, "%r", localtime(&now));
	mysql_write_temp(temp);
}

int main(int argc, const char * argv[]) {
	now = time((time_t*)NULL);
	mysql_connect();
	mysql_create_table(); //Create the mysql table for this test
	fd = wiringPiI2CSetup(address); // Automatically selects i2c on GPIO

	while(1) {
		requestTemperature(); //Request the current Tiva temperature
		sleep(10); //Sleep for 10 seconds
	}
	
	mysql_disconnect(); //Will never reach actually
	
	return 0;
}