/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * File: snake.c                                           *
 * Description: Play a game of snake                       *
 * Authors: Kyle and Connor                                *
 * Date: November 2017                                     *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define I2C_DEVICE "/dev/i2c-1"

#define DISPLAY_ADDR 0x71
#define SCORE_ADDR 0x70
#define NUNCHUCK_ADDR 0x52

#define LED_GREEN 1
#define LED_RED 2
#define LED_YELLOW 3
#define LED_OFF 0

int init_i2c_comm();
int init_i2c_display(const int);
int init_i2c_scoreboard(const int);
int init_i2c_nunchuck(const int);
int get_nunchuck(const int, char*);
int init_game();
int start_game();
int move_snake(int);
int snake_bigger();
int snake_smaller();
uint8_t get_x(uint16_t);
uint8_t get_y(uint16_t);
uint16_t combine_xy(uint8_t, uint8_t);
int detect_collision();
int detect_fruit();

int running = 0;

char game_display[8][8] = {{0}};

uint16_t displaybuffer[8];

// Snake parameters
uint16_t snake_position[64] = {{0}};
int snake_direction;
int snake_size;

uint16_t fruit_position;

int fd;

int main()
{
	// Initialize I2C communication
	init_i2c_comm();

	// Initialize game
	init_game();

	// Start game
	start_game();

	return 0;
}

/* Initialize I2C communication */
int init_i2c_comm()
{	
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

	// Initialize the scoreboard
	if (init_i2c_scoreboard(fd)) return -1;

	// Initialize the nunchuck
	if (init_i2c_nunchuck(fd)) return -1;

	return 0;
}

/* Initialize the I2C display */
int init_i2c_display(const int fd)
{
	int result;
	int i;

	unsigned char buffer[17];

	// Set the slave address
	result = ioctl(fd, I2C_SLAVE, DISPLAY_ADDR);
	if (result < 0) {
		fprintf(stderr, "Error setting the display slave address!\n");
		close(fd);
		return -1;
	}

	// Turn on the oscillator
	buffer[0] = (0x2 << 4) | 0x1;
	result = write(fd, buffer, 1);
	if (result < 0) {
		fprintf(stderr, "Error turing on the display occillator!\n");
		close(fd);
		return -1;
	}

	// Turn on Display, No blick
	buffer[0] = (0x2 << 6) | 0x1;
	result = write(fd, buffer, 1);
	if (result < 0) {
		fprintf(stderr, "Error turning on the display!\n");
		close(fd);
		return -1;
	}
	
	// Set the brightness
	buffer[0] = (0x7 << 5) | 0xd;
	result = write(fd, buffer, 1);
	if (result < 0) {
		fprintf(stderr, "Error setting display brightness!\n");
		close(fd);
		return -1;
	}

	// Clear the display
	for (i = 0; i <= 16; i++) {
		buffer[i] = 0x00;
	}
	result = write(fd, buffer, 17);
	if (result < 0) {
		fprintf(stderr, "Error clearing the display!\n");
		close(fd);
		return -1;
	}

	return 0;
}

/* Initialize the I2C scoreboard */
int init_i2c_scoreboard(const int fd)
{
	int result;
	int i;

	unsigned char buffer[17];

	// Set the slave address
	result = ioctl(fd, I2C_SLAVE, SCORE_ADDR);
	if (result < 0) {
		fprintf(stderr, "Error setting the score slave address!\n");
		close(fd);
		return -1;
	}

	// Turn on the oscillator
	buffer[0] = (0x2 << 4) | 0x1;
	result = write(fd, buffer, 1);
	if (result < 0) {
		fprintf(stderr, "Error turing on the score occillator!\n");
		close(fd);
		return -1;
	}

	// Turn on Display, No blick
	buffer[0] = (0x2 << 6) | 0x1;
	result = write(fd, buffer, 1);
	if (result < 0) {
		fprintf(stderr, "Error turning on the score!\n");
		close(fd);
		return -1;
	}
	
	// Set the brightness
	buffer[0] = (0x7 << 5) | 0xd;
	result = write(fd, buffer, 1);
	if (result < 0) {
		fprintf(stderr, "Error setting score brightness!\n");
		close(fd);
		return -1;
	}

	// Clear the display
	for (i = 0; i <= 16; i++) {
		buffer[i] = 0x00;
	}
	result = write(fd, buffer, 17);
	if (result < 0) {
		fprintf(stderr, "Error clearing the score!\n");
		close(fd);
		return -1;
	}
	
	return 0;
}

