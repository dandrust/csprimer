#include <stdio.h>
#include <stdlib.h>

// Given a buffer and a max bytelength,
// return the bytelenth at which the buffer
// can be cleanly truncated
size_t clean_truncate_length(size_t max_bytelength, char* buffer) {
  size_t pos = 0;
  size_t boundary = 0;

  // "Consume" chars until we've reached max bytelength
  while (pos < max_bytelength) {

  }
    
  return boundary;

  // walk thru the string
  // if max_bytelength 
  // if the char's MSB is 0, add 1 to counter and move on (it's an ASCII char)
  // if the char's MSB is 1, get the multi-byte size and increment count
}

#define DEBUG 0

int main() {
  size_t* buf_capacity_p = malloc(sizeof(size_t));
  *buf_capacity_p = 255;

  char* buf_p = malloc(*buf_capacity_p);

  FILE* cases = fopen("cases", "r");
  FILE* output = fopen("output", "w");

  int trunc_limit;
  ssize_t chars_read;
  size_t chars_written, n_to_write;
  
  // TODO: Why is this not detecting EOF?
  // feof looks for the EOF flag in the FILE struct - presumably it's not set?
  while (!feof(cases)) {
    trunc_limit = fgetc(cases);

    chars_read = getline(&buf_p, buf_capacity_p, cases);
    chars_read--; // "drop" newline

    if ((size_t)chars_read < trunc_limit) {
      n_to_write = chars_read;
    } else {
      n_to_write = (size_t)trunc_limit;
      // TODO: Implement and call
      // n_to_write = clean_truncate_length(trunc_limit, buf_p)
    }

    chars_written = fwrite(buf_p, sizeof(char), n_to_write, output);
    fputc('\n', output);
    
    if (DEBUG)
      printf("Wrote %zd chars to output from buffer: %s", chars_written, buf_p);
  }
  
  fclose(cases);
  fclose(output);

  free(buf_capacity_p);
  free(buf_p);

  return 0;
}