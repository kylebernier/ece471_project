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
int init_i2c_display();
int init_i2c_scoreboard();
int init_i2c_nunchuck();
int write_score(const int);
int draw_pixels(const uint16_t, const uint16_t, const int);
int write_display();
int get_nunchuck(char**);
int init_game();
int start_game();
int move_snake();
int snake_bigger();
int snake_smaller();
uint8_t get_x(const uint16_t);
uint8_t get_y(const uint16_t);
uint16_t combine_xy(const uint8_t, const uint8_t);
int detect_collision();
int detect_fruit();

int running = 0;

char game_display[8][8] = {{0}};

uint16_t displaybuffer[8];

// Snake parameters
uint16_t snake_position[64] = {0};
int snake_direction;
int snake_size;

uint16_t fruit_position;

int fd;

/* Main function */
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
	if (init_i2c_display()) return -1;

	// Initialize the scoreboard
	if (init_i2c_scoreboard()) return -1;

	// Initialize the nunchuck
	if (init_i2c_nunchuck()) return -1;

	return 0;
}

/* Initialize the I2C display */
int init_i2c_display()
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
int init_i2c_scoreboard()
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
int init_i2c_nunchuck()
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

/* Write to the score */
int write_score(const int score) {
	int result;
	int i;
	int one, ten, hun, tho;

	unsigned char buffer[17];

	// Upside Down   0     1     2     3     4     5     6     7     8     9
	char nums[] = {0x3f, 0x30, 0x5b, 0x79, 0x74, 0x6d, 0x6f, 0x38, 0x7f, 0x7c};
	
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

	one = 0;
	ten = 0;
	hun = 0;
	tho = 0;

	// Obtain the individual digits of the score
	if (score > 999) {
		tho = score / 1000;
		hun = (score % 1000) / 100;
		ten = ((score % 1000) % 100) / 10;
		one = ((score % 1000) % 100) % 10;
	} else if (score > 99) {
		hun = score / 100;
		ten = (score % 100) / 10;
		one = (score % 100) % 10;
	} else if (score > 9) {
		ten = score / 10;
		one = score % 10;
	} else if (score > -1) {
		one = score;
	}

	// Fill the buffer with the score
	buffer[1] = nums[one];
	buffer[3] = nums[ten];
	buffer[7] = nums[hun];
	buffer[9] = nums[tho];

	// Display the score
	result = write(fd, buffer, 17);
	if (result < 0) {
		fprintf(stderr, "Error writing the score!\n");
		close(fd);
		return -1;
	}

	return 0;
}

