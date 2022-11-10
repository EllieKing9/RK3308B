#include <iostream>

#include <stdio.h>

#include <time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <linux/i2c-dev.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <pthread.h>
#include <sched.h>

// PATH DEFINE
#define GPIO_BASE_PATH      "/sys/class/gpio/gpio"
#define GPIO_EXPORT_PATH    "/sys/class/gpio/export"
#define GPIO_UNEXPORT_PATH  "/sys/class/gpio/unexport"

#define GPIO_VAL_PATH       "/value"
#define GPIO_INT_PATH       "/edge"
#define GPIO_DIR_PATH       "/direction"
#define GPIO_ACT_PATH       "/active_low"

#define I2C2_DEVICE_PATH    "/dev/i2c-2"  // AM2320 temperature, humidity

#define PWM_BASE_PATH       "/sys/class/pwm/pwmchip"
#define PWM0_PATH           "/pwm0"

#define PWM_EXPORT_PATH     "/export"
#define PWM_UNEXPORT_PATH   "/unexport"

#define PWM_PER_PATH        "/period"
#define PWM_DUT_PATH        "/duty_cycle"
#define PWM_ENA_PATH        "/enable"
#define PWM_POL_PATH        "/polarity"

// GPIO PIN NUMBER DEFINE
#define GPIO4_A6 134  // LED, green(user)
#define GPIO0_B2 10   // LED, blue

#define GPIO2_A7 71   // SWITCH

#define GPIO2_A6 70   // STEPPING MOTOR direction
#define GPIO2_B4 76   // STEPPING MOTOR enable
 
//PWM CHIPSET NUMBER DEFINE
#define PWM7 2        // ff170030
#define PWM6 1        // ff170020

//
#define POLL_EVENT POLLPRI | POLLERR 

//
#define TIMEOUT 60

#define NSAMPLES 100

#define AM2320_ADDR 0x5C


void set_realtime_prio();
int configure_pins_switch();
void close_fd(int);
int am2320(float *out_temperature, float *out_humidity);
int configure_pins_stepping();

char period_val[256]= {0x00, };
char duty_val[256]= {0x00, };
uint8_t input_val= 0;

