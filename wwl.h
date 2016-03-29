/*
 * Program: wwl
 * 
 * This program combines two handy hamradio Maindensquare programs into one.
 * When used as locator, it will take the Maindenhead square on the
 * command line and write it back out as lat / long.
 * When used as wwl, it will calculate distance and azimuth
 * between the two Maidenhead squares given.
 * If only four characters of the Maidenhead square is given, this
 * program will auto fill in the missing two chars with 'AA'
 *
 * This version by va3db db@db.net db@FreeBSD.org Oct 1 2007
 * rewritten completely. There were equator crossing bugs in the original
 * (dead) version of wwl on sunsite, so I rewrote it from scratch.
 *
 * wwl     -- Originally by IK0ZSN Mirko Caserta <ik0zsn@amsat.org>
 * locator -- originally written by Harald M.
 * Stauss harald.stauss@web.de DO1JHS @ DB0GR.#BLN.DEU.EU
 * There is no code from the original (dead?) version of wwl or
 * the original (dead?) version of locator in this version.
 *
 * The bearing/distance code is from Amateur Radio Software by John
 * Morris, GM4ANB.
 *
 * <db@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this code, except you may not
 * license it under any form of the GPL.
 * A postcard or QSL card showing me you appreciate
 * this code would be nice. Diane Bruce va3db
 */

#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#define SCRATCH_WWL_LEN 9
#define WWL_LEN		6
#define EARTHRADIUS	6371.33

volatile char *rcs="$Id: wwl.c,v 1.12 2007/10/15 19:53:08 db Exp db $";

struct location {
  double latitude;
  double longitude;
};

static int is_valid_locator(const char wwl[]);
static struct location convert_locator(char *wwl);
static void upstring(char *s);
static void bearing_dist(struct location *my_location,
	struct location *dx_location, int *dist, int *bearing);
static double rad_to_deg(double radians);
static double deg_to_rad(double degrees);

//static int locator = 0;	/* Doing locator instead of wwl ? */


//main (int argc, char **argv)
int distance (char *dx, char *my)
{
	int l, p;
	struct location my_location, dx_location;
	char my_wwl[SCRATCH_WWL_LEN], dx_wwl[SCRATCH_WWL_LEN];
	snprintf(my_wwl, sizeof(my_wwl), "%sAA", my);
	my_wwl[WWL_LEN] ='\0';
	my_location = convert_locator(my_wwl);
	if(!is_valid_locator(my_wwl)) {
		fprintf(stderr, "%s: not a valid locator\n", my_wwl);
		//exit(EXIT_FAILURE);
	}
	snprintf(dx_wwl, sizeof(dx_wwl), "%sAA", dx);
	dx_wwl[WWL_LEN] ='\0';
	dx_location = convert_locator(dx_wwl);
	if (!is_valid_locator(dx_wwl)) {
		printf("%s: not a valid locator\n", dx_wwl);
		//exit(EXIT_FAILURE);
	}
	bearing_dist(&my_location, &dx_location, &p, &l);
	//printf("qrb: %d kilometers, azimuth: %d degrees\n", p, l);
    return p;
	//exit(EXIT_SUCCESS);
}

/*
 * is_valid_locator
 * check for valid locator
 *
 * inputs	- string to locator
 * output	- 1 if valid locator 0 if not
 * side effects	-
 */

static int
is_valid_locator(const char wwl[])
{
	if (strlen(wwl) != WWL_LEN)
		return 0;

	if (wwl[0] < 'A' || wwl[0] > 'R' ||
	    wwl[1] < 'A' || wwl[1] > 'R' ||
	    wwl[2] < '0' || wwl[2] > '9' ||
	    wwl[3] < '0' || wwl[3] > '9' ||
	    wwl[4] < 'A' || wwl[4] > 'X' ||
	    wwl[5] < 'A' || wwl[5] > 'X' ) 
		return(0);
	else
		return(1);
}

/*
 * convert_locator
 *
 * inputs	- string to convert
 * output	- return a struct location
 * side effects	- none
 */

static struct location
convert_locator(char *wwl)
{
	struct location loc;
	upstring(wwl);

	loc.latitude = (double)(wwl[1] - 'A') * 10 - 90 +
		(double)(wwl[3] - '0') + (double)(wwl[5] - 'A') / 24 + 1 / 48;
	loc.latitude = deg_to_rad(loc.latitude);
	loc.longitude = (double)(wwl[0] - 'A') * 20. - 180. +
		(double)(wwl[2] - '0') * 2 +
		(double)(wwl[4] - 'A') / 12 + 1 / 24;
	loc.longitude = deg_to_rad(loc.longitude);
	return(loc);
}

/*
 * upstring
 * convert string to upper case
 *
 * inputs	- string to convert to upper case modified in place
 */
static void
upstring(char *s)
{
	while(*s != '\0') {
		*s = toupper(*s);
		s++;
	}
}

/*
 * Convert degrees to radians
 *
 * input	- degrees
 * output	- radians
 * side effects	- none
 */
static double
deg_to_rad(double degrees)
{
	return(degrees/180.) * M_PI;
}

/*
 * Convert radians to degrees
 *
 * input	- radians
 * output	- degrees
 * side effects	- none
 */
static double
rad_to_deg(double radians)
{
	return((radians/M_PI) * 180.);
}

/*
 *
 * Given location of start and location of end, calculate
 * bearing and azimuth
 *
 * inputs	- pointer to my location
 *		- pointer to dx location
 * 		- pointer to result for dist
 *		- pointer to result for bearing
 * output	- dist and bearing as integer
 * side effects	- none
 */
static void
bearing_dist(struct location *my_location, struct location *dx_location,
	     int *dist, int *bearing)
{
	double co, he, e, hn, n, ca, az;

	hn = my_location->latitude;
	he = my_location->longitude;
	n = dx_location->latitude;
	e = dx_location->longitude;

	co = cos(he - e) * cos(hn) * cos(n) + sin(hn) * sin(n);
	ca = atan2(sqrt(1 - pow(co,2)), co); 
	az = atan2(sin(e - he) * cos(n) * cos(hn), sin(n) - sin(hn) * cos(ca));
	if( az < 0)
		az += 2 * M_PI;

	/* Round up azimuth */
	*bearing = (double)((int)((rad_to_deg(az) * 10.) + 5))/10;
	/* Round up distance */
	*dist = (double)((int)(EARTHRADIUS * ca * 10. + 5))/10;
}
