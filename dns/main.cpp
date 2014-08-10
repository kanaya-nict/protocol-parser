
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

void memdump(void* buffer, int length)
{
    uint32_t* addr32 = (uint32_t*)buffer;
    int i;
    int j;
    int k;
    int lines = length/16 + (length%16?1:0);
    printf("	rdata:	");
    for (i=0; i<lines; i++) {
        printf("%08x %08x %08x %08x\n",
                htonl(*(addr32)),
                htonl(*(addr32+1)),
                htonl(*(addr32+2)),
                htonl(*(addr32+3))
              );
        addr32 += 4;
    }

    j = length%16;
    if (j == 0) return;
    k = 0;
    uint8_t*  addr8 = (uint8_t*)addr32;
    printf("		");
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

void parse_dns(unsigned char *payload, int length)
{
    //res_init();

    ns_msg ns_handle;
    memset(&ns_handle, 0, sizeof(ns_handle));

    //ns_initparse(sample_dns_que,  sizeof(sample_dns_que), &ns_handle);
    //ns_initparse(sample_dns_res,  sizeof(sample_dns_res), &ns_handle);
    ns_initparse(payload, length, &ns_handle);

    printf("id:%d\n", ns_msg_id(ns_handle));
    /*
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
    */
    printf("QR:%x\n", ns_msg_getflag(ns_handle, ns_f_qr));
    printf("OP:%x\n", ns_msg_getflag(ns_handle, ns_f_opcode));
    printf("AA:%x\n", ns_msg_getflag(ns_handle, ns_f_aa));
    printf("TC:%x\n", ns_msg_getflag(ns_handle, ns_f_tc));
    printf("RD:%x\n", ns_msg_getflag(ns_handle, ns_f_rd));
    printf("RA:%x\n", ns_msg_getflag(ns_handle, ns_f_ra));
    printf("Z :%x\n", ns_msg_getflag(ns_handle, ns_f_z));
    printf("AD:%x\n", ns_msg_getflag(ns_handle, ns_f_ad));
    printf("CD:%x\n", ns_msg_getflag(ns_handle, ns_f_cd));
    printf("RC:%x\n", ns_msg_getflag(ns_handle, ns_f_rcode));

    /*
    ns_s_qd Query: Question.
    ns_s_zn Update: Zone.
    ns_s_an Query: Answer.
    ns_s_pr Update: Prerequisites.
    ns_s_ns Query: Name servers.
    ns_s_ud Update: Update.
    ns_s_ar Query|Update: Additional records.
    */
    int query_count;
    query_count = ns_msg_count(ns_handle, ns_s_qd);
    printf("QUERY_COUNT     :%d\n", query_count);

    int answer_count;
    answer_count = ns_msg_count(ns_handle, ns_s_an);
    printf("ANSWER_COUNT    :%d\n", answer_count);

    int authority_count;
    authority_count = ns_msg_count(ns_handle, ns_s_ns);
    printf("AUTHORITY_COUNT :%d\n", authority_count);

    int additional_count;
    additional_count = ns_msg_count(ns_handle, ns_s_ar);
    printf("ADDITIONAL_COUNT:%d\n", additional_count);

    ns_rr rr;
    char buffer[BUFSIZ];
    int i;
    printf("QUERY");
    for (i=0; i<query_count; i++) {
        memset(&rr, 0, sizeof(rr));
        memset(buffer, 0, sizeof(buffer));
        ns_parserr(&ns_handle, ns_s_qd, i, &rr);

        ns_name_uncompress(ns_msg_base(ns_handle),
                           ns_msg_end(ns_handle),
                           ns_rr_rdata(rr),
                           buffer,
                           BUFSIZ);
        printf("	%s\n", buffer);
        printf("	NAME :%s\n", ns_rr_name(rr));
        printf("	TYPE :%x\n", ns_rr_type(rr));
        printf("	CLASS:%x\n", ns_rr_class(rr));
    }

    printf("ANSWER");
    for (i=0; i<answer_count; i++) {
        memset(&rr, 0, sizeof(rr));
        memset(buffer, 0, sizeof(buffer));
        ns_parserr(&ns_handle, ns_s_an, i, &rr);

        ns_name_uncompress(ns_msg_base(ns_handle),
                           ns_msg_end(ns_handle),
                           ns_rr_rdata(rr),
                           buffer,
                           BUFSIZ);
        //printf("	%s\n", buffer);
        printf("	NAME :%s\n", ns_rr_name(rr));
        printf("	TYPE :%x\n", ns_rr_type(rr));
        printf("	CLASS:%x\n", ns_rr_class(rr));
        printf("	TTL  :%d\n", ns_rr_ttl(rr));

        memcpy(&buffer, ns_rr_rdata(rr), sizeof(buffer));
        if (1 == ns_rr_type(rr)) {
            memset(buffer, 0, sizeof(buffer));
            memcpy(&buffer, ns_rr_rdata(rr), sizeof(buffer));
            struct in_addr* addr = (struct in_addr*)&buffer;
            printf("	A    :%s\n", inet_ntoa(*addr));
        } else {
            //memdump((void*)ns_rr_rdata(rr) ,ns_rr_rdlen(rr));
        }
    }

    printf("AUTHORITY\n");
    for (i=0; i<authority_count; i++) {
        memset(&rr, 0, sizeof(rr));
        memset(buffer, 0, sizeof(buffer));
        ns_parserr(&ns_handle, ns_s_ns, i, &rr);

        ns_name_uncompress(ns_msg_base(ns_handle),
                           ns_msg_end(ns_handle),
                           ns_rr_rdata(rr),
                           buffer,
                           BUFSIZ);
        //printf("	%s\n", buffer);
        printf("	NAME :%s\n", ns_rr_name(rr));
        printf("	TYPE :%x\n", ns_rr_type(rr));
        printf("	CLASS:%x\n", ns_rr_class(rr));
        printf("	TTL  :%d\n", ns_rr_ttl(rr));

        memcpy(&buffer, ns_rr_rdata(rr), sizeof(buffer));
        if (1 == ns_rr_type(rr)) {
            memset(buffer, 0, sizeof(buffer));
            memcpy(&buffer, ns_rr_rdata(rr), sizeof(buffer));
            struct in_addr* addr = (struct in_addr*)&buffer;
            printf("	A    :%s\n", inet_ntoa(*addr));
        } else {
            memdump((void*)ns_rr_rdata(rr) ,ns_rr_rdlen(rr));
        }
    }

    printf("ADDITIONAL\n");
    for (i=0; i<additional_count; i++) {
        memset(&rr, 0, sizeof(rr));
        memset(buffer, 0, sizeof(buffer));
        ns_parserr(&ns_handle, ns_s_ar, i, &rr);

        ns_name_uncompress(ns_msg_base(ns_handle),
                           ns_msg_end(ns_handle),
                           ns_rr_rdata(rr),
                           buffer,
                           BUFSIZ);
        //printf("	%s\n", buffer);
        printf("	NAME :%s\n", ns_rr_name(rr));
        printf("	TYPE :%x\n", ns_rr_type(rr));
        printf("	CLASS:%x\n", ns_rr_class(rr));
        printf("	TTL  :%d\n", ns_rr_ttl(rr));

        memcpy(&buffer, ns_rr_rdata(rr), sizeof(buffer));
        if (1 == ns_rr_type(rr)) {
            memset(buffer, 0, sizeof(buffer));
            memcpy(&buffer, ns_rr_rdata(rr), sizeof(buffer));
            struct in_addr* addr = (struct in_addr*)&buffer;
            printf("	A    :%s\n", inet_ntoa(*addr));
        } else {
            //memdump((void*)ns_rr_rdata(rr) ,ns_rr_rdlen(rr));
        }
    }
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