int main(int argc, char *argv[]) {

  int fd, count, poll_ret, ret, moving= 0;
  double period;
  char value;
  struct pollfd poll_gpio_val;
  struct timespec tstart= {0,0}, tend= {0,0};
  char str_buf[256]= {0x00, };
  float temp, humi;
  int fd_pwm_enable, fd_gpio_enable;

  if(argc == 3) {
    sprintf(period_val, "%s", argv[1]);
    sprintf(duty_val, "%s", argv[2]);
    input_val= argc;
    printf("period: %s  duty_cycle: %s\n\n", period_val, duty_val);
  }
  else if(argc == 2) {
    sprintf(period_val, "%s", argv[1]);
    input_val= argc;
    printf("period: %s  duty_cycle: 2000\n\n", period_val, duty_val);
  }

  set_realtime_prio();
  configure_pins_switch();
  configure_pins_stepping();
  
  sprintf(str_buf,"%s%d%s", GPIO_BASE_PATH, GPIO2_A7, GPIO_VAL_PATH);
  while((fd= open(str_buf, O_RDONLY)) <= 0);
  
  // file descriptor from SW is being polled
  poll_gpio_val.fd= fd; 
  poll_gpio_val.events= POLL_EVENT;
  poll_gpio_val.revents= 0;

  ret= read(fd, &value, 1); //1: buffer size
  printf("v: %c\n", value);

  while(1){
    
	  clock_gettime(CLOCK_MONOTONIC, &tstart);
    for(count= 0; count < NSAMPLES; count++) {
      lseek(fd, 0, SEEK_SET);
      ret= read(fd, &value, 1);
      //printf("v: %c\n", value);

      poll_ret= poll(&poll_gpio_val, 1, TIMEOUT*1000);
      
      if(!poll_ret) {
        printf("Timeout\n");
        return 0;
      }
      else {
        if(poll_ret == -1) {
          perror("poll");
          return EXIT_FAILURE;
        }
    
        if((poll_gpio_val.revents) & (POLL_EVENT)){
          lseek(fd, 0, SEEK_SET);
          ret= read(fd, &value, 1); // read GPIO value
          printf("GPIO changed state! v: %c \n", value);

          ret= am2320(&temp, &humi);
          if(ret) {
            printf("Err num: %d\n", ret);
          }
          else {
            printf( "Temperature %.1f [C]\n", temp);
            printf( "Humidity    %.1f [%%]\n", humi);
          }

          if(moving == 0) {
            memset(str_buf, 0x00, sizeof(str_buf));
            sprintf(str_buf,"%s%d%s%s", PWM_BASE_PATH, PWM7, PWM0_PATH, PWM_ENA_PATH);
            while((fd_pwm_enable = open(str_buf, O_RDWR)) <=0);
            while(write(fd_pwm_enable, "1", 1) < 0);
            close_fd(fd_pwm_enable);

            memset(str_buf, 0x00, sizeof(str_buf));
            sprintf(str_buf,"%s%d%s", GPIO_BASE_PATH, GPIO2_B4, GPIO_VAL_PATH);
            while((fd_gpio_enable = open(str_buf, O_RDWR)) <=0);
            while(write(fd_gpio_enable, "1", 1) < 0);
            close_fd(fd_gpio_enable);
            moving= 1; //true            
          }
          else {
            memset(str_buf, 0x00, sizeof(str_buf));
            sprintf(str_buf,"%s%d%s%s", PWM_BASE_PATH, PWM7, PWM0_PATH, PWM_ENA_PATH);
            while((fd_pwm_enable = open(str_buf, O_RDWR)) <=0);
            while(write(fd_pwm_enable, "0", 1) < 0);
            close_fd(fd_pwm_enable);

            memset(str_buf, 0x00, sizeof(str_buf));
            sprintf(str_buf,"%s%d%s", GPIO_BASE_PATH, GPIO2_B4, GPIO_VAL_PATH);
            while((fd_gpio_enable = open(str_buf, O_RDWR)) <=0);
            while(write(fd_gpio_enable, "0", 1) < 0);
            close_fd(fd_gpio_enable);
            moving= 0;
          }

        }
      }
    } 
    clock_gettime(CLOCK_MONOTONIC, &tend); 
    period = (((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec))*1000; // period in ms
    //period = 1.0e-9*((tend.tv_nsec) - (tstart.tv_nsec));  // period in 

    period /= NSAMPLES; // divide by n samples
    printf("Period (ms): %.3f \n", period);
    printf("Frequency (kHz): %.3f \n", 1/period);

	}

  close(fd); //close value file
  return EXIT_SUCCESS;
}

void set_realtime_prio(){
  pthread_t this_thread = pthread_self(); // operates in the current running thread
  struct sched_param params;
  int ret;
  
  // set max prio 
  params.sched_priority = sched_get_priority_max(SCHED_FIFO);    
  ret = pthread_setschedparam(this_thread, SCHED_FIFO, &params);

  if(ret != 0) {
    perror("Unsuccessful in setting thread realtime prio");
  }
}

// close file descriptor
void close_fd(int fd){
	if(close(fd) < 0){
		perror("Warning: Unable to close file correctly");
	}
}

int configure_pins_switch(){
  int fd, fd_export, fd_unexport, fd_edge, fd_input, ret;
  char num_buf[256] = {0x00, };
  char str_buf[256] = {0x00, };
  char tmp_str[256] = {0x00, };

  printf("gpio%d\n", GPIO2_A7);


  /*******************UNEXPORT*******************/
  memset(num_buf, 0x00, sizeof(num_buf));
  sprintf(num_buf, "%d", GPIO2_A7);
  memset(str_buf, 0x00, sizeof(str_buf));
  sprintf(str_buf,"%s%d%s", GPIO_BASE_PATH, GPIO2_A7, GPIO_VAL_PATH);
  
  // open unexport file
  if((fd_unexport= open(GPIO_UNEXPORT_PATH, O_WRONLY)) <= 0) {
    //O_RDONLY //O_WRONLY //O_RDWR
    perror("Unable to open unexport");
    return EXIT_FAILURE;
  }
  else {
    if((fd= open(str_buf, O_RDONLY)) > 0) {
      // unexport SW GPIO
      if(write(fd_unexport, num_buf, strlen(num_buf)) < 0){
        if(errno != EBUSY) { // maybe.. does not end if pin is already exported
          perror("Unable to unexport");
          close_fd(fd_unexport);
          return EXIT_FAILURE;
        }
        perror("Warning: Unable to unexport");
      }
      else {
        printf("Unexported gpio%d successfully\n", GPIO2_A7);
        close_fd(fd_unexport);
        close_fd(fd);
        sleep(2); //usleep(2000000);
      }
    }
  }
  

  /*******************EXPORT*******************/
  // open export file
  if((fd_export = open(GPIO_EXPORT_PATH, O_WRONLY)) <= 0) {
    perror("Unable to open export");
    return EXIT_FAILURE;
  }
  else {
    // export SW GPIO
    if(write(fd_export, num_buf, strlen(num_buf)) < 0) {
      if(errno != EBUSY) { // maybe.. does not end if pin is already exported
        perror("Unable to export");
        close_fd(fd_export);
        return EXIT_FAILURE;
      }
      perror("Warning: Unable to export");
    } 
    else {
      printf("Exported gpio%d successfully\n", GPIO2_A7);
      close_fd(fd_export);
      usleep(200000);
    }
  }

  
  /******************DIRECTION******************/
  memset(str_buf, 0x00, sizeof(str_buf));
  sprintf(str_buf, "%s%d%s", GPIO_BASE_PATH, GPIO2_A7, GPIO_DIR_PATH);

  // open direction file
  if((fd_input = open(str_buf, O_RDWR)) <=0) {
    perror("Unable to open direction");
    return EXIT_FAILURE;
  } 
  else {
    if(write(fd_input, "in", 2) < 0) {
      if(errno != EBUSY){
        perror("Unable to change direction");
        close_fd(fd_input);
        return EXIT_FAILURE;
      }
      perror("Warning: unable to change direction");
    }
    else {
      memset(tmp_str, 0x00, sizeof(tmp_str));
      usleep(100000);
      lseek(fd_input, 0, SEEK_SET);
      ret= read(fd_input, tmp_str, 10);
      printf("Changed direction: %s", tmp_str);
      close_fd(fd_input);
    }  
  }


  /******************ACTIVE_LOW******************/
  memset(str_buf, 0x00, sizeof(str_buf));
  sprintf(str_buf, "%s%d%s", GPIO_BASE_PATH, GPIO2_A7, GPIO_ACT_PATH);
  // open active_low file
  if((fd_input = open(str_buf, O_RDWR)) <=0) {
      perror("Unable to open active_low");
      return EXIT_FAILURE;
  } 
  else { 
    if(write(fd_input, "1", 1) < 0) {
      if(errno != EBUSY) {
        perror("Unable to change active_low");
        close_fd(fd_input);
        return EXIT_FAILURE;
      }
      perror("Warning: unable to change active_low");
    }
    else {
      memset(tmp_str, 0x00, sizeof(tmp_str));
      usleep(100000);
      lseek(fd_input, 0, SEEK_SET);
      ret= read(fd_input, tmp_str, 10);
      printf("Changed active_low: %s", tmp_str);
      close_fd(fd_input);
    }
  }

  
  /********************EDGE*********************/
  memset(str_buf, 0x00, sizeof(str_buf));
  sprintf(str_buf, "%s%d%s", GPIO_BASE_PATH, GPIO2_A7, GPIO_INT_PATH);

  while((fd_edge = open(str_buf, O_RDWR)) <=0);
  while(write(fd_edge, "rising", 6) < 0);
  // while(write(fd_edge, "both", 4) < 0);

  memset(tmp_str, 0x00, sizeof(tmp_str));
  usleep(100000);
  lseek(fd_edge, 0, SEEK_SET);
  ret= read(fd_edge, tmp_str, 10);
  printf("Changed edge: %s", tmp_str);
  close_fd(fd_edge);

  return EXIT_SUCCESS;
}


// int main(int argc, char *argv[])
// {
//     std::cout << "Hello World!!" << std::endl;

//     struct timespec tstart = {0,0}, tend = {0,0};
// 	int count=0, fd;
// 	double period;
// 	unsigned char value0, value1;  // catch rising edge
	
// 	configure_pins();

// 	while((fd = open(SW_VAL_PATH, O_RDONLY)) <= 0);
// 	printf("Opened SW value sucessfully\n");

// 	while(1){
// 		while(count <= NSAMPLES){// discard first, add last
// 			if(count == 1){ // discard first sample
// 				clock_gettime(CLOCK_MONOTONIC, &tstart);
// 			}

// 			lseek(fd, 0, SEEK_SET);
// 			if(read(fd, &value0, 1) > 1)
// 				perror("could not read the file");
            
// 			lseek(fd, 0, SEEK_SET);
// 			if(read(fd, &value1, 1) > 1)
// 				perror("could not read the file");

//             printf("value0: %c | value1: %c | count: %d \n", value0, value1, count);

// 			//if((value0 == '0' && value1 == '1') || (count > 1 && value0 == '1' && value1 == '0')){   // rising edge
//             // if((value0 == '1' && value1 == '0') || (count > 1 && value0 == '0' && value1 == '1')){   // falling edge
// 				count++;  // acquired sample
// 			// }
// 		}
// 		clock_gettime(CLOCK_MONOTONIC, &tend);
// 		count = 0;
// 		period = (((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec))*1000; // period in ms
// 		period /= NSAMPLES;
// 	    printf("Period (ms): %.3f \n", period);
// 		printf("Frequency (kHz): %.3f \n", 1/period);
// 	}

// 	close_fd(fd);	

// 	return EXIT_SUCCESS;
// }


/*
cd /sys/bus/i2c/devices/i2c-2 //new_device

cd /sys/class/i2c-dev/i2c-2 //dev 89::2

*/


static uint16_t 
_calc_crc16(const uint8_t *buf, size_t len) {
  uint16_t crc = 0xFFFF;
  
  while(len--) {
    crc ^= (uint16_t) *buf++;
    for (unsigned i = 0; i < 8; i++) {
      if (crc & 0x0001) {
	crc >>= 1;
	crc ^= 0xA001;
      } else {
	crc >>= 1;      
      }
    }
  }
  
  return crc;
}

static uint16_t
_combine_bytes(uint8_t msb, uint8_t lsb)
{
  return ((uint16_t)msb << 8) | (uint16_t)lsb;
}

int am2320(float *out_temperature, float *out_humidity) 
{
  int fd, ret;
  uint8_t data[8];

  while((fd = open(I2C2_DEVICE_PATH, O_RDWR)) <= 0);
  if (fd < 0)
    return 1;

  if (ioctl(fd, I2C_SLAVE, AM2320_ADDR) < 0)
    return 2;
   
  /* wake AM2320 up, goes to sleep to not warm up and
   * affect the humidity sensor 
   */
  ret= write(fd, NULL, 0);
  usleep(1000); /* at least 0.8ms, at most 3ms */
  
  /* write at addr 0x03, start reg = 0x00, num regs = 0x04 */
  data[0] = 0x03; 
  data[1] = 0x00; 
  data[2] = 0x04;
  if (write(fd, data, 3) < 0)
    return 3;
  
  /* wait for AM2320 */
  usleep(1600); /* Wait at least 1.5ms */
  
  /*
   * Read out 8 bytes of data
   * Byte 0: Should be Modbus function code 0x03
   * Byte 1: Should be number of registers to read (0x04)
   * Byte 2: Humidity msb
   * Byte 3: Humidity lsb
   * Byte 4: Temperature msb
   * Byte 5: Temperature lsb
   * Byte 6: CRC lsb byte
   * Byte 7: CRC msb byte
   */
  if (read(fd, data, 8) < 0)
    return 4;
  
  close(fd);

  //printf("[0x%02x 0x%02x  0x%02x 0x%02x  0x%02x 0x%02x  0x%02x 0x%02x]\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7] );

  /* Check data[0] and data[1] */
  if (data[0] != 0x03 || data[1] != 0x04)
    return 9;

  /* Check CRC */
  uint16_t crcdata = _calc_crc16(data, 6);
  uint16_t crcread = _combine_bytes(data[7], data[6]);
  if (crcdata != crcread) 
    return 10;

  uint16_t temp16 = _combine_bytes(data[4], data[5]); 
  uint16_t humi16 = _combine_bytes(data[2], data[3]);   
  //printf("temp=%u 0x%04x  hum=%u 0x%04x\n", temp16, temp16, humi16, humi16);
  
  /* Temperature resolution is 16Bit, 
   * temperature highest bit (Bit15) is equal to 1 indicates a
   * negative temperature, the temperature highest bit (Bit15)
   * is equal to 0 indicates a positive temperature; 
   * temperature in addition to the most significant bit (Bit14 ~ Bit0)
   *  indicates the temperature sensor string value.
   * Temperature sensor value is a string of 10 times the
   * actual temperature value.
   */
  if (temp16 & 0x8000)
    temp16 = -(temp16 & 0x7FFF);

  *out_temperature = (float)temp16 / 10.0;
  *out_humidity = (float)humi16 / 10.0;

  return 0;
}

/*
cat /sys/kernel/debug/pwm
period: 69997 ns duty: 30000 ns polarity: inverse
50000 ~ 150000 

ls -l /sys/class/pwm/   

gpio 70 -> dir : active_low(1) | value(0, left | 1, right)
gpio 76 -> en :  active_low(1)

*/

int configure_pins_stepping(){
  int fd, fd_export, fd_unexport, fd_edge, fd_input, ret;
  char num_buf[256] = {0x00, };
  char str_buf[256] = {0x00, };
  char tmp_str[256] = {0x00, };

  printf("gpio%d\n", GPIO2_A6);


  /*******************UNEXPORT*******************/
  memset(num_buf, 0x00, sizeof(num_buf));
  sprintf(num_buf, "%d", GPIO2_A6);
  memset(str_buf, 0x00, sizeof(str_buf));
  sprintf(str_buf,"%s%d%s", GPIO_BASE_PATH, GPIO2_A6, GPIO_VAL_PATH);
  
  // open unexport file
  if((fd_unexport= open(GPIO_UNEXPORT_PATH, O_WRONLY)) <= 0) {
    perror("Unable to open unexport");
    return EXIT_FAILURE;
  }
  else {
    if((fd= open(str_buf, O_RDONLY)) > 0) {
      // unexport SW GPIO
      if(write(fd_unexport, num_buf, strlen(num_buf)) < 0){
        if(errno != EBUSY) { // maybe.. does not end if pin is already exported
          perror("Unable to unexport");
          close_fd(fd_unexport);
          return EXIT_FAILURE;
        }
        perror("Warning: Unable to unexport");
      }
      else {
        printf("Unexported gpio%d successfully\n", GPIO2_A6);
        close_fd(fd_unexport);
        close_fd(fd);
        sleep(2); //usleep(2000000);
      }
    }
  }
  

  /*******************EXPORT*******************/
  // open export file
  if((fd_export = open(GPIO_EXPORT_PATH, O_WRONLY)) <= 0) {
    perror("Unable to open export");
    return EXIT_FAILURE;
  }
  else {
    // export SW GPIO
    if(write(fd_export, num_buf, strlen(num_buf)) < 0) {
      if(errno != EBUSY) { // maybe.. does not end if pin is already exported
        perror("Unable to export");
        close_fd(fd_export);
        return EXIT_FAILURE;
      }
      perror("Warning: Unable to export");
    } 
    else {
      printf("Exported gpio%d successfully\n", GPIO2_A6);
      close_fd(fd_export);
      usleep(200000);
    }
  }

  
  /******************DIRECTION******************/
  memset(str_buf, 0x00, sizeof(str_buf));
  sprintf(str_buf, "%s%d%s", GPIO_BASE_PATH, GPIO2_A6, GPIO_DIR_PATH);

  // open direction file
  if((fd_input = open(str_buf, O_RDWR)) <=0) {
    perror("Unable to open direction");
    return EXIT_FAILURE;
  } 
  else {
    if(write(fd_input, "out", 3) < 0) {
      if(errno != EBUSY){
        perror("Unable to change direction");
        close_fd(fd_input);
        return EXIT_FAILURE;
      }
      perror("Warning: unable to change direction");
    }
    else {
      memset(tmp_str, 0x00, sizeof(tmp_str));
      usleep(100000);
      lseek(fd_input, 0, SEEK_SET);
      ret= read(fd_input, tmp_str, 10);
      printf("Changed direction: %s", tmp_str);
      close_fd(fd_input);
    }  
  }


  /******************ACTIVE_LOW******************/
  memset(str_buf, 0x00, sizeof(str_buf));
  sprintf(str_buf, "%s%d%s", GPIO_BASE_PATH, GPIO2_A6, GPIO_ACT_PATH);
  // open active_low file
  if((fd_input = open(str_buf, O_RDWR)) <=0) {
      perror("Unable to open active_low");
      return EXIT_FAILURE;
  } 
  else { 
    if(write(fd_input, "1", 1) < 0) {
      if(errno != EBUSY) {
        perror("Unable to change active_low");
        close_fd(fd_input);
        return EXIT_FAILURE;
      }
      perror("Warning: unable to change active_low");
    }
    else {
      memset(tmp_str, 0x00, sizeof(tmp_str));
      usleep(100000);
      lseek(fd_input, 0, SEEK_SET);
      ret= read(fd_input, tmp_str, 10); 
      printf("Changed active_low: %s", tmp_str);
      close_fd(fd_input);
    }
  }


  /******************VALUE******************/
  memset(str_buf, 0x00, sizeof(str_buf));
  sprintf(str_buf, "%s%d%s", GPIO_BASE_PATH, GPIO2_A6, GPIO_VAL_PATH);
  // open active_low file
  if((fd_input = open(str_buf, O_RDWR)) <=0) {
      perror("Unable to open value");
      return EXIT_FAILURE;
  } 
  else { 
    if(write(fd_input, "0", 1) < 0) {
      if(errno != EBUSY) {
        perror("Unable to change value");
        close_fd(fd_input);
        return EXIT_FAILURE;
      }
      perror("Warning: unable to change value");
    }
    else {
      memset(tmp_str, 0x00, sizeof(tmp_str));
      usleep(100000);
      lseek(fd_input, 0, SEEK_SET);
      ret= read(fd_input, tmp_str, 10);
      printf("Changed value: %s", tmp_str);
      close_fd(fd_input);
    }
  }



  printf("gpio%d\n", GPIO2_B4);


  /*******************UNEXPORT*******************/
  memset(num_buf, 0x00, sizeof(num_buf));
  sprintf(num_buf, "%d", GPIO2_B4);
  memset(str_buf, 0x00, sizeof(str_buf));
  sprintf(str_buf,"%s%d%s", GPIO_BASE_PATH, GPIO2_B4, GPIO_VAL_PATH);
  
  // open unexport file
  if((fd_unexport= open(GPIO_UNEXPORT_PATH, O_WRONLY)) <= 0) {
    perror("Unable to open unexport");
    return EXIT_FAILURE;
  }
  else {
    if((fd= open(str_buf, O_RDONLY)) > 0) {
      // unexport SW GPIO
      if(write(fd_unexport, num_buf, strlen(num_buf)) < 0){
        if(errno != EBUSY) { // maybe.. does not end if pin is already exported
          perror("Unable to unexport");
          close_fd(fd_unexport);
          return EXIT_FAILURE;
        }
        perror("Warning: Unable to unexport");
      }
      else {
        printf("Unexported gpio%d successfully\n", GPIO2_B4);
        close_fd(fd_unexport);
        close_fd(fd);
        sleep(2); //usleep(2000000);
      }
    }
  }
  

  /*******************EXPORT*******************/
  // open export file
  if((fd_export = open(GPIO_EXPORT_PATH, O_WRONLY)) <= 0) {
    perror("Unable to open export");
    return EXIT_FAILURE;
  }
  else {
    // export SW GPIO
    if(write(fd_export, num_buf, strlen(num_buf)) < 0) {
      if(errno != EBUSY) { // maybe.. does not end if pin is already exported
        perror("Unable to export");
        close_fd(fd_export);
        return EXIT_FAILURE;
      }
      perror("Warning: Unable to export");
    } 
    else {
      printf("Exported gpio%d successfully\n", GPIO2_B4);
      close_fd(fd_export);
      usleep(200000);
    }
  }

  
  /******************DIRECTION******************/
  memset(str_buf, 0x00, sizeof(str_buf));
  sprintf(str_buf, "%s%d%s", GPIO_BASE_PATH, GPIO2_B4, GPIO_DIR_PATH);

  // open direction file
  if((fd_input = open(str_buf, O_RDWR)) <=0) {
    perror("Unable to open direction");
    return EXIT_FAILURE;
  } 
  else {
    if(write(fd_input, "out", 3) < 0) {
      if(errno != EBUSY){
        perror("Unable to change direction");
        close_fd(fd_input);
        return EXIT_FAILURE;
      }
      perror("Warning: unable to change direction");
    }
    else {
      memset(tmp_str, 0x00, sizeof(tmp_str));
      usleep(100000);
      lseek(fd_input, 0, SEEK_SET);
      ret= read(fd_input, tmp_str, 10);
      printf("Changed direction: %s", tmp_str);
      close_fd(fd_input);
    }  
  }


  /******************ACTIVE_LOW******************/
  memset(str_buf, 0x00, sizeof(str_buf));
  sprintf(str_buf, "%s%d%s", GPIO_BASE_PATH, GPIO2_B4, GPIO_ACT_PATH);
  // open active_low file
  if((fd_input = open(str_buf, O_RDWR)) <=0) {
      perror("Unable to open active_low");
      return EXIT_FAILURE;
  } 
  else { 
    if(write(fd_input, "1", 1) < 0) {
      if(errno != EBUSY) {
        perror("Unable to change active_low");
        close_fd(fd_input);
        return EXIT_FAILURE;
      }
      perror("Warning: unable to change active_low");
    }
    else {
      memset(tmp_str, 0x00, sizeof(tmp_str));
      usleep(100000);
      lseek(fd_input, 0, SEEK_SET);
      ret= read(fd_input, tmp_str, 10);
      printf("Changed active_low: %s", tmp_str);
      close_fd(fd_input);
    }
  }


  /******************VALUE******************/
  memset(str_buf, 0x00, sizeof(str_buf));
  sprintf(str_buf, "%s%d%s", GPIO_BASE_PATH, GPIO2_B4, GPIO_VAL_PATH);
  // open active_low file
  if((fd_input = open(str_buf, O_RDWR)) <=0) {
      perror("Unable to open value");
      return EXIT_FAILURE;
  } 
  else { 
    if(write(fd_input, "0", 1) < 0) {
      if(errno != EBUSY) {
        perror("Unable to change value");
        close_fd(fd_input);
        return EXIT_FAILURE;
      }
      perror("Warning: unable to change value");
    }
    else {
      memset(tmp_str, 0x00, sizeof(tmp_str));
      usleep(100000);
      lseek(fd_input, 0, SEEK_SET);
      ret= read(fd_input, tmp_str, 10); 
      printf("Changed value: %s", tmp_str);
      close_fd(fd_input);
    }
  }



  printf("pwm7\n");


  /*******************UNEXPORT*******************/
  memset(num_buf, 0x00, sizeof(num_buf));
  sprintf(num_buf, "%d", PWM7);
  memset(str_buf, 0x00, sizeof(str_buf));
  sprintf(str_buf,"%s%d%s%s", PWM_BASE_PATH, PWM7, PWM0_PATH, PWM_ENA_PATH);
  memset(tmp_str, 0x00, sizeof(tmp_str));
  sprintf(tmp_str, "%s%d%s", PWM_BASE_PATH, PWM7, PWM_UNEXPORT_PATH);

  // open unexport file
  if((fd_unexport= open(tmp_str, O_WRONLY)) <= 0) {
    perror("Unable to open unexport");
    return EXIT_FAILURE;
  }
  else {
    // unexport
    if((fd= open(str_buf, O_RDONLY)) > 0) {

      if(write(fd_unexport, "0", strlen(num_buf)) < 0){
        if(errno != EBUSY) { // maybe.. does not end if pin is already exported
          perror("Unable to unexport");
          close_fd(fd_unexport);
          return EXIT_FAILURE;
        }
        perror("Warning: Unable to unexport");
      }
      else {
        printf("Unexported pwm7 successfully\n");
        close_fd(fd_unexport);
        close_fd(fd);
        sleep(2); //usleep(2000000);
      }
    }
  }
  

  /*******************EXPORT*******************/
  memset(str_buf, 0x00, sizeof(str_buf));
  sprintf(str_buf,"%s%d%s", PWM_BASE_PATH, PWM7, PWM_EXPORT_PATH);

  // open export file
  if((fd_export = open(str_buf, O_WRONLY)) <= 0) {
    perror("Unable to open export");
    return EXIT_FAILURE;
  }
  else {
    // export
    if(write(fd_export, "0", strlen(num_buf)) < 0) {
      if(errno != EBUSY) { // maybe.. does not end if pin is already exported
        perror("Unable to export");
        close_fd(fd_export);
        return EXIT_FAILURE;
      }
      perror("Warning: Unable to export");
    } 
    else {
      printf("Exported pwm7 successfully\n");
      close_fd(fd_export);
      usleep(200000);
    }
  }

  
  /******************PERIOD******************/
  memset(str_buf, 0x00, sizeof(str_buf));
  sprintf(str_buf,"%s%d%s%s", PWM_BASE_PATH, PWM7, PWM0_PATH, PWM_PER_PATH);
  if(input_val >= 2) {
    memset(num_buf, 0x00, sizeof(num_buf));
    sprintf(num_buf,"%s", period_val);
  }
  else {
    memset(num_buf, 0x00, sizeof(num_buf));
    sprintf(num_buf,"120000");
  }

  if((fd_input = open(str_buf, O_RDWR)) <=0) {
    perror("Unable to open period");
    return EXIT_FAILURE;
  } 
  else {
    if(write(fd_input, num_buf, strlen(num_buf)) < 0) {
      if(errno != EBUSY){
        perror("Unable to change period");
        close_fd(fd_input);
        return EXIT_FAILURE;
      }
      perror("Warning: unable to change period");
    }
    else {
      memset(tmp_str, 0x00, sizeof(tmp_str));
      usleep(100000);
      lseek(fd_input, 0, SEEK_SET);
      ret= read(fd_input, tmp_str, 10);
      printf("Changed period: %s", tmp_str);
      close_fd(fd_input);
    }  
  }


  /******************DUTY_CYCLE******************/
  memset(str_buf, 0x00, sizeof(str_buf));
  sprintf(str_buf,"%s%d%s%s", PWM_BASE_PATH, PWM7, PWM0_PATH, PWM_DUT_PATH);
  if(input_val == 3) {
    memset(num_buf, 0x00, sizeof(num_buf));
    sprintf(num_buf,"%s", duty_val);
  }
  else {
    memset(num_buf, 0x00, sizeof(num_buf));
    sprintf(num_buf,"2000");
  }

  if((fd_input = open(str_buf, O_RDWR)) <=0) {
      perror("Unable to open duty_cycle");
      return EXIT_FAILURE;
  } 
  else { 
    if(write(fd_input, num_buf, strlen(num_buf)) < 0) {
      if(errno != EBUSY) {
        perror("Unable to change duty_cycle");
        close_fd(fd_input);
        return EXIT_FAILURE;
      }
      perror("Warning: unable to change duty_cycle");
    }
    else {
      memset(tmp_str, 0x00, sizeof(tmp_str));
      usleep(100000);
      lseek(fd_input, 0, SEEK_SET);
      ret= read(fd_input, tmp_str, 10);
      printf("Changed duty_cycle: %s", tmp_str);
      close_fd(fd_input);
    }
  }


  /******************POLARITY******************/
  memset(str_buf, 0x00, sizeof(str_buf));
  sprintf(str_buf,"%s%d%s%s", PWM_BASE_PATH, PWM7, PWM0_PATH, PWM_POL_PATH);
  // open active_low file
  if((fd_input = open(str_buf, O_RDWR)) <=0) {
      perror("Unable to open polarity");
      return EXIT_FAILURE;
  } 
  else { 
    if(write(fd_input, "normal", 6) < 0) {
      if(errno != EBUSY) {
        perror("Unable to change polarity");
        close_fd(fd_input);
        return EXIT_FAILURE;
      }
      perror("Warning: unable to change polarity");
    }
    else {
      memset(tmp_str, 0x00, sizeof(tmp_str));
      usleep(100000);
      lseek(fd_input, 0, SEEK_SET);
      ret= read(fd_input, tmp_str, 10); 
      printf("Changed polarity: %s", tmp_str);
      close_fd(fd_input);
    }
  }

  /******************ENABLE******************/
  memset(str_buf, 0x00, sizeof(str_buf));
  sprintf(str_buf,"%s%d%s%s", PWM_BASE_PATH, PWM7, PWM0_PATH, PWM_ENA_PATH);
  // open active_low file
  if((fd_input = open(str_buf, O_RDWR)) <=0) {
      perror("Unable to open enable");
      return EXIT_FAILURE;
  } 
  else { 
    if(write(fd_input, "0", 1) < 0) {
      if(errno != EBUSY) {
        perror("Unable to change enable");
        close_fd(fd_input);
        return EXIT_FAILURE;
      }
      perror("Warning: unable to change enable");
    }
    else {
      memset(tmp_str, 0x00, sizeof(tmp_str));
      usleep(100000);
      lseek(fd_input, 0, SEEK_SET);
      ret= read(fd_input, tmp_str, 10); 
      printf("Changed enable: %s", tmp_str);
      close_fd(fd_input);
    }
  }

  return EXIT_SUCCESS;
}
