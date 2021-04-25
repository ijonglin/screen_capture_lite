//
// Created by ijonglin on 4/25/21.
//
#include "EPaperUpdater.h"

int EPaperUpdater::memory_write(int fd, unsigned int addr, unsigned int length, char *data)
{
    unsigned char write_cmd[12] = {
        0xfe, 0x00,
        (unsigned char) ((addr >> 24) & 0xff),
        (unsigned char) ((addr >> 16) & 0xff),
        (unsigned char) ((addr >> 8) & 0xff),
        (unsigned char) (addr && 0xff),
        0x82,
        (unsigned char) ((length >> 8) & 0xff),
        (unsigned char) (length & 0xff),
        0x00, 0x00, 0x00
    };

    sg_io_hdr_t io_hdr;

    int i;
    for (i = 0; i < 12; i += 4) {
        printf("%02X %02X %02X %02X\n", write_cmd[i], write_cmd[i + 1],
               write_cmd[i + 2], write_cmd[i + 3]);
    }
    printf("\n");

    memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = 12;
    io_hdr.dxfer_direction = SG_DXFER_TO_DEV;
    io_hdr.dxfer_len = length;
    io_hdr.dxferp = data;
    io_hdr.cmdp = write_cmd;
    io_hdr.timeout = 10000;

    if (ioctl(fd, SG_IO, &io_hdr) < 0) {
        perror("SG_IO memory write failed");
    }

    return 0;
}

int EPaperUpdater::load_image_area(int fd, int addr, int x, int y, int w, int h,
                unsigned char *data)
{
    unsigned char load_image_cmd[16] = {
        0xfe, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xa2
    };

    IT8951_area area;
    memset(&area, 0, sizeof(IT8951_area));
    area.address = addr;
    area.x = __bswap_32(x);
    area.y = __bswap_32(y);
    area.w = __bswap_32(w);
    area.h = __bswap_32(h);

    int length = w * h;

    auto *data_buffer = (unsigned char *) malloc(length + sizeof(IT8951_area));
    memcpy(data_buffer, &area, sizeof(IT8951_area));
    memcpy(&data_buffer[sizeof(IT8951_area)], data, length);

    sg_io_hdr_t io_hdr;

    memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = 16;
    io_hdr.dxfer_direction = SG_DXFER_TO_DEV;
    io_hdr.dxfer_len = length + sizeof(IT8951_area);
    io_hdr.dxferp = data_buffer;
    io_hdr.cmdp = load_image_cmd;
    io_hdr.timeout = 5000;

    if (ioctl(fd, SG_IO, &io_hdr) < 0) {
        perror("SG_IO image load failed");
    }
    return 0;
}

int EPaperUpdater::display_area(int fd, int addr, int x, int y, int w, int h, int mode)
{
    unsigned char display_image_cmd[16] = {
        0xfe, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x94
    };

    IT8951_display_area area;
    memset(&area, 0, sizeof(IT8951_display_area));
    area.address = addr;
    area.x = __bswap_32(x);
    area.y = __bswap_32(y);
    area.w = __bswap_32(w);
    area.h = __bswap_32(h);
    area.wait_ready = __bswap_32(1);
    area.wavemode = __bswap_32(mode);

    unsigned char *data_buffer = (unsigned char *) malloc(sizeof(IT8951_display_area));
    memcpy(data_buffer, &area, sizeof(IT8951_display_area));

    sg_io_hdr_t io_hdr;

    memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = 16;
    io_hdr.dxfer_direction = SG_DXFER_TO_DEV;
    io_hdr.dxfer_len = sizeof(IT8951_display_area);
    io_hdr.dxferp = data_buffer;
    io_hdr.cmdp = display_image_cmd;
    io_hdr.timeout = 5000;

    if (ioctl(fd, SG_IO, &io_hdr) < 0) {
        perror("SG_IO display failed");
    }
    return 0;
}

