#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstddef>
#include <string.h>

#include <errno.h>

#include <time.h>

#define GPIO_BASE  0x41200000

#define GPIO_SIZE  0x1000

#define GPIO_DATA1  0x00
#define GPIO_DATA2 0x08

#define GPIO_TRI1   0x04
#define GPIO_TRI2  0x0C

#define WRITE_PIN_OFFSET 1
#define ADRESS_PIN_OFFSET 8

#define WRITE_PIN_MASK  (1 << WRITE_PIN_OFFSET)
#define ADRESS_PINS_MASK (255 << ADRESS_PIN_OFFSET)

void print_binary(unsigned int n) {
    // Determine the number of bits in the integer
    int bits = sizeof(n) * 8;
    int started = 0;

    for (int i = bits - 1; i >= 0; i--) {
        // Shift bit to the 1st position and mask it
        int bit = (n >> i) & 1;
        
        if (bit) started = 1; // Skip leading zeros
        
        if (started) {
            printf("%d", bit);
        }
    }
    if (!started) printf("0"); // If the number is 0
    printf("\n");
}

void writebit(uint32_t position, uint32_t value, volatile uint32_t* target) {
    uint32_t mask = 1 << position;

    //clamp value to 1
    if(value > 1) {
        value = 1;
    }
    uint32_t insert = value << position;

    //clearvalue at position
    *target = *target & (~mask);

    *target = *target | insert;
}

void writebits(uint32_t offset, uint32_t value, uint32_t nbits, volatile uint32_t* target) {

    uint32_t max_val = ((1 << nbits) - 1);
    uint32_t mask = max_val << offset;
    
    if(value > max_val) {
        value = max_val;
    }

    uint32_t insert = value << offset; 

    *target = *target & (~mask);

    *target = *target | insert;
}

#define DEBUG_BUILD

void writeCoefficient(volatile uint32_t* gpio, uint32_t value, uint32_t adress, uint32_t timing_ns = 1000) {

    //write the needed data in the data bus
    gpio[GPIO_DATA2 / 4] = value;

    
    //write in the adress
    writebits(ADRESS_PIN_OFFSET, adress, 8, &gpio[GPIO_DATA1 / 4]);
    
    
#ifdef DEBUG_BUILD
    printf("data: ");
    printf("%u\n", gpio[GPIO_DATA2 / 4]);
    printf("write address: ");
    printf("%u\n", adress);
#endif

    //create timing struct
    timespec ts{};
    ts.tv_sec = 0;
    ts.tv_nsec = timing_ns;

    //timing result TODO: figure out if the wait actually happened
    timespec rem{};

    //sleep to make sure the FPGA has read the values in the GPIO buffer
    nanosleep(&ts, &rem);

    //set write pin
    writebit(WRITE_PIN_OFFSET, 1, &gpio[GPIO_DATA1 / 4]);
    
    //sleep inbetween writes
    nanosleep(&ts, &rem);

    //reset the write pin
    writebit(WRITE_PIN_OFFSET, 0, &gpio[GPIO_DATA1 / 4]);

    nanosleep(&ts, &rem);
}

struct BiquadCoefficients {
    uint32_t b0;
    uint32_t b1;
    uint32_t b2;
    uint32_t a1;
    uint32_t a2;
};

struct FilterFileLayout {
    BiquadCoefficients filters[4];
};

void writeBiquad(BiquadCoefficients coeffs, uint32_t biquad, volatile uint32_t* gpio) {
    //the adresses in memory where the filters are located
    int32_t fpga_filtercoeff_base_adress[4] = {0, 5, 10, 15};

     //filter 1
    uint32_t adress = fpga_filtercoeff_base_adress[biquad];
    writeCoefficient(gpio, coeffs.b0, adress++);
    writeCoefficient(gpio, coeffs.b1, adress++);
    writeCoefficient(gpio, coeffs.b2, adress++);
    writeCoefficient(gpio, coeffs.a1, adress++);
    writeCoefficient(gpio, coeffs.a2, adress++);
}

