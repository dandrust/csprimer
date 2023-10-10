#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

#define DEBUG 1

typedef struct file_header {
  u_int32_t magic_number;
  u_int16_t version_major;
  u_int16_t version_minor;
  u_int32_t timezone;
  u_int32_t timestamp_accuracy;
  u_int32_t snap_length;
  u_int32_t link_layer_type;
} PcapFileHeader;

typedef struct packet_header {
  u_int32_t timestamp_seconds;
  u_int32_t timestamp_subseconds;
  u_int32_t captured_packet_length;
  u_int32_t original_packet_length;
} PacketHeader;

typedef struct null_link_layer_header { 
  u_int32_t address_family;
} NullLoopbackHeader;

typedef struct ipv4_header {
  u_int8_t octet_one;
  u_int8_t octet_two;
  u_int16_t total_length;
  u_int16_t identification;
  u_int16_t flags_fragment_offset;
  u_int8_t ttl;
  u_int8_t protocol;
  u_int16_t header_checksum;
  u_int64_t source_ip_address;
  u_int64_t destination_ip_address;
} InternetHeader;

u_int8_t internet_header_length(InternetHeader* header) {
  // return lower 4 bits
  return header->octet_one & 0x0F;
}

void print_file_header(PcapFileHeader* header) {
  printf("[PCAP] Magic Number:\t0x%x\n", header->magic_number);
  printf("[PCAP] Major Version:\t0x%x\n", header->version_major);
  printf("[PCAP] Minor Version:\t0x%x\n", header->version_minor);
  printf("[PCAP] Timezone:\t0x%x\n", header->timezone);
  printf("[PCAP] Timestamp Accuracy:\t0x%x\n", header->timestamp_accuracy);
  printf("[PCAP] Snap Length:\t0x%x\n", header->snap_length);
  printf("[PCAP] Link-layer Type:\t0x%x\n", header->link_layer_type);
  printf("\n");
}

void print_packet_header(PacketHeader* header) {
  printf("[PKT] Timestamp (seconds):\t0x%x\n", header->timestamp_seconds);
  printf("[PKT] Timestamp (subseconds):\t0x%x\n", header->timestamp_subseconds);
  printf("[PKT] Captured Length:\t0x%x\n", header->captured_packet_length);
  printf("[PKT] Original Length:\t0x%x\n", header->original_packet_length);
  printf("\n");
}

int main() {
  int fd = open("synflood.pcap", O_RDONLY);

  PcapFileHeader *header = malloc(sizeof(PcapFileHeader));

  read(fd, header, sizeof(PcapFileHeader));

  if (DEBUG) print_file_header(header);
  
  PacketHeader *pkt_hdr = malloc(sizeof(PacketHeader));
  NullLoopbackHeader * ll_hdr = malloc(sizeof(NullLoopbackHeader));

  // until EOF
    read(fd, pkt_hdr, sizeof(PacketHeader));
    if (DEBUG) print_packet_header(pkt_hdr);

    u_int32_t packet_bytes_remaining = pkt_hdr->captured_packet_length;
  
    // Read the Link Layer Header
    read(fd, ll_hdr, sizeof(NullLoopbackHeader));
    if (DEBUG) printf("[LL] Address Family:\t0x%x\n", ll_hdr->address_family);
    if (DEBUG) printf("[LL] AF_INET:\t0x%x\n\n", AF_INET);

    packet_bytes_remaining -= sizeof(NullLoopbackHeader);

    // Read the Internet Header




  free(ll_hdr);
  free(pkt_hdr);
  free(header);
  close(fd);
}