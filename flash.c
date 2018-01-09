#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define VID 0x1b1c
#define PID 0x1b13

#define URB_TIMEOUT 100

#define PKT_LEN 64

static unsigned char colors[12][PKT_LEN] = {
  // Red
  { 0x7f,   0x01,   0x3c,   0x00 },
  { 0x7f,   0x02,   0x3c,   0x00 },
  { 0x7f,   0x03,   0x18,   0x00 },
  { 0x07,   0x28,   0x01,   0x03,   0x01,   0x00 },

  // Green
  { 0x7f,   0x01,   0x3c,   0x00 },
  { 0x7f,   0x02,   0x3c,   0x00 },
  { 0x7f,   0x03,   0x18,   0x00 },
  { 0x07,   0x28,   0x02,   0x03,   0x01,   0x00 },

  // Blue
  { 0x7f,   0x01,   0x3c,   0x00 },
  { 0x7f,   0x02,   0x3c,   0x00 },
  { 0x7f,   0x03,   0x18,   0x00 },
  { 0x07,   0x28,   0x03,   0x03,   0x02,   0x00 },
};

int input_endpoint = 0x81;
int output_endpoint = 0x03;

int main(int argc, char **argv) {
    struct libusb_device_handle *dev;
    int bConfig;
    int bytes_written;
    int status = 0;

    if (geteuid() != 0) {
        printf("Interfacing with hardware devices requires root.\n");
        return 1;
    }

    status = libusb_init(NULL);
    if (status != 0) {
        printf("Could not init libusb: %s\n", libusb_error_name(status));
        return 1;
    }

    // Get the keyboard and open it within the default context
    dev = libusb_open_device_with_vid_pid(NULL, VID, PID);
    if (dev == NULL) {
        printf("Could not find Corsair K70 RGB\n");
        return 1;
    }

    printf("Keyboard detected!\n");
    status = libusb_set_auto_detach_kernel_driver(dev, 1);
    printf("Auto Detach: %s\n", libusb_error_name(status));

    status = libusb_get_configuration(dev, &bConfig);
    printf("bConfig: %d", bConfig);
    if (bConfig != 1) {
        printf("Setting bConfig to 1");
        status = libusb_set_configuration(dev, 1);
        printf("bConfig Set %s\n", bConfig);
    }

    // Claim interfaces to perform IO
    status = libusb_claim_interface(dev, 0);
    printf("If0Claim: %s\n", libusb_error_name(status));
    status = libusb_claim_interface(dev, 1);
    printf("If1Claim: %s\n", libusb_error_name(status));
    status = libusb_claim_interface(dev, 2);
    printf("If2Claim: %s\n", libusb_error_name(status));

    for (int i = 0; i < 9; i+=4) {
        // Set up max brightness
        memset(colors[i] + 4, 0xff, 0x3c*sizeof(char));
        memset(colors[i+1] + 4, 0xff, 0x3c*sizeof(char));
        memset(colors[i+2] + 4, 0xff, 0x18*sizeof(char));
        // Write a color
        printf("Color: %08x\n", colors[i+3][2]);
        for (int j = 0; j < 12; j++) {
            status = libusb_interrupt_transfer(dev, output_endpoint, colors[j], PKT_LEN, &bytes_written, URB_TIMEOUT);
            printf("transfer: %s\n", libusb_error_name(status));
        }
        sleep(1);
        // Reset colors
        memset(colors[i] + 4, 0x00, 0x3c*sizeof(char));
        memset(colors[i+1] + 4, 0x00, 0x3c*sizeof(char));
        memset(colors[i+2] + 4, 0x00, 0x18*sizeof(char));
    }

    // Reset keyboard to blanks
    for (int j = 0; j < 12; j++) {
        status = libusb_interrupt_transfer(dev, output_endpoint, colors[j], PKT_LEN, &bytes_written, URB_TIMEOUT);
        printf("transfer: %s\n", libusb_error_name(status));
    }

    // Clean up
    status = libusb_release_interface(dev, 0);
    printf("If0Rel: %s\n", libusb_error_name(status));
    status = libusb_release_interface(dev, 1);
    printf("If1Rel: %s\n", libusb_error_name(status));
    status = libusb_release_interface(dev, 2);
    printf("If2Rel: %s\n", libusb_error_name(status));

    libusb_exit(NULL);

    return 0;
}
