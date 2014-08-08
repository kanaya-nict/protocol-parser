#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>

#include <nameser.h>
#include <resolv.h>

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


const unsigned char sample_dns_que[] = { 0xdf, 0x37, 0x01, 0x00,
                                         0x00, 0x01, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x00,
                                         0x03, 0x77, 0x77, 0x77,
                                         0x05, 0x6a, 0x61, 0x69,
                                         0x73, 0x74, 0x02, 0x61,
                                         0x63, 0x02, 0x6a, 0x70,
                                         0x00, 0x00, 0x01, 0x00,
                                         0x01};
const unsigned char sample_dns_res[] = { 0xdf, 0x37, 0x81, 0x80, 
                                         0x00, 0x01, 0x00, 0x01,
                                         0x00, 0x02, 0x00, 0x02,
                                         0x03, 0x77, 0x77, 0x77,
                                         0x05, 0x6a, 0x61, 0x69,
                                         0x73, 0x74, 0x02, 0x61,
                                         0x63, 0x02, 0x6a, 0x70,
                                         0x00, 0x00, 0x01, 0x00,
                                         0x01, 0xc0, 0x0c, 0x00,
                                         0x01, 0x00, 0x01, 0x00,
                                         0x00, 0x0d, 0xf4, 0x00,
                                         0x04, 0x96, 0x41, 0x05,
                                         0xd0, 0xc0, 0x10, 0x00,
                                         0x02, 0x00, 0x01, 0x00,
                                         0x00, 0x14, 0x1e, 0x00,
                                         0x05, 0x02, 0x6e, 0x73,
                                         0xc0, 0x10, 0xc0, 0x10,
                                         0x00, 0x02, 0x00, 0x01,
                                         0x00, 0x00, 0x14, 0x1e,
                                         0x00, 0x06, 0x03, 0x6e,
                                         0x73, 0x33, 0xc0, 0x10,
                                         0xc0, 0x3d, 0x00, 0x01,
                                         0x00, 0x01, 0x00, 0x00,
                                         0x4c, 0x56, 0x00, 0x04,
                                         0x96, 0x41, 0x01, 0x01,
                                         0xc0, 0x4e, 0x00, 0x01,
                                         0x00, 0x01, 0x00, 0x00,
                                         0x4c, 0x56, 0x00, 0x04,
                                         0x7d, 0xce, 0xf5, 0x32};

int main(int argc, char** argv)
{
    res_init();

    ns_msg ns_handle;
    memset(&ns_handle, 0, sizeof(ns_handle));

    //ns_initparse(sample_dns_que,  sizeof(sample_dns_que), &ns_handle);
    ns_initparse(sample_dns_res,  sizeof(sample_dns_res), &ns_handle);

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

    int type;
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

    return 0;
}

