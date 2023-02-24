#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include "esp32-proxy.h"

#include "mini_pupper_host_base.h"
#include "mini_pupper_protocol.h"

static bool const print_debug     {false};
static bool const print_debug_max {false};

static char const * filename {"/dev/ttyAMA1"};

static char const * version = PROJECT_VER;

// Setpoint and feedback data format for client-server communication (PoD)
struct setpoint_and_feedback_data
{
    parameters_control_instruction_format control;
    parameters_control_acknowledge_format feedback;
};

// Task handling communication with ESP32
// - parameter (input/output) : the client/server shared-memory buffer
void esp32_protocol(setpoint_and_feedback_data * control_block)
{
    // Check control block once
    if(control_block==NULL) exit(EXIT_FAILURE);
    
    // Initialize all control/goal_position to neutral
    u16 const neutral_pos {512};
    for(auto & goal_position : control_block->control.goal_position)
    {
        goal_position = neutral_pos;
    }

    // Initialize all control/torque_switch to enable
    for(auto & torque_enable : control_block->control.torque_enable)
    {
        torque_enable = 1;
    }

    // reference : https://www.pololu.com/docs/0J73/15.5

    // Open serial device
    int fd = open(
        filename,           // UART device
        O_RDWR | O_NOCTTY   // Read-Write access + Not the terminal of this process
    );
    if (fd < 0)
    {
        printf("%s: failed to open UART device\n", __func__);
        exit(EXIT_FAILURE);
    }

    // Flush away any bytes previously read or written.
    int result = tcflush(fd, TCIOFLUSH);
    if (result)
    {
        printf("%s: failed to flush\r\n", __func__);
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Get the current configuration of the serial port.
    struct termios options;
    result = tcgetattr(fd, &options);
    if (result)
    {
        printf("%s: failed to allocate UART TTY instance\n", __func__);
        close(fd);
        exit(EXIT_FAILURE);
    }

    // note : Les fonctions termios établissent une interface générale 
    //        sous forme de terminal, permettant de contrôler les ports 
    //        de communication asynchrone.  

    // Turn off any options that might interfere with our ability to send and
    // receive raw binary bytes.
    options.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF); // IGNPAR ?
    options.c_oflag &= ~(ONLCR | OCRNL);
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

    // Set up timeouts: Calls to read() will return as soon as there is
    // at least one byte available or when 100 ms has passed.
    options.c_cc[VTIME] = 1;
    options.c_cc[VMIN] = 0;

    // Set baud rate
    cfsetospeed(&options, B3000000);

    // Apply options
    result = tcsetattr(fd, TCSANOW, &options);
    if (result)
    {
        printf("%s: failed to set attributes\r\n", __func__);
        close(fd);
        exit(EXIT_FAILURE);
    }

    // control-loop
    for (;;)
    {
        // heart-beat
        if (print_debug_max)
        {
            printf(".");
        }

        // Flush away any bytes previously read or written.
        int result = tcflush(fd, TCIOFLUSH);
        if (result)
        {
            printf("%s: failed to flush\r\n", __func__);
            close(fd);
            exit(EXIT_FAILURE);
        }

        /*
         * Encode a CONTROL frame
         */

        // Compute the size of the payload (parameters length + 2)
        size_t const tx_payload_length { sizeof(parameters_control_instruction_format) + 2 };

        // Compute the size of the frame
        size_t const tx_buffer_size { tx_payload_length + 4 };

        // Build the frame
        u8 tx_buffer[tx_buffer_size]
        {
            0xFF,               // header
            0xFF,               // header
            0x01,               // default ID
            tx_payload_length,  // length
            INST_CONTROL        // instruction
        };
        memcpy(tx_buffer+5,&control_block->control,sizeof(parameters_control_instruction_format));

        // Checksum
        tx_buffer[tx_buffer_size-1] = compute_checksum(tx_buffer);

        // Send
        result = write(fd, (char *)tx_buffer, tx_buffer_size);
        if(result != (ssize_t)tx_buffer_size)
        {
            printf("failed to write to port");
            close(fd);
            exit(EXIT_FAILURE);
        }
        if (print_debug_max)
        {
    	    printf("uart writen:%d\n",result);
    	}

        /*
        result = tcdrain(fd);
        if(result)
        {
            printf("failed to drain port");
            close(fd);
            exit(EXIT_FAILURE);
        }
        */

        // Read buffer
    	// If we do not get data within 1 second we assume ESP32 stopped sending and we send again
        size_t const rx_buffer_size { 128 };
        u8 rx_buffer[rx_buffer_size] {0};
    	size_t const expected_lenght {4 + 1 + sizeof(parameters_control_acknowledge_format) + 1}; // header + status + pos + load + chksum
        size_t received_length {0};
    	while(received_length<expected_lenght)
        {
            ssize_t read_length = read(
                fd,
                (char*)(rx_buffer+received_length),
                expected_lenght-received_length
            );
            if(read_length<0)
            {
                printf("failed to read from port");
                close(fd);
                exit(EXIT_FAILURE);
            }
            if (print_debug_max)
            {
                printf("uart read:%lu, waiting for %lu/%lu\n",read_length, received_length,expected_lenght);
            }
            if(read_length==0)
            {
              // time out
              break;
            }
    	    received_length += read_length;
    	}

        // not enough data received ?
    	if(received_length<expected_lenght) continue;

        /*
         * Decode a CONTROL ACK frame
         */

        bool const rx_header_check {
                    (rx_buffer[0]==0xFF)
                &&  (rx_buffer[1]==0xFF)
                &&  (rx_buffer[2]==0x01) // my ID
                &&  (rx_buffer[3]<=(rx_buffer_size-4)) // keep the header in the rx buffer
        };
        if(!rx_header_check)
        {
            // log
            if (print_debug)
    	    {
                printf("RX frame error : header invalid!\n");
    	    }
            // next
            continue;
        }

        // checksum
        u8 expected_checksum {0};
        bool const rx_checksum {checksum(rx_buffer,expected_checksum)};

        // waiting for a valid instruction and checksum...
        bool const rx_payload_checksum_check {
                    (rx_buffer[4]==0x00)
                &&  rx_checksum
        };
        if(!rx_payload_checksum_check)
        {
            // log
            if (print_debug)
    	    {
                    printf("RX frame error : bad instruction [%d] or checksum [received:%d,expected:%d]!\n",rx_buffer[4],rx_buffer[rx_buffer[3]+3],expected_checksum);
    	    }
            // next
            continue;
        }

        // decode parameters
        memcpy(&control_block->feedback,rx_buffer+5,sizeof(parameters_control_acknowledge_format));

        // log
        if (print_debug)
    	{
            printf("Present Position: %d %d %d %d %d %d %d %d %d %d %d %d\n",
            control_block->feedback.present_position[0],control_block->feedback.present_position[1],control_block->feedback.present_position[2],
            control_block->feedback.present_position[3],control_block->feedback.present_position[4],control_block->feedback.present_position[5],
            control_block->feedback.present_position[6],control_block->feedback.present_position[7],control_block->feedback.present_position[8],
            control_block->feedback.present_position[9],control_block->feedback.present_position[10],control_block->feedback.present_position[11]
            );
            printf("            Load: %d %d %d %d %d %d %d %d %d %d %d %d\n",
            control_block->feedback.present_load[0],control_block->feedback.present_load[1],control_block->feedback.present_load[2],
            control_block->feedback.present_load[3],control_block->feedback.present_load[4],control_block->feedback.present_load[5],
            control_block->feedback.present_load[6],control_block->feedback.present_load[7],control_block->feedback.present_load[8],
            control_block->feedback.present_load[9],control_block->feedback.present_load[10],control_block->feedback.present_load[11]
            );

            printf("Attitude:  ax:%.3f  ay:%.3f  az:%.3f  gx:%.3f  gy:%.3f  gz:%.3f\n",
                control_block->feedback.ax, control_block->feedback.ay, control_block->feedback.az,
                control_block->feedback.gx, control_block->feedback.gy, control_block->feedback.gz
            );
            printf("Power:  %.3fV  %.3fA\n", control_block->feedback.voltage_V, control_block->feedback.current_A);
	   }
    }
}

