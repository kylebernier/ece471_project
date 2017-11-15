#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/ioctl.h>

#include <linux/i2c-dev.h>


#define I2C_DEVICE "/dev/i2c-1"
#define DISPLAY_ADDR 0x70
#define NUNCHUCK_ADDR 0x52

int init_i2c_comm();
int init_i2c_display();
int init_i2c_nunchuck();
int init_game();

int running = 0;

int main()
{
	// Initialize I2C Nunchuck and Display
	init_i2c_comm();

	// Initialize game
	init_game();

	return 0;
}

int init_i2c_comm()
{
	int fd;
	
	// Open the I2C device
	fd = open(I2C_DEVICE, O_RDWR);
	// Handle errors
	if (fd < 0) {
		fprintf(stderr, "Error opeing I2C device!\n");
		close(fd);
		return -1;
	}

	// Initialize the display
	if (init_i2c_display(fd)) return -1;

	// Initialize the nunchuck
	if (init_i2c_nunchuck(fd)) return -1;

	return 0;
}

int init_i2c_display(const int fd)
{
	// Set the slave address
	
	// Turn on the oscillator
	
	// Set the brightness
	

	return 0;
}

int init_i2c_nunchuck(const int fd)
{

	return 0;
}

int init_game()
{

	return 0;
}