/* Write a pixel to the display buffer */
int write_pixel(const uint16_t x, const uint16_t y, const int color) {
	// Check if pixel is within bounds
	if ((y < 0) || (y >= 8)) return -1;
	if ((x < 0) || (y >= 8)) return -1;

	// Check the color of pixel to draw
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

/* Write pixel data to the display */
int write_display() {
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

	// Fill the buffer with pixel data
	for (i = 1; i < 16; i+=2) {
		buffer[i] = displaybuffer[(i-1)/2] & 0xFF;
		buffer[i+1] = displaybuffer[(i-1)/2] >> 8;
	}

	// Write pixel data to the display
	result = write(fd, buffer, 17);
	if (result < 0) {
		fprintf(stderr, "Error writing to display!\n");
		close(fd);
		return -1;
	}

	return 0;	
}

/* Get data from nunchuck */
int get_nunchuck(char** data) {
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
	result = read(fd, *data, 6);
	if (result < 0) {
		fprintf(stderr, "Error reading nunchuck data!\n");
		close(fd);
		return -1;
	}

	return 0;
}

/* Get nunchuck accelerometer data */
float get_nunchuck_accel(const char* data, int axis) {
	
	
	return 0;
}

/* Initialize the game */
int init_game()
{
	// Initailize snake size, direction, and position
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
	int result;
	int i, j;
	int fruit_found;
	int fruit;
	int skip_last;
	int score;
	int random;

	char rand_x[1];
	char rand_y[1];

	char* data[6];

	uint8_t fruit_x, fruit_y;

	// Open the urandom filesystem
	random = open("/dev/urandom", O_RDONLY);
	if (random < 0) {
		fprintf(stderr, "Error opening urandom filesystem!\n");
		close(random);
		return -1;
	}

	// Set running to true
	running = 1;
		
	printf("The game is running!\n");

	// Initialize score and fruit
	score = 0;
	fruit = 0;
	fruit_x = 0;
	fruit_y = 0;

	// Don't skip last segment
	skip_last = 0;

	// Run the game loop while running is true
	while (running) {
		// Set fruit to not found
		fruit_found = 0;

		// Display each snake segment
		for (i = 0; i < snake_size; i++) {
			uint16_t pos = snake_position[i];
			printf("Segment %d: [%d, %d]\n", i, get_x(pos), get_y(pos));
			write_pixel(get_x(pos) - 1, get_y(pos) - 1, 3);
		}
		write_display();

		// Game delay
		for (i = 0; i < 10; i++) {
			usleep(50000);
			get_nunchuck(data);
		}
	
		// Move the snake
		move_snake(skip_last);
		printf("Snake Moved!\n");

		// Check collisions
		if(detect_collision() == 1) {
			printf("CRASH --- Game Over!\n");
			break;
		}

		// Check for eaten fruit
		if(detect_fruit()) {
			// No fruit on display
			fruit = 0;
			// Snake grows
			snake_bigger();
			// Don't move last segment
			skip_last = 1;
			// Increase score
			score++;
			// Update the score
			write_score(score);
			printf("Fruit Eaten!\n");
		} else {
			// Don't skip last segment
			skip_last = 0;
		}

		// Update the score
		write_score(score);
		
		// Attempt 7 times to make a fruit
		// Most lines in loop execute every game loop to keep consistant timing
		for (i = 0; i < 7; i++) {
			// Obtain random number
			result = read(random, rand_x, 1);
			if (result < 0) {
				fprintf(stderr, "Error reading random number!\n");
				close(random);
				return -1;
			}
			// Obtain random number
			result = read(random, rand_y, 1);
			if (result < 0) {
				fprintf(stderr, "Error reading random number!\n");
				close(random);
				return -1;
			}

			// If not current fruit convert random values to positions
			if (!fruit_found && !fruit) {
				fruit_x = rand_x[0] % 8 + 1;
				fruit_y = rand_y[0] % 8 + 1;
			}

			// Check if new fruit is on top of snake
			for (j = 0; j < snake_size; j++) {
				if (combine_xy(fruit_x, fruit_y) == snake_position[j]) {
					// Delete the fruit if collision
					if (!fruit_found && !fruit) {
						fruit_x = 0;
						fruit_y = 0;
					}
				}
			}

			// Check if fruit was made/found
			if (fruit_x != 0 && fruit_y != 0) {
				// Do the fruit
				fruit_found = 1;
				fruit = 1;
				fruit_position = combine_xy(fruit_x, fruit_y);
			}
		}

		printf("Fruit: [%d, %d]\n", fruit_x, fruit_y);
	}

	return 0;
}

/* Move the snake */
int move_snake(int skip)
{
	int i;

	uint16_t next_pos;
	uint16_t prev_pos = snake_position[0];

	// Move the head based on the current direction
	switch (snake_direction) {
		// North
		case 0:
			snake_position[0] = combine_xy(get_x(prev_pos), get_y(prev_pos) - 1);
			break;
		// East
		case 1:
			snake_position[0] = combine_xy(get_x(prev_pos) + 1, get_y(prev_pos));
			break;
		// South
		case 2:
			snake_position[0] = combine_xy(get_x(prev_pos), get_y(prev_pos) + 1);
			break;
		// West
		case 3:
			snake_position[0] = combine_xy(get_x(prev_pos) - 1, get_y(prev_pos));
			break;
		// Otherwise error
		default:
			fprintf(stderr, "Invalid snake direction!\n");
			return -1;
	}

	// Move each segment of the snake
	for (i = 1; i < snake_size - skip; i++) {
		next_pos = snake_position[i];	
		snake_position[i] = prev_pos;
		prev_pos = next_pos;
	}

	return 0;
}

/* Detect snake collision with wall or self (1: collision 0: none) */
int detect_collision()
{
	int i;

	uint16_t pos = snake_position[0];

	uint8_t pos_x = get_x(pos);
	uint8_t pos_y = get_y(pos);

	// Check collision with wall
	if (pos_x < 1 || pos_x > 8 || pos_y < 1 || pos_y > 8) {
		return 1;
	}

	// Check collision with self
	for (i = 1; i < snake_size; i++) {
		if (snake_position[i] == pos) {
			return 1;
		}
	}

	return 0;
}

/* Detect if snake eats fruit (1: eaten 0: not eaten) */
int detect_fruit()
{
	// Compare the snake head position to the fruit position
	if (snake_position[0] == fruit_position) {
		return 1;
	}

	return 0;
}

/* Make the snake bigger */
int snake_bigger()
{
	uint16_t prev_pos = snake_position[snake_size - 1];
	snake_size++;
	snake_position[snake_size - 1] = prev_pos;

	return 0;
}
 /* Make the snake smaller */
int snake_smaller()
{
	snake_position[snake_size - 1] = 0;
	snake_size--;

	return 0;
}

/* Obtain the x position from a 2d position */
uint8_t get_x(const uint16_t both) {
	return (both >> 8);
}

/* Obtain the y position from a 2d position */
uint8_t get_y(const uint16_t both) {
	return (uint8_t)(both & 0xFF);
}

/* Create a 2d position from an x and y position */
uint16_t combine_xy(const uint8_t x, const uint8_t y)
{
	return ((((uint16_t) x) << 8) | y);
}