int main(int argc, char *argv[])
{
    // allocate a shared-memory buffer for setpoint and feedback data exchange between clients and server
    void * control_block = mmap(NULL, sizeof(setpoint_and_feedback_data), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    memset(control_block, 0, sizeof(setpoint_and_feedback_data));

    /* print version string */
    printf("%s\n", version);

    /* start UART protocol with ESP32 */
    int pid = fork();
    if (pid == 0)
    {
	   esp32_protocol(reinterpret_cast<setpoint_and_feedback_data*>(control_block));
    }

    /* Create local socket. */

    int connection_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (connection_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /*
     * For portability clear the whole structure, since some
     * implementations have additional (nonstandard) fields in
     * the structure.
     */

    struct sockaddr_un name;
    memset(&name, 0, sizeof(name));

    /* Bind socket to socket name. */

    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, SOCKET_NAME, sizeof(name.sun_path) - 1);
    int ret = unlink(SOCKET_NAME);
    if (ret == -1 && errno != ENOENT) {
        printf("unlink %s", strerror(errno));
        perror("unlink");
        exit(EXIT_FAILURE);
    }
    ret = bind(connection_socket, (const struct sockaddr *) &name,
               sizeof(name));
    if (ret == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    /*
     * Prepare for accepting connections. The backlog size is set
     * to 20. So while one request is being processed other requests
     * can be waiting.
     */

    ret = listen(connection_socket, 20);
    if (ret == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    /* This is the main loop for handling connections. */

    int data_socket {0};
    u8 r_buffer[38];
    u8 s_buffer[38];
    for (;;) {

        /* Wait for incoming connection. */

        data_socket = accept(connection_socket, NULL, NULL);
        if (data_socket == -1) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

	pid = fork();
        if (pid == 0) {

            for (;;) {

                /* Wait for next data packet. */

                ret = read(data_socket, r_buffer, sizeof(r_buffer));
                if (ret == -1) {
                    /* client terminated? */
                    exit(EXIT_SUCCESS);
                }

                /* Handle commands. */
		int offset;
                offset = 0;
                if(r_buffer[1] == INST_SETPOS && r_buffer[0] == 38) {
                    memcpy((char*)control_block + offset, &r_buffer[2], sizeof(parameters_control_instruction_format));
                    s_buffer[0]= 2;
                    s_buffer[1]= INST_SETPOS;
                }

                offset = sizeof(parameters_control_instruction_format);
                if(r_buffer[1] == INST_GETPOS && r_buffer[0] == 2) {
                    s_buffer[0]= 2 + 12*sizeof(u16);
                    s_buffer[1]= INST_GETPOS;
                    memcpy(&s_buffer[2], (char*)control_block + offset, 12*sizeof(u16));
                }

                offset += 12*sizeof(u16);
                if(r_buffer[1] == INST_GETLOAD && r_buffer[0] == 2) {
                    s_buffer[0]= 2 + 12*sizeof(s16);
                    s_buffer[1]= INST_GETLOAD;
                    memcpy(&s_buffer[2], (char*)control_block + offset, 12*sizeof(s16));
                }

                offset += 12*sizeof(s16);
                if(r_buffer[1] == INST_GETIMU && r_buffer[0] == 2) {
                    s_buffer[0]= 2 + 6*sizeof(float);
                    s_buffer[1]= INST_GETIMU;
                    memcpy(&s_buffer[2], (char*)control_block + offset, 6*sizeof(float));
                }

                offset += 6*sizeof(float);
                if(r_buffer[1] == INST_GETPOWER && r_buffer[0] == 2) {
                    s_buffer[0]= 2 + 2*sizeof(float);
                    s_buffer[1]= INST_GETPOWER;
                    memcpy(&s_buffer[2], (char*)control_block + offset, 2*sizeof(float));
                }

                /* Send result. */

                ret = write(data_socket, s_buffer, s_buffer[0]);
                if (ret == -1) {
                    /* client terminated? */
                    exit(EXIT_SUCCESS);
                }

            }
        }
        else {
            close(data_socket);
            continue;
        }
    }

    close(connection_socket);

    /* Unlink the socket. */

    unlink(SOCKET_NAME);

    exit(EXIT_SUCCESS);
}