void releaseShadowBuffer(volatile uint32_t* gpio) {
     //toggle shadow pulse
    timespec ts{};
    ts.tv_sec = 0;
    ts.tv_nsec = 1000;

    timespec rs{};

    writebit(2, 1, &gpio[GPIO_DATA1 / 4]);
    nanosleep(&ts, &rs);
    writebit(2, 0, &gpio[GPIO_DATA1 / 4]);
    nanosleep(&ts, &rs);
}

enum RequestType {
    CLOSE_CONNECTION = 0,
    CLOSE_SERVER = 1,
    RUN_BITFILE = 2,
    SET_COEFFICIENTS = 3
};

struct RequestHeader {
    RequestType type;
    uint32_t payload;
    uint32_t eor;
};

#pragma pack(push, 1)
    struct CoefficientWritePair {
        uint32_t address;
        uint32_t value;
    };
#pragma pack(pop)

int main (int argc, char **argv) {

    setbuf(stdout, NULL);

    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open /dev/mem");
        return 1;
    }

    volatile uint32_t *gpio1 = (volatile uint32_t*)mmap(
        nullptr, GPIO_SIZE,
        PROT_READ | PROT_WRITE,
        MAP_SHARED, fd, GPIO_BASE
    );

     // Configure all GPIO bits as outputs
    gpio1[GPIO_TRI1 / 4] =  0x00000000;
    gpio1[GPIO_TRI2 / 4] = 0x00000000;
    

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        printf("failed to create socket\n");
        return -1;
    }

    //listen on port 8000
    struct sockaddr_in serv_addr;
    uint16_t port = 8000;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);


    int opt = 1;

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                &opt, sizeof(opt)) < 0)
    {
        printf("failed to set reuse addr");
    }

    if(bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("failed to bind socket\n");
        printf("bind failed: %d (%s)\n", errno, strerror(errno));
        return -1;
    }


    if(listen(sockfd, SOMAXCONN) < 0) {
        printf("socket is not able to listen\n");
        return -1;
    }

    struct sockaddr_in client_addr;
    int cli_len = sizeof(client_addr);
    while(1) {
        printf("ready to accept new connection\n");
        int newsockfd = accept(sockfd, (sockaddr*) &client_addr, (socklen_t*) &cli_len);

        if(newsockfd < 0) {
            printf("socket accept failed\n");
            continue;
        }

        printf("accepted new connection\n");

        //we keep the socket open to accept new data
        while(1) {
            uint8_t message[1024] = {0};
            int n = recv(newsockfd, message, sizeof(message), 0);
            if(n < 0) {
                printf("an error occured on the connection!\n");
                break;
            }

            if(n == 0) {
                printf("the client killed the connection\n");
                break;
            }

            RequestHeader* h = (RequestHeader*)message;
            
            //if the connection is closed, we can accept new connections
            if(h->type == CLOSE_CONNECTION) {
                break;
            }

            if(h->type == CLOSE_SERVER) {
                printf("closing the server\n");

                shutdown(newsockfd, SHUT_RDWR);
                close(newsockfd);
                close(sockfd);
                return 0;
            }

            if(h->type == RUN_BITFILE) {
                char* path = (char*)h + h->payload;
                char cmd[1024] = {0};
                snprintf(cmd, 1024, "fpgautil -b %s", path);
                printf("%s\n", cmd);
                system(cmd); 
            }

            if(h->type == SET_COEFFICIENTS) {
                CoefficientWritePair* p = (CoefficientWritePair*)((char*)h + h->payload);
                CoefficientWritePair* end = (CoefficientWritePair*)((char*)h + h->eor);
                

                //write all coefficients that are in the are in the request
                while(p < end) {
                    writeCoefficient(gpio1, p->value, p->address);
                    p++;
                }

                releaseShadowBuffer(gpio1);
            }
        }

        shutdown(newsockfd, SHUT_RDWR);
        close(newsockfd);
    }

    close(sockfd);

   

    return 0;
}