//
// Created by ijonglin on 4/25/21.
//

#ifndef SCREEN_CAPTURE_LITE_EPAPERUPDATER_H
#define SCREEN_CAPTURE_LITE_EPAPERUPDATER_H

#include <byteswap.h>
#include <errno.h>
#include <fcntl.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string>

typedef struct it8951_inquiry {
    unsigned char dontcare[8];
    unsigned char vendor_id[8];
    unsigned char product_id[16];
    unsigned char product_ver[4];
} IT8951_inquiry;

typedef struct it8951_deviceinfo {
    unsigned int uiStandardCmdNo;
    unsigned int uiExtendedCmdNo;
    unsigned int uiSignature;
    unsigned int uiVersion;
    unsigned int width;
    unsigned int height;
    unsigned int update_buffer_addr;
    unsigned int image_buffer_addr;
    unsigned int temperature_segment;
    unsigned int ui_mode;
    unsigned int frame_count[8];
    unsigned int buffer_count;
    unsigned int reserved[9];
    void *command_table;
} IT8951_deviceinfo;

typedef struct it8951_area {
    int address;
    int x;
    int y;
    int w;
    int h;
} IT8951_area;

typedef struct it8951_display_area {
    int address;
    int wavemode;
    int x;
    int y;
    int w;
    int h;
    int wait_ready;
} IT8951_display_area;

class EPaperUpdater {
  public:
    EPaperUpdater(std::string theDevSpecifier);
    ~EPaperUpdater(void);

    void Open(void);
    void Close(void);
    int memory_write(int fd, unsigned int addr, unsigned int length, char *data);
    int load_image_area(int fd, int addr, int x, int y, int w, int h, unsigned char *data);
    int display_area(int fd, int addr, int x, int y, int w, int h, int mode);
    int pmic_set(int fd, int power, int vcom);
    void update_region(const char *filename, int x, int y, int w, int h, int mode);


  private:
    const int MAX_TRANSFER = 60800;
    const int debug = 0;
    const int clear = 0;
    std::string mDevSpecifier;
    bool mDevIsOpen;
    int mFileDescriptor;
    void fill_buffer_from_stdin(int w, int h, unsigned char *&image, int& stride) const;
};

#endif // SCREEN_CAPTURE_LITE_EPAPERUPDATER_H