/* Initialize the I2C nunchuck */
int init_i2c_nunchuck(const int fd)
{
	int result;

	unsigned char buffer[17];

	// Set the slave address
	result = ioctl(fd, I2C_SLAVE, NUNCHUCK_ADDR);
	if (result < 0) {
		fprintf(stderr, "Error setting the nunchuck slave address!\n");
		close(fd);
		return -1;
	}

	// Send setup byte 0x40
	buffer[0] = 0x40;
	result = write(fd, buffer, 1);
	if (result < 0) {
		fprintf(stderr, "Error sending handshake to nunchuck!\n");
		close(fd);
		return -1;
	}

	// Send setup byte 0x00
	buffer[0] = 0x00;
	result = write(fd, buffer, 1);
	if (result < 0) {
		fprintf(stderr, "Error sending handshake to nunchuck!\n");
		close(fd);
		return -1;
	}
	
	return 0;
}

int write_score(const int fd, int score) {
	int result;
	int i;

	unsigned char buffer[17];
	
	// Set the slave address
	result = ioctl(fd, I2C_SLAVE, SCORE_ADDR);
	if (result < 0) {
		fprintf(stderr, "Error setting the score slave address!\n");
		close(fd);
		return -1;
	}
	
	// Clear the buffer
	for (i = 0; i <= 16; i++) {
		buffer[i] = 0x00;
	}

	buffer[1] = 0x79;
	buffer[3] = 0x39;
	buffer[7] = 0x79;
	write(fd, buffer, 17);


	return 0;
}

/* Write a pixel to the display buffer */
int write_pixel(uint16_t x, uint16_t y, int color) {
	if ((y < 0) || (y >= 8)) return -1;
	if ((x < 0) || (y >= 8)) return -1;

	if (color == LED_GREEN) {
		// Turn on green LED
		displaybuffer[y] |= 1 << x;
		// Turn off red LED
		displaybuffer[y] &= ~(1 << (x + 8));
	} else if (color == LED_RED) {
		// Turn on red LED
		displaybuffer[y] |= 1 << (x + 8);
		// Turn off green LED
		displaybuffer[y] &= ~(1 << x);
	} else if (color == LED_YELLOW) {
		// Turn on green and red LED
		displaybuffer[y] |= (1 << (x + 8)) | (1 << x);
	} else if (color == LED_OFF) {
		// Turn off green and red LED
		displaybuffer[y] &= ~(1 << x) & ~(1 << (x + 8));
	}

	return 0;
}

int write_display(const int fd) {
	int result;
	int i;

	unsigned char buffer[17];

	// Set the slave address
	result = ioctl(fd, I2C_SLAVE, DISPLAY_ADDR);
	if (result < 0) {
		fprintf(stderr, "Error setting the display slave address!\n");
		close(fd);
		return -1;
	}
	
	// Clear the buffer
	for (i = 0; i <= 16; i++) {
		buffer[i] = 0x00;
	}
	result = write(fd, buffer, 1);

	if (result < 0) {
		fprintf(stderr, "Error writing to display!\n");
		close(fd);
		return -1;
	}

	for (i = 0; i < 8; i++) {
		buffer[0] = displaybuffer[i] & 0xFF;
		result = write(fd, buffer, 1);
		if (result < 0) {
			fprintf(stderr, "Error writing to display!\n");
			close(fd);
			return -1;
		}
		
		buffer[0] = displaybuffer[i] >> 8;
		result = write(fd, buffer, 1);
		if (result < 0) {
			fprintf(stderr, "Error writing to display!\n");
			close(fd);
			return -1;
		}
	}

	return 0;	
}

int get_nunchuck(const int fd, char* data) {
	int result;

	unsigned char buffer[17];
	
	// Set the slave address
	result = ioctl(fd, I2C_SLAVE, NUNCHUCK_ADDR);
	if (result < 0) {
		fprintf(stderr, "Error setting the nunchuck slave address!\n");
		close(fd);
		return -1;
	}
	
	// Send request byte 0x00
	buffer[0] = 0x00;
	result = write(fd, buffer, 1);
	if (result < 0) {
		fprintf(stderr, "Error requesting nunchuck data!\n");
		close(fd);
		return -1;
	}

	// Read the data
	result = read(fd, data, 6);
	if (result < 0) {
		fprintf(stderr, "Error reading nunchuck data!\n");
		close(fd);
		return -1;
	}

	return 0;
}

