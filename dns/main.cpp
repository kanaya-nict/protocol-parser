
// macosx 
// cc main.c -lresolv
// linux, freebsd
// cc main.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/un.h>

#if defined(__linux__) || defined(__FreeBSD__)
#include <arpa/nameser.h>
#else
#include <nameser.h>
#endif

#include <resolv.h>

#include <iostream>
#include <string>
#include <map>
#include <vector>

void memdump_format0(void* buffer, int length)
{
    uint32_t* addr32 = (uint32_t*)buffer;
    int i;
    int j;
    int k;
    int lines = length/16 + (length%16?1:0);
    if (lines > 1) {
        for (i=0; i<lines; i++) {
            printf("    rdata:");
            printf("%08x %08x %08x %08x\n",
                    htonl(*(addr32)),
                    htonl(*(addr32+1)),
                    htonl(*(addr32+2)),
                    htonl(*(addr32+3))
                  );
            addr32 += 4;
        }
    }

    j = length%16;
    if (j == 0) return;
    k = 0;
    uint8_t*  addr8 = (uint8_t*)addr32;
    printf("    rdata:");
    for (i=0; i<16; i++) {
        if (k%4 == 0 && i != 0) printf(" ");
        if (j > i) {
            printf("%02x", *addr8);
            addr8++;
        } else {
            printf("XX");
        }
        k++;
    }
    printf("\n");
    return;
}

void memdump_format1(void* buffer, int length)
{
    uint32_t* addr32 = (uint32_t*)buffer;
    int i;
    int j;
    int k;
    int lines = length/16 + (length%16?1:0);
    if (lines > 1) {
        for (i=0; i<lines; i++) {
            printf("%08x %08x %08x %08x\n",
                    htonl(*(addr32)),
                    htonl(*(addr32+1)),
                    htonl(*(addr32+2)),
                    htonl(*(addr32+3))
                  );
            addr32 += 4;
        }
    }

    j = length%16;
    if (j == 0) return;
    k = 0;
    uint8_t*  addr8 = (uint8_t*)addr32;
    for (i=0; i<16; i++) {
        if (k%4 == 0 && i != 0) printf(" ");
        if (j > i) {
            printf("%02x", *addr8);
            addr8++;
        } else {
            printf("XX");
        }
        k++;
    }
    printf("\n");
    return;
}

int
rr_print(ns_msg* ns_handle, int field, int count, int format)
{

    ns_rr rr;
    memset(&rr, 0, sizeof(rr));
    char buffer[BUFSIZ];
    memset(buffer, 0, sizeof(buffer));

    ns_parserr(ns_handle, (ns_sect)field, count, &rr);

    if (format == 0) {
        printf("    NAME :%s\n", ns_rr_name(rr));
        printf("    TYPE :%x\n", ns_rr_type(rr));
        printf("    CLASS:%x\n", ns_rr_class(rr));
        if (field == ns_s_qd) return 0; 
        printf("    TTL  :%d\n", ns_rr_ttl(rr));
        memcpy(&buffer, ns_rr_rdata(rr), sizeof(buffer));
        if (1 == ns_rr_type(rr)) {
            memset(buffer, 0, sizeof(buffer));
            memcpy(&buffer, ns_rr_rdata(rr), sizeof(buffer));
            struct in_addr* addr = (struct in_addr*)&buffer;
            printf("    A    :%s\n", inet_ntoa(*addr));
        } else {
            memdump_format0((void*)ns_rr_rdata(rr) ,ns_rr_rdlen(rr));
        }
    } else if (format == 1) {
        printf("| %-31s |\n", ns_rr_name(rr));
        printf("+----------------+----------------+\n");
        printf("| TYPE:%9d | CLASS:%8d |\n", ns_rr_type(rr), ns_rr_class(rr));
        if (field == ns_s_qd) return 0; 
        printf("+----------------+----------------+\n");
        printf("| TTL:%27d |\n", ns_rr_ttl(rr));
        printf("+----------------+----------------+\n");
        printf("| RRLEN:%8d |                |\n", ns_rr_rdlen(rr));
        printf("+----------------+                +\n");
        if (1 == ns_rr_type(rr)) {
            memset(buffer, 0, sizeof(buffer));
            memcpy(&buffer, ns_rr_rdata(rr), sizeof(buffer));
            struct in_addr* addr = (struct in_addr*)&buffer;
            printf("| %-31s |\n", inet_ntoa(*addr));
        } else {
            memdump_format1((void*)ns_rr_rdata(rr) ,ns_rr_rdlen(rr));
        }
    } else {
        return 1;
    } 
    return 0;
}

