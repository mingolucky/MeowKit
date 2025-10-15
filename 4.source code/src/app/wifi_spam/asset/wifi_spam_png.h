#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>

// 定义图像结构体
typedef struct {
    const uint8_t *data;
    uint16_t width;
    uint16_t height;
    uint8_t dataSize;
} tImage;

// 声明图像对象（在 C 文件中定义）
extern const tImage wifi_killer;

#endif