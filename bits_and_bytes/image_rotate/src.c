#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DEBUG 1

#define PIXEL_ARRAY_OFFSET_OFFSET 10
#define WIDTH_OFFSET 18
#define HEIGHT_OFFSET 22
#define BPP_OFFSET 28
#define COMPRESSION_OFFSET 30

typedef struct Pixel {
  u_int8_t red;
  u_int8_t green;
  u_int8_t blue;
} Pixel;

struct bmp_data {
  u_int32_t pixel_array_offset;
  int32_t width;
  int32_t height;
  u_int16_t bpp;
  u_int32_t compression;
  ssize_t row_size_bytes;
};

void print_image_data(struct bmp_data *data) {
  printf("Pixel array offset at (hex)\t0x%x\n", data->pixel_array_offset);
  printf("Image height (dec)\t%u\n", data->height);
  printf("Image width (dec)\t%u\n", data->width);
  printf("Bits per pixel (dec)\t%u\n", data->bpp);
  printf("Compression (hex)\t0x%x\n", data->compression);
}

int main() {
  struct bmp_data *image_data = malloc(sizeof(struct bmp_data));

  int fd = open("./teapot.bmp", O_RDWR);

  pread(fd, &image_data->pixel_array_offset, sizeof(u_int32_t),
        PIXEL_ARRAY_OFFSET_OFFSET);
  pread(fd, &image_data->width, sizeof(u_int32_t), WIDTH_OFFSET);
  pread(fd, &image_data->height, sizeof(u_int32_t), HEIGHT_OFFSET);
  pread(fd, &image_data->bpp, sizeof(u_int16_t), BPP_OFFSET);
  pread(fd, &image_data->compression, sizeof(u_int32_t), COMPRESSION_OFFSET);
  image_data->row_size_bytes = image_data->width * sizeof(Pixel);

  if (DEBUG) {
    print_image_data(image_data);
  }

  // Do I have to free this? I don't think so
  struct bmp_data output_image_data = *image_data;

  output_image_data.width = image_data->height;
  output_image_data.height = image_data->width;
  output_image_data.row_size_bytes = output_image_data.width * sizeof(Pixel);

  // Array of pointers, pointing to arrays of Pixel structs
  Pixel **output = malloc(output_image_data.height * sizeof(Pixel *));

  // Populate pointers to arrays of Pixel
  for (int32_t i = 0; i < output_image_data.height; i++) {
    output[i] = malloc(output_image_data.row_size_bytes);
  }

  // Initialize a buffer for reading rows from the original
  Pixel *buffer = malloc(image_data->row_size_bytes);

  // Seek to where pixel data starts
  lseek(fd, image_data->pixel_array_offset, SEEK_SET);

  // Read the image file row by row
  for (int32_t row = 0; row < image_data->height; row++) {
    read(fd, buffer, image_data->row_size_bytes);

    for (int32_t col = 0; col < image_data->width; col++) {
      // Transformation for 90 deg rotation is [r][c] -> [-c][r]
      // Since positions are zero-indexed from the top left
      // [-c] means subtracting column position from
      // max column position (ie, width)
      output[image_data->width - 1 - col][row] = buffer[col];
    }
  }

  // Seek to *pixel_array_offset again
  lseek(fd, image_data->pixel_array_offset, SEEK_SET);

  // Write the value of output_rows to the file
  for (int32_t row = 0; row < output_image_data.height; row++) {
    write(fd, output[row], output_image_data.row_size_bytes);
  }

  // Clean up
  close(fd);

  free(image_data);
  free(buffer);
  for (int32_t j = 0; j < output_image_data.height; j++) {
    free(output[j]);
  }
  free(output);
}