char* bin8(unsigned char bin)
{
    static char result[9];
    memset(result, 0, sizeof(result));

    int i;
    for(i=0; i<8; i++){
        if(bin % 2 == 0) {
            result[7-i] = '0';
        } else {
            result[7-i] = '1';
        }
        bin = bin / 2;
    }
    return result;
}

int
ns_print(ns_msg* ns_handle, int format)
{

    /*
    // flags
    ns_f_qr Question/Response
    ns_f_opcode Operation code
    ns_f_aa Authoritative Answer
    ns_f_tc Truncation occurred
    ns_f_rd Recursion Desired
    ns_f_ra Recursion Available
    ns_f_z  MBZ
    ns_f_ad Authentic Data (DNSSEC)
    ns_f_cd Checking Disabled (DNSSEC)
    ns_f_rcode Response code
    ns_f_max

    // field
    ns_s_qd Query: Question.
    ns_s_zn Update: Zone.
    ns_s_an Query: Answer.
    ns_s_pr Update: Prerequisites.
    ns_s_ns Query: Name servers.
    ns_s_ud Update: Update.
    ns_s_ar Query|Update: Additional records.
    */
    int query_count;
    query_count = ns_msg_count(*ns_handle, ns_s_qd);
    int answer_count;
    answer_count = ns_msg_count(*ns_handle, ns_s_an);
    int authority_count;
    authority_count = ns_msg_count(*ns_handle, ns_s_ns);
    int additional_count;
    additional_count = ns_msg_count(*ns_handle, ns_s_ar);

    if (format == 0) {
        printf("id:%d\n", ns_msg_id(*ns_handle));
        printf("QR:%x\n", ns_msg_getflag(*ns_handle, ns_f_qr));
        printf("OP:%x\n", ns_msg_getflag(*ns_handle, ns_f_opcode));
        printf("AA:%x\n", ns_msg_getflag(*ns_handle, ns_f_aa));
        printf("TC:%x\n", ns_msg_getflag(*ns_handle, ns_f_tc));
        printf("RD:%x\n", ns_msg_getflag(*ns_handle, ns_f_rd));
        printf("RA:%x\n", ns_msg_getflag(*ns_handle, ns_f_ra));
        printf("Z :%x\n", ns_msg_getflag(*ns_handle, ns_f_z));
        printf("AD:%x\n", ns_msg_getflag(*ns_handle, ns_f_ad));
        printf("CD:%x\n", ns_msg_getflag(*ns_handle, ns_f_cd));
        printf("RC:%x\n", ns_msg_getflag(*ns_handle, ns_f_rcode));
        printf("QUERY_COUNT     :%d\n", query_count);
        printf("ANSWER_COUNT    :%d\n", answer_count);
        printf("AUTHORITY_COUNT :%d\n", authority_count);
        printf("ADDITIONAL_COUNT:%d\n", additional_count);
        int i;
        for (i=0; i<query_count; i++) {
            printf("QUERY%d ----\n", i+1);
            rr_print(ns_handle, ns_s_qd, i, format);
            printf("----\n");
        }
        for (i=0; i<answer_count; i++) {
            printf("ANSQER%d ----\n", i+1);
            rr_print(ns_handle, ns_s_an, i, format);
            printf("----\n");
        }
        for (i=0; i<authority_count; i++) {
            printf("AUTHORITY%d ----\n", i+1);
            rr_print(ns_handle, ns_s_ns, i, format);
            printf("----\n");
        }
        for (i=0; i<additional_count; i++) {
            printf("ADDITIONAL%d ----\n", i+1);
            rr_print(ns_handle, ns_s_ar, i, format);
            printf("----\n");
        }
        /*
        */
    } else if (format == 1) {
        char upper[9];
        char lower[9];
        memset(upper, 0, sizeof(upper));
        memset(lower, 0, sizeof(lower));
        sprintf(upper, "%s", bin8(((ns_handle->_flags)>>8)&0x00FF)); 
        sprintf(lower, "%s", bin8((ns_handle->_flags)&0x00FF)); 

        printf("0                16               31\n");
        printf("+----------------+----------------+\n");
        printf("| id:%11d |%s%s|\n", ns_msg_id(*ns_handle), upper, lower);
        printf("+----------------+----------------+\n");
        printf("| QUE_COUNT:%4d | ANS_COUNT:%4d |\n", query_count, answer_count);
        printf("+----------------+----------------+\n");
        printf("| NAME_COUNT:%3d | ADD_COUNT:%4d |\n", authority_count, additional_count);
        printf("+----------------+----------------+\n");
        int i;
        for (i=0; i<query_count; i++) {
            rr_print(ns_handle, ns_s_qd, i, format);
            printf("+----------------+----------------+\n");
        }
        for (i=0; i<answer_count; i++) {
            rr_print(ns_handle, ns_s_an, i, format);
            printf("+----------------+----------------+\n");
        }
        for (i=0; i<authority_count; i++) {
            rr_print(ns_handle, ns_s_ns, i, format);
            printf("+----------------+----------------+\n");
        }
        for (i=0; i<additional_count; i++) {
            rr_print(ns_handle, ns_s_ar, i, format);
            printf("+----------------+----------------+\n");
        }
    } else {
        return 1;
    }

    return 0;
}