int EPaperUpdater::pmic_set(int fd, int power, int vcom)
{


    unsigned char load_image_cmd[16] = {
        0xfe, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xa3,
        static_cast<unsigned char>((vcom >> 8) & 0xff),
        static_cast<unsigned char>(vcom & 0xff),
        0x01,
        0x01,
        static_cast<unsigned char>(power & 0xff)
    };

    sg_io_hdr_t io_hdr;

    memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = 16;
    io_hdr.dxfer_direction = SG_DXFER_TO_DEV;
    io_hdr.dxfer_len = 0;
    io_hdr.cmdp = load_image_cmd;
    io_hdr.timeout = 5000;

    if (ioctl(fd, SG_IO, &io_hdr) < 0) {
        perror("SG_IO power set failed");
    }
    return 0;
}

void EPaperUpdater::update_region(const char *filename, int x, int y, int w, int h, int mode)
{

    unsigned char inquiry_cmd[6] = {0x12, 0, 0, 0, 0, 0};
    unsigned char inquiry_result[96];
    unsigned char deviceinfo_cmd[12] = {
        0xfe, 0x00, // SCSI Customer command
        0x38, 0x39, 0x35, 0x31, // Chip signature
        0x80, 0x00, // Get System Info
        0x01, 0x00, 0x02, 0x00 // Version
    };
    unsigned char deviceinfo_result[112];


    sg_io_hdr_t io_hdr;

    memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = 6;
    io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
    io_hdr.dxfer_len = 96;
    io_hdr.dxferp = inquiry_result;
    io_hdr.cmdp = inquiry_cmd;
    io_hdr.timeout = 1000;

    int fd;
    if (ioctl(fd, SG_IO, &io_hdr) < 0) {
        perror("SG_IO INQUIRY failed");
    }

    IT8951_inquiry *inquiry = (IT8951_inquiry *) inquiry_result;

    if (strncmp(reinterpret_cast<const char *>(inquiry->vendor_id), "Generic ", 8) != 0) {
        fprintf(stderr, "SCSI Vendor does not match\n");
        exit(EXIT_FAILURE);
    }
    if (strncmp(reinterpret_cast<const char *>(inquiry->product_id), "Storage RamDisc ", 8) != 0) {
        fprintf(stderr, "SCSI Product does not match\n");
        exit(EXIT_FAILURE);
    }
    if (strncmp(reinterpret_cast<const char *>(inquiry->product_ver), "1.00", 4) != 0) {
        fprintf(stderr, "SCSI Productver does not match\n");
        exit(EXIT_FAILURE);
    }

    if (debug == 1) {
        printf("Fetching device info\n");
    }

    memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = sizeof(deviceinfo_cmd);
    io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
    io_hdr.dxfer_len = 112;
    io_hdr.dxferp = deviceinfo_result;
    io_hdr.cmdp = deviceinfo_cmd;
    io_hdr.timeout = 10000;

    if (ioctl(fd, SG_IO, &io_hdr) < 0) {
        perror("SG_IO device info failed");
        exit(EXIT_FAILURE);
    }

    IT8951_deviceinfo *deviceinfo = (IT8951_deviceinfo *) deviceinfo_result;

    int width = __bswap_32(deviceinfo->width);
    int height = __bswap_32(deviceinfo->height);

    if (debug == 1) {
        printf("Found a %dx%d epaper display\n", width, height);
    }

    int addr = deviceinfo->image_buffer_addr;
    unsigned char *image;
    image = nullptr;
    int stride;
    fill_buffer_from_stdin(w, h, image, stride);

    int offset = 0;
    int lines = MAX_TRANSFER / w;
    int size = w * h;
    while (offset < size) {
        if ((offset / w) + lines > h) {
            lines = h - (offset / w);
        }
        if (debug == 1) {
            printf("Sending %dx%d chunk to %d,%d\n", w, lines, x, y + (offset / w));
        }
        load_image_area(fd, addr, x, y + (offset / w), w, lines, &image[offset]);
        offset += lines * w;
    }
    if (debug == 1) {
        printf("Starting refresh\n");
    }
    display_area(fd, addr, x, y, w, h, mode);
}

void EPaperUpdater::fill_buffer_from_stdin(int w, int h, unsigned char *&image, int& stride) const
{
    int size= w * h;
    if (image == nullptr) {
            image= (unsigned char *) malloc(size);
        }

    if (clear == 1) {
        memset(image, 0xff, size);
    } else {
        size_t total_left = size;
        unsigned char *buffer_pointer = image;
        while (total_left > 0) {
            size_t current = read(STDIN_FILENO, buffer_pointer, total_left);
            if (current < 0) {
                perror("stdin read");
                exit(EXIT_FAILURE);
            } else if (current == 0) {
                fprintf(stderr, "stdin input is truncated\n");
                exit(EXIT_FAILURE);
            } else {
                total_left -= current;
                buffer_pointer += current;
            }
        }
    }

    stride = w;
}

