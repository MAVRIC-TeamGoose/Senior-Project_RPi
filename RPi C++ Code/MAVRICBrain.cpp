// MAVRIC Brain
// Author: Team Goose
// Requires WiringPi http://wiringpi.com/

/*
 * Todo
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

#define MOTORDATA                   0xF

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
		sprintf(final_query, "CREATE TABLE %s (Time TIME, Temperature Double, Prox1 int, Prox2 int, Prox3 int, Prox4 int, Prox5 int, Prox6 int, Prox7 int, Prox8 int, BattLevel int, LeftMotor Int, RightMotor Int)", date);
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
		sprintf(final_query, "INSERT INTO %s (Time, Temperature) VALUES(CURTIME(), %f)", date, temperature);
		if (mysql_query(mysql1, final_query)) 
		{
			fprintf(stderr, "%s\n", mysql_error(mysql1));
			return;
		}
    }
}

void mysql_write_prox(int sensor, int proximity)
{
	if(mysql1 != NULL)
	{
		char final_query[1024];
		sprintf(final_query, "INSERT INTO %s (Time, Prox%d) VALUES(CURTIME(), %d)", date, (sensor - 1), proximity);
		if(mysql_query(mysql1, final_query))
		{
			fprintf(stderr, "%s\n", mysql_error(mysql1));
			return;
		}
	}
}

void mysql_write_motor(char leftMotor, char rightMotor)
{
	if(mysql1 != NULL)
	{
		char final_query[1024];
		sprintf(final_query, "INSERT INTO %s (Time, LeftMotor, RightMotor) VALUES(CURTIME(), %d, %d)", date, leftMotor, rightMotor);
		if(mysql_query(mysql1, final_query))
		{
			fprintf(stderr, "%s\n", mysql_error(mysql1));
			return;
		}
	}
}

void mysql_write_data(double temperature, int prox1, int prox2, int prox3, int prox4, int prox5, int prox6, int prox7, int prox8, int battery)
{
	if(mysql1 != NULL)
	{
		char final_query[1024];
		sprintf(final_query, "INSERT INTO %s (Time, Temperature, Prox1, Prox2, Prox3, Prox4, Prox5, Prox6, Prox7, Prox8, BattLevel) VALUES(CURTIME(), %f, %d, %d, %d, %d, %d, %d, %d, %d, %d)", date, temperature, prox1, prox2, prox3, prox4, prox5, prox6, prox7, prox8, battery);
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
	temp = 147.5 - ((225.0 * number) / 4095.0);
	temp = ((temp * 9) + 160) / 5.0;
	printf("\nTemp is %f", temp);
	//mysql_write_temp(temp);
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
	//mysql_write_prox(sensor, proximity);	
	return proximity;
}

void sendMotorCommand(char leftMotor, char rightMotor)
{
	//Motor value ranges from -100 to 100
	writeNumber(MOTORDATA); //Inform Tiva that motor data is about to be sentry
	writeNumber(leftMotor);
	writeNumber(rightMotor);
	mysql_write_motor(leftMotor, rightMotor);
}

int main(int argc, const char * argv[]) 
{
	now = time((time_t*)NULL);
	mysql_connect();
	mysql_create_table(); //Create the mysql table for this test
	fd = wiringPiI2CSetup(address); // Automatically selects i2c on GPIO
	int proximity[8] = {0}; //Make array of proximity values initialized to 0
	
	double temp;
	int batterylevel;

	while(1) {
		temp = requestTemperature(); //Request the current Tiva temperature (2 bytes)
		proximity[0] = requestProx(PROX1DATA);
		proximity[1] = requestProx(PROX2DATA);
		proximity[2] = requestProx(PROX3DATA);
		proximity[3] = requestProx(PROX4DATA);
		proximity[4] = requestProx(PROX5DATA);
		proximity[5] = requestProx(PROX6DATA);
		proximity[6] = requestProx(PROX7DATA);
		proximity[7] = requestProx(PROX8DATA);
		requestBattData();
		mysql_write_data(temp, proximity[0], proximity[1], proximity[2], proximity[3], proximity[4], proximity[5], proximity[6], proximity[7], batterylevel);
		sleep(10);
	}
	
	mysql_disconnect(); //Will never reach actually
	
	return 0;
}