/* Initialize the game */
int init_game()
{
	// Do stuff
	snake_size = 2;
	snake_direction = 0;
	snake_position[0] = combine_xy(4, 7);
	snake_position[1] = combine_xy(4, 8);

	fruit_position = 0;

	return 0;
}

/* Start the game */
int start_game()
{
	int i, j;

	int result;

	char rand_x[2];
	char rand_y[2];

	uint8_t fruit_x;
	uint8_t fruit_y;

	int fruit_found;

	int fruit;

	int skip_last;

	int random = open("/dev/urandom", O_RDONLY);
	if (random < 0) {
		return -1;
	}

	// Set running to true
	running = 1;
		
	printf("The game is running!\n");

	fruit = 0;
	fruit_x = 0;
	fruit_y = 0;

	skip_last = 0;

	// Start the game loop
	while (running) {
		usleep(500000);

		fruit_found = 0;

		for (i = 0; i < snake_size; i++) {
			uint16_t pos = snake_position[i];
			printf("Segment %d: [%d, %d]\n", i, get_x(pos), get_y(pos));
			write_pixel(get_x(pos) - 1, get_y(pos) - 1, 3);
		}
		write_display(fd);
		write_score(fd, 4);
	
		move_snake(skip_last);
		printf("Snake Moved!\n");

		if(detect_collision() == 1) {
			printf("CRASH --- Game Over!\n");
			break;
		}

		if(detect_fruit()) {
			fruit = 0;
			snake_bigger();
			skip_last = 1;
			printf("Fruit Eaten!\n");
		} else {
			skip_last = 0;
		}
		
		for (i = 0; i < 7; i++) {

			result = read(random, rand_x, 1);
			result = read(random, rand_y, 1);

			if (!fruit_found && !fruit) {
				fruit_x = rand_x[0] % 8 + 1;
				fruit_y = rand_y[0] % 8 + 1;
			}

			for (j = 0; j < snake_size; j++) {
				if (combine_xy(fruit_x, fruit_y) == snake_position[j]) {
					if (!fruit_found && !fruit) {
						fruit_x = 0;
						fruit_y = 0;
					}
				}
			}

			if (fruit_x != 0 && fruit_y != 0) {
				fruit_found = 1;
				fruit = 1;
				fruit_position = combine_xy(fruit_x, fruit_y);
			}
		}

		printf("Fruit: [%d, %d]\n\n", fruit_x, fruit_y);
	}

	return 0;
}

int move_snake(int skip)
{
	int i;

	uint16_t next_pos;

	uint16_t prev_pos = snake_position[0];

	switch (snake_direction) {
		case 0:
			snake_position[0] = combine_xy(get_x(prev_pos), get_y(prev_pos) - 1);
			break;
		case 1:
			snake_position[0] = combine_xy(get_x(prev_pos) + 1, get_y(prev_pos));
			break;
		case 2:
			snake_position[0] = combine_xy(get_x(prev_pos), get_y(prev_pos) + 1);
			break;
		case 3:
			snake_position[0] = combine_xy(get_x(prev_pos) - 1, get_y(prev_pos));
			break;
		default:
			return -1;
	}

	for (i = 1; i < snake_size - skip; i++) {
		next_pos = snake_position[i];	
		snake_position[i] = prev_pos;
		prev_pos = next_pos;
	}

	return 0;
}

int detect_collision()
{
	int i;
	uint16_t pos = snake_position[0];
	uint8_t pos_x = get_x(pos);
	uint8_t pos_y = get_y(pos);
	if (pos_x < 1 || pos_x > 8 || pos_y < 1 || pos_y > 8) {
		return 1;
	}

	for (i = 1; i < snake_size; i++) {
		if (snake_position[i] == pos) {
			return 1;
		}
	}

	return 0;
}

int detect_fruit()
{
	if (snake_position[0] == fruit_position) {
		return 1;
	}

	return 0;
}

int snake_bigger()
{
	uint16_t prev_pos = snake_position[snake_size - 1];
	snake_size++;
	snake_position[snake_size - 1] = prev_pos;

	return 0;
}

int snake_smaller()
{
	snake_position[snake_size - 1] = 0;
	snake_size--;

	return 0;
}

uint8_t get_x(const uint16_t both) {
	return (both >> 8);
}

uint8_t get_y(const uint16_t both) {
	return (uint8_t)(both & 0xFF);
}

uint16_t combine_xy(const uint8_t x, const uint8_t y)
{
	return ((((uint16_t) x) << 8) | y);
}