void parse_dns(unsigned char *payload, int length)
{
    ns_msg ns_handle;
    memset(&ns_handle, 0, sizeof(ns_handle));

    ns_initparse(payload, length, &ns_handle);
    //ns_print(&ns_handle, 0);
    ns_print(&ns_handle, 1);
    return;
}

std::string read_line(int sock)
{
    std::string line;
    int len;

    for (;;) {
        char c;

        // read関数何回も呼びすぎ・・・
        len = read(sock, &c, 1);
        if (len <= 0) {
            perror("couldn't read");
            exit(1);
        }

        if (c == '\n') {
            return line;
        } else {
            line += c;
        }
    }
}

void split(std::vector<std::string> &res, const std::string &str, char delim){
    size_t current = 0, found;

    while((found = str.find_first_of(delim, current)) != std::string::npos){
        res.push_back(std::string(str, current, found - current));
        current = found + 1;
    }

    res.push_back(std::string(str, current, str.size() - current));
}

void parse_header(std::map<std::string, std::string> &res,
                  const std::string &line)
{
    std::vector<std::string> sp;

    split(sp, line, ',');

    for (auto it = sp.begin(); it != sp.end(); ++it) {
        std::vector<std::string> kv;

        split(kv, *it, '=');

        if (kv.size() != 2) {
            std::cerr << "invalid header" << std::endl;
            exit(1);
        }

        res[kv[0]] = kv[1];
    }
}

int main(int argc, char *argv[])
{
    char *uxpath = (char*)"/tmp/stap/udp/dns";

    if (argc >= 2) {
        uxpath = argv[1];
    }

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("socket");
        exit(1);
    }

    struct sockaddr_un sa = {0};
    sa.sun_family = AF_UNIX;
    strcpy(sa.sun_path, uxpath);

    if (connect(sock, (struct sockaddr*) &sa, sizeof(struct sockaddr_un)) == -1)
    {
        perror("connect");
        exit(1);
    }

    for (;;) {
        std::map<std::string, std::string> header;

        std::string line = read_line(sock);
        parse_header(header, line);

        std::cout << line << std::endl;

        auto it = header.find("len");
        if (it == header.end()) {
            std::cerr << "no length field in header" << std::endl;
            exit(1);
        }

        unsigned char buf[65536];
        int  len = read(sock, buf, atoi(header["len"].c_str()));
        if (len <= 0) {
            perror("couldn't read");
            exit(1);
        }

        parse_dns(buf, len);
    }
}
