#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>

int relayInit(void) {
	int fd = open("/dev/i2c-1", O_RDWR);
	if (fd == -1) {
		perror("Opening i2c device for relay");
		return -1;
	} 
	if (ioctl(fd, I2C_SLAVE, 0x10) < 0) {
		perror("ioctl(I2C_SLAVE)");
		return -1;
	}
	return fd;

}

int relayFini(int fd) {
	return close(fd);
}


int relaySet(int fd, bool enabled) {
	uint8_t buf[2];
	buf[0] = 1; // register, i.e. relay number 1-4
	buf[1] = enabled ? 0xff : 0;
	if (write(fd, buf, 2) != 2) {
		perror("I2C write failed");
		return -1;
	}
	return 0;
}

#ifdef RELAY_TEST
int main(void) {
	int fd;

	if ((fd = relayInit()) < 0) {
		return 1;
	}
	if (relaySet(fd, true)) {
		return 1;
	}
	sleep(3);
	if (relaySet(fd, false)) {
		return 1;
	}
	return relayFini(fd);

}
#endif
