#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <string>
#include <iostream>

struct fabs_appif_header {
    union {
        uint32_t b32; // big endian
        uint8_t  b128[16];
    } l3_addr1;

    union {
        uint32_t b32; // big endian
        uint8_t  b128[16];
    } l3_addr2;

    timeval  tm; // machine-dependent endian

    uint16_t l4_port1; // big endian
    uint16_t l4_port2; // big endian

    uint8_t  event; // 0: created, 1: destroyed, 2: data
    uint8_t  from;  // FROM_ADDR1 (== 0): from addr1, FROM_ADDR2 (== 1): from addr2
    uint16_t len;   // machine-dependent endian
    uint8_t  hop;
    uint8_t  l3_proto; // IPPROTO_IP or IPPROTO_IPV6
    uint8_t  l4_proto; // IPPROTO_TCP or IPPROTO_UDP
    uint8_t  match; // 0: matched up's regex, 1: matched down's regex, 2: none

    uint8_t  unused[4]; // safety packing for 32 bytes boundary
} __attribute__((packed, aligned(32)));


static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
            ret += '=';
    }

    return ret;
}

void
print_ip_json(const icmp *ptr_icmp, const fabs_appif_header &hdr)
{
    const ip *p = &ptr_icmp->icmp_ip;

    if (hdr.len < 8 + sizeof(ip))
        return;

    printf("\"ip\":{");

    printf("\"ver\":%d,", p->ip_v);
    printf("\"hlen\":%d,", p->ip_hl);
    printf("\"tos\":%d,", p->ip_tos);
    printf("\"len\":%d,", ntohs(p->ip_len));
    printf("\"id\":%d,", ntohs(p->ip_id));
    printf("\"offset\":%d,", ntohs(p->ip_off));
    printf("\"ttl\":%d,", p->ip_ttl);
    printf("\"proto\":%d,", p->ip_p);
    printf("\"cksum\":%d,", ntohs(p->ip_sum));

    char buf[128];
    inet_ntop(PF_INET, &p->ip_src, buf, sizeof(buf));
    printf("\"src\":\"%s\",", buf);

    inet_ntop(PF_INET, &p->ip_dst, buf, sizeof(buf));
    printf("\"dst\":\"%s\",", buf);

    std::string data;
    int len = hdr.len - (8 + sizeof(ip));
    if (len > 0) {
        data = base64_encode((unsigned char*)p + len, len);
    }

    printf("\"data\":\"%s\"", data.c_str());

    printf("}");
}

void
print_json(const icmp *p, const fabs_appif_header &hdr)
{
    if (hdr.len < 8) {
        printf("\"msg\":\"too short packet\",");

        std::string data;
        data = base64_encode((unsigned char*)p, hdr.len);

        printf("\"data\":\"%s\"", data.c_str());
        return;
    }

    printf("{");

    char buf1[128], buf2[128];
    snprintf(buf1, sizeof(buf1), "%lf", (hdr.tm.tv_sec + hdr.tm.tv_usec * 1e-6));

    printf("\"time\":%s,", buf1);
    printf("\"len\":%d,", hdr.len);

    if (hdr.l3_proto == IPPROTO_IP) {
        inet_ntop(PF_INET, &hdr.l3_addr1.b32, buf1, sizeof(buf1));
        inet_ntop(PF_INET, &hdr.l3_addr2.b32, buf2, sizeof(buf2));
    } else if (hdr.l3_proto == IPPROTO_IPV6) {
        inet_ntop(PF_INET6, hdr.l3_addr1.b128, buf1, sizeof(buf1));
        inet_ntop(PF_INET6, hdr.l3_addr2.b128, buf2, sizeof(buf2));
    }

    if (hdr.from == 0) {
        printf("\"src\":\"%s\",", buf1);
    } else {
        printf("\"src\":\"%s\",", buf2);
    }

    if (hdr.from == 0) {
        printf("\"dst\":\"%s\",", buf2);
    } else {
        printf("\"dst\":\"%s\",", buf1);
    }

    printf("\"type\":%d,", p->icmp_type);
    printf("\"code\":%d,", p->icmp_code);
    printf("\"chksum\":%d,", p->icmp_cksum);

    if (p->icmp_type == 0 || p->icmp_type == 8) { // echo and echo reply
        printf("\"id\":%d,", ntohs(p->icmp_id));
        printf("\"seq\":%d,", ntohs(p->icmp_seq));

        std::string data;
        int len = hdr.len - 8;
        if (len > 0) {
            data = base64_encode((unsigned char*)p->icmp_data, len);
        }

        printf("\"data\":\"%s\"", data.c_str());
    } else if (p->icmp_type == 3 || p->icmp_type == 3 || p->icmp_type == 4) { // unreachable, time exceeded, and source quench
        printf("\"padding\":%d,", ntohl(p->icmp_void));
        print_ip_json(p, hdr);
    } else if (p->icmp_type == 15 || p->icmp_type == 15) { // information and information reply
        printf("\"id\":%d,", ntohs(p->icmp_id));
        printf("\"seq\":%d", ntohs(p->icmp_seq));
    } else if (p->icmp_type == 15) { // parameter problem
        printf("\"pointer\":%d,", p->icmp_pptr);
        print_ip_json(p, hdr);
    } else if (p->icmp_type == 5) { // redirect
        inet_ntop(PF_INET, &p->icmp_gwaddr, buf1, sizeof(buf1));
        printf("\"gateway\":\"%s\",", buf1);
        print_ip_json(p, hdr);
    } else if (p->icmp_type == 13 || p->icmp_type == 14) {
        printf("\"id\":%d,", ntohs(p->icmp_id));
        printf("\"seq\":%d,", ntohs(p->icmp_seq));
        printf("\"otime\":%d,", ntohl(p->icmp_otime));
        printf("\"rtime\":%d,", ntohl(p->icmp_rtime));
        printf("\"ttime\":%d", ntohl(p->icmp_ttime));
    } else {
        printf("\"msg\":\"invalid message type\",");

        std::string data;
        data = base64_encode((unsigned char*)p, hdr.len);

        printf("\"data\":\"%s\"", data.c_str());
    }

    printf("}\n");
}

int
main(int argc, char *argv[])
{
    int result;
    std::string uxpath = (char*)"/tmp/sf-tap/icmp/icmp";

    while((result = getopt(argc, argv, "u:pjhb"))!=-1){
        switch (result) {
        case 'u':
            uxpath = optarg;
            break;
        case 'h':
        default:
            std::cout << argv[0] << " -u unix_domain_path\n"
                      << std::endl;
            return 0;
        }
    }

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("socket");
        exit(1);
    }

    struct sockaddr_un sa = {0};
    sa.sun_family = AF_UNIX;
    strcpy(sa.sun_path, uxpath.c_str());

    if (connect(sock, (struct sockaddr*) &sa, sizeof(struct sockaddr_un)) == -1)
    {
        perror("connect");
        exit(1);
    }

    for (;;) {
        // read binary header
        fabs_appif_header hdr;
        int len = read(sock, &hdr, sizeof(hdr));
        if (len < (int)sizeof(hdr)) {
            perror("couldn't read");
            exit(1);
        }

        char buf[65536];
        len = read(sock, buf, hdr.len);
        if (len < hdr.len) {
            perror("couldn't read");
            exit(1);
        }

        print_json((icmp*)buf, hdr);
    }

    return 0;
}