EPaperUpdater::EPaperUpdater(std::string theDevSpecifier)
{
    mDevSpecifier = theDevSpecifier;
    mDevIsOpen = false;
    mFileDescriptor = 0;
}

void EPaperUpdater::Open()
{
    // Do nothing for now, refactor so we only have to do this once
    if (mDevIsOpen)
        // Do nothing if it's already open.
        return;

    int to, res;

    mFileDescriptor = open(mDevSpecifier.c_str(), O_RDWR | O_NONBLOCK);
    if (mFileDescriptor < 0) {
        perror("Could not open scsi device");
        exit(EXIT_FAILURE);
    }

    res = ioctl(mFileDescriptor, SCSI_IOCTL_GET_BUS_NUMBER, &to);
    if (res < 0) {
        fprintf(stderr, "%s is not a SCSI device\n", mDevSpecifier.c_str());
        exit(EXIT_FAILURE);
    }

    unsigned char inquiry_cmd[6] = {0x12, 0, 0, 0, 0, 0};
    unsigned char inquiry_result[96];
    unsigned char deviceinfo_cmd[12] = {
        0xfe, 0x00,             // SCSI Customer command
        0x38, 0x39, 0x35, 0x31, // Chip signature
        0x80, 0x00,             // Get System Info
        0x01, 0x00, 0x02, 0x00  // Version
    };
    unsigned char deviceinfo_result[112];

    sg_io_hdr_t io_hdr;

    memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = 6;
    io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
    io_hdr.dxfer_len = 96;
    io_hdr.dxferp = inquiry_result;
    io_hdr.cmdp = inquiry_cmd;
    io_hdr.timeout = 1000;

    if (ioctl(mFileDescriptor, SG_IO, &io_hdr) < 0) {
        perror("SG_IO INQUIRY failed");
    }

    IT8951_inquiry *inquiry = (IT8951_inquiry *)inquiry_result;

    if (strncmp(reinterpret_cast<const char *>(inquiry->vendor_id), "Generic ", 8) != 0) {
        fprintf(stderr, "SCSI Vendor does not match\n");
        exit(EXIT_FAILURE);
    }
    if (strncmp(reinterpret_cast<const char *>(inquiry->product_id), "Storage RamDisc ", 8) != 0) {
        fprintf(stderr, "SCSI Product does not match\n");
        exit(EXIT_FAILURE);
    }
    if (strncmp(reinterpret_cast<const char *>(inquiry->product_ver), "1.00", 4) != 0) {
        fprintf(stderr, "SCSI Productver does not match\n");
        exit(EXIT_FAILURE);
    }

    if (debug == 1) {
        printf("Fetching device info\n");
    }

    memset(&io_hdr, 0, sizeof(sg_io_hdr_t));
    io_hdr.interface_id = 'S';
    io_hdr.cmd_len = sizeof(deviceinfo_cmd);
    io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
    io_hdr.dxfer_len = 112;
    io_hdr.dxferp = deviceinfo_result;
    io_hdr.cmdp = deviceinfo_cmd;
    io_hdr.timeout = 10000;

    if (ioctl(mFileDescriptor, SG_IO, &io_hdr) < 0) {
        perror("SG_IO device info failed");
        exit(EXIT_FAILURE);
    }

    IT8951_deviceinfo *deviceinfo = (IT8951_deviceinfo *)deviceinfo_result;

    int width = __bswap_32(deviceinfo->width);
    int height = __bswap_32(deviceinfo->height);

    if (debug == 1) {
        printf("Found a %dx%d epaper display\n", width, height);
    }
}

void EPaperUpdater::Close()
{
    close(mFileDescriptor);
    mFileDescriptor = 0;
    mDevIsOpen = false;
}

EPaperUpdater::~EPaperUpdater(void)
{
    if(mDevIsOpen) {
        Close();
    }
}



