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

#define A      1
#define NS     2
#define MD     3
#define MF     4
#define CNAME  5
#define SOA    6
#define MB     7
#define MG     8
#define MR     9
#define NULL_TYPE 10
#define WKS   11
#define PTR   12
#define HINFO 13
#define MINFO 14
#define MX    15
#define TXT   16
#define RP    17
#define AFSDB 18
#define X25   19
#define ISDN  20
#define RT    21
#define NSAP  22
#define NSAP_PTR 23
#define SIG   24
#define KEY   25
#define PX    26
#define GPOS  27
#define AAAA  28
#define LOC   29
#define NXT   30
#define EID   31
#define NIMLOC 32
#define SRV   33
#define ATMA  34
#define NAPTR 35
#define KX    36
#define CERT  37
#define A6    38
#define DNAME 39
#define SINK  40
#define OPT   41
#define APL   42
#define DS    43
#define SSHFP 44
#define IPSECKEY 45
#define RRSIG 46
#define NSEC  47
#define DNSKEY 48
#define DHCID 49
#define NSEC3 50
#define NSEC3PARAM 51
#define TLSA  52
#define HIP   55
#define NINFO 56
#define RKEY  57
#define TALINK 58
#define CDS   59
#define CDNSKEY 60
#define OPENPGPKEY 61
#define CSYNC 62
#define SPF   99
#define UINFO 100
#define UID   101
#define GID   102
#define UNSPEC 103
#define NID   104
#define L32   105
#define L64   106
#define LP    107
#define EUI48 108
#define EUI64 109
#define TKEY  249
#define TSIG  250
#define IXFR  251
#define MAILB 253
#define MAILA 254
#define ANY   255
#define TA    32768
#define DLV   32769

int ns_format = 2;

std::map<uint16_t, std::string> rr_type;

void init_rr_type()
{
    rr_type[A]     = "A";
    rr_type[NS]    = "NS";
    rr_type[CNAME] = "CNAME";
    rr_type[SOA]   = "SOA";
    rr_type[PTR]   = "PTR";
    rr_type[MX]    = "MX";
    rr_type[AAAA]  = "AAAA";
    rr_type[A]     = "A";
    rr_type[NS]    = "NS";
    rr_type[MD]    = "MD";
    rr_type[MF]    = "MF";
    rr_type[CNAME] = "CNAME";
    rr_type[SOA]   = "SOA";
    rr_type[MB]    = "MB";
    rr_type[MG]    = "MG";
    rr_type[MR]    = "MR";
    rr_type[NULL_TYPE] = "NULL";
    rr_type[WKS]   = "WKS";
    rr_type[PTR]   = "PTR";
    rr_type[HINFO] = "HINFO";
    rr_type[MINFO] = "MINFO";
    rr_type[MX]    = "MX";
    rr_type[TXT]   = "TXT";
    rr_type[RP]    = "RP";
    rr_type[AFSDB] = "AFSDB";
    rr_type[X25]   = "X25";
    rr_type[ISDN]  = "ISDN";
    rr_type[RT]    = "RT";
    rr_type[NSAP]  = "NSAP";
    rr_type[NSAP_PTR] = "NSAP_PTR";
    rr_type[SIG]   = "SIG";
    rr_type[KEY]   = "KEY";
    rr_type[PX]    = "PX";
    rr_type[GPOS]  = "GPOS";
    rr_type[AAAA]  = "AAAA";
    rr_type[LOC]   = "LOC";
    rr_type[NXT]   = "NXT";
    rr_type[EID]   = "EID";
    rr_type[NIMLOC] = "NIMLOC";
    rr_type[SRV]   = "SRV";
    rr_type[ATMA]  = "ATMA";
    rr_type[NAPTR] = "NAPTR";
    rr_type[KX]    = "KX";
    rr_type[CERT]  = "CERT";
    rr_type[A6]    = "A6";
    rr_type[DNAME] = "DNAME";
    rr_type[SINK]  = "SINK";
    rr_type[OPT]   = "OPT";
    rr_type[APL]   = "APL";
    rr_type[DS]    = "DS";
    rr_type[SSHFP] = "SSHFP";
    rr_type[IPSECKEY] = "IPSECKEY";
    rr_type[RRSIG] = "RRSIG";
    rr_type[NSEC]  = "NSEC";
    rr_type[DNSKEY] = "DNSKEY";
    rr_type[DHCID] = "DHCID";
    rr_type[NSEC3] = "NSEC3";
    rr_type[NSEC3PARAM] = "NSEC3PARAM";
    rr_type[TLSA]  = "TLSA";
    rr_type[HIP]   = "HIP";
    rr_type[NINFO] = "HINFO";
    rr_type[RKEY]  = "RKEY";
    rr_type[TALINK] = "TALINK";
    rr_type[CDS]   = "CDS";
    rr_type[CDNSKEY] = "CDNSKEY";
    rr_type[OPENPGPKEY] = "OPENPGPKEY";
    rr_type[CSYNC] = "CSYNC";
    rr_type[SPF]   = "SPF";
    rr_type[UINFO] = "UINFO";
    rr_type[UID]   = "UID";
    rr_type[GID]   = "GID";
    rr_type[UNSPEC] = "UNSPEC";
    rr_type[NID]   = "NID";
    rr_type[L32]   = "L32";
    rr_type[L64]   = "L64";
    rr_type[LP]    = "LP";
    rr_type[EUI48] = "EUI48";
    rr_type[EUI64] = "EUI64";
    rr_type[TKEY]  = "TKEY";
    rr_type[TSIG]  = "TSIG";
    rr_type[IXFR]  = "IXFR";
    rr_type[MAILB] = "MAILB";
    rr_type[MAILA] = "MAILA";
    rr_type[ANY]   = "ANY";
    rr_type[TA]    = "TA";
    rr_type[DLV]   = "DLV";
}


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

void memdump_format2(char* buffer, int length)
{
    for (int i = 0; i < length; i++) {
        printf("%02x", (int)buffer[i] & 0xff);
    }
}


int
rr_print(ns_msg* ns_handle, int field, int count, int format)
{
    char dname[NS_MAXDNAME];
    ns_rr rr;
    memset(&rr, 0, sizeof(rr));
    char buffer[BUFSIZ];
    memset(buffer, 0, sizeof(buffer));

    if (ns_parserr(ns_handle, (ns_sect)field, count, &rr)) {
        fprintf(stderr, "ns_parserr: %s\n", strerror(errno));
        if (format == 2) {
            printf("{}");
        }
        return 0;
    }

#define NS_UNCOMPRESS(RR) ns_name_uncompress(   \
        ns_msg_base(*ns_handle),                \
        ns_msg_end(*ns_handle),                 \
        RR,                                     \
        dname,                                  \
        sizeof(dname))

    if (format == 1) {
        printf("| %-31s |\n", ns_rr_name(rr));
        printf("+----------------+----------------+\n");
        printf("| TYPE:%9d | CLASS:%8d |\n", ns_rr_type(rr), ns_rr_class(rr));
        if (field == ns_s_qd) return 0;
        printf("+----------------+----------------+\n");
        printf("| TTL:%27d |\n", ns_rr_ttl(rr));
        printf("+----------------+----------------+\n");
        printf("| RRLEN:%8d |                |\n", ns_rr_rdlen(rr));
        printf("+----------------+                +\n");
        if (A == ns_rr_type(rr)) { // A record
            memset(buffer, 0, sizeof(buffer));
            memcpy(&buffer, ns_rr_rdata(rr), ns_rr_rdlen(rr));
            struct in_addr* addr = (struct in_addr*)&buffer;
            printf("| %-31s |\n", inet_ntoa(*addr));
        } else {
            memdump_format1((void*)ns_rr_rdata(rr) ,ns_rr_rdlen(rr));
        }
    } else if (format == 2) {
        if (rr_type.find(ns_rr_type(rr)) != rr_type.end()) {
            printf("{\"name\":\"%s\",\"type\":\"%s\",\"class\":%d",
                   ns_rr_name(rr), rr_type[ns_rr_type(rr)].c_str(), ns_rr_class(rr));
        } else {
            printf("{\"name\":\"%s\",\"type\":\"%u\",\"class\":%d",
                   ns_rr_name(rr), ns_rr_type(rr), ns_rr_class(rr));
        }

        if (field == ns_s_qd) {
            printf("}");
            return 0;
        }

        printf(",\"ttl\":%d,", ns_rr_ttl(rr));

        memcpy(&buffer, ns_rr_rdata(rr), sizeof(buffer));

        switch (ns_rr_type(rr)) {
        case A:
        {
            memset(buffer, 0, sizeof(buffer));
            memcpy(&buffer, ns_rr_rdata(rr), ns_rr_rdlen(rr));
            struct in_addr* addr = (struct in_addr*)&buffer;
            printf("\"a\":\"%s\"}", inet_ntoa(*addr));
            break;
        }
        case NS:
        {
            if (NS_UNCOMPRESS(ns_rr_rdata(rr)) < 0) {
                printf("}");
            } else {
                printf("\"ns\":\"%s\"}", dname);
            }
            break;
        }
        case CNAME:
        {
            if (NS_UNCOMPRESS(ns_rr_rdata(rr)) < 0) {
                printf("}");
            } else {
                printf("\"cname\":\"%s\"}", dname);
            }
            break;
        }
        case PTR:
        {
            if (NS_UNCOMPRESS(ns_rr_rdata(rr)) < 0) {
                printf("}");
            } else {
                printf("\"ptr\":\"%s\"}", dname);
            }
            break;
        }
        case MX:
        {
            uint16_t preference = *(uint16_t *)ns_rr_rdata(rr);
            printf("\"preference\":\"%u\"", ntohs(preference));
            if (NS_UNCOMPRESS(ns_rr_rdata(rr) + 2) < 0) {
                printf("}");
            } else {
                printf(",\"exchange\":\"%s\"}", dname);
            }
            break;
        }
        case AAAA:
            memset(buffer, 0, sizeof(buffer));
            memcpy(&buffer, ns_rr_rdata(rr), ns_rr_rdlen(rr));

            char addr[INET6_ADDRSTRLEN];
            if (inet_ntop(AF_INET6, buffer, addr, sizeof(addr)) == NULL) {
                printf("}");
            } else {
                printf("\"aaaa\":\"%s\"}", addr);
            }
            break;
        case SOA:
            const unsigned char *cp1, *cp2;

            cp1 = cp2 = ns_rr_rdata(rr);

            if (ns_name_skip(&cp2, ns_msg_end(*ns_handle)) < 0) {
                printf("}");
                break;
            }

            NS_UNCOMPRESS(cp1);
            printf("\"mname\":\"%s\"", dname);

            if (NS_UNCOMPRESS(cp2) < 0) {
                printf("}");
            } else {
                printf(",\"rname\":\"%s\"", dname);
            }

            ns_name_skip(&cp2, ns_msg_end(*ns_handle));

            struct {
                uint32_t serial;
                uint32_t refresh;
                uint32_t retry;
                uint32_t expire;
                uint32_t minumun;
            } soa_rdata;

            if (cp2 + sizeof(soa_rdata) > ns_msg_end(*ns_handle)) {
                printf("}");
            } else {
                memcpy(&soa_rdata, cp2, sizeof(soa_rdata));
                printf(",\"serial\":%u,\"refresh\":%u,\"retry\":%u,\"expire\":%u,\"minumum\":%u}", ntohl(soa_rdata.serial), ntohl(soa_rdata.refresh), ntohl(soa_rdata.retry), ntohl(soa_rdata.expire), ntohl(soa_rdata.minumun));
            }
            break;
        default:
            printf("\"rr\":\"");
            memdump_format2((char*)ns_rr_rdata(rr), ns_rr_rdlen(rr));
            printf("\"}");
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
ns_print(ns_msg* ns_handle, int format,
         std::map<std::string, std::string> &header)
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

    if (format == 1) {
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
            if (rr_print(ns_handle, ns_s_qd, i, format) == 0)
                break;
            printf("+----------------+----------------+\n");
        }
        for (i=0; i<answer_count; i++) {
            if (rr_print(ns_handle, ns_s_an, i, format) == 0)
                break;
            printf("+----------------+----------------+\n");
        }
        for (i=0; i<authority_count; i++) {
            if (rr_print(ns_handle, ns_s_ns, i, format) == 0)
                break;
            printf("+----------------+----------------+\n");
        }
        for (i=0; i<additional_count; i++) {
            if (rr_print(ns_handle, ns_s_ar, i, format) == 0)
                break;
            printf("+----------------+----------------+\n");
        }
    } else if (format == 2) {
        printf("{");

        printf("\"src\":{");
        if (header["from"] == "1") {
            printf("\"ip\":\"%s\",", header["ip1"].c_str());
            printf("\"port\":%d", atoi(header["port1"].c_str()));
        } else {
            printf("\"ip\":\"%s\",", header["ip2"].c_str());
            printf("\"port\":%d", atoi(header["port2"].c_str()));
        }
        printf("},");

        printf("\"dst\":{");
        if (header["from"] == "1") {
            printf("\"ip\":\"%s\",", header["ip2"].c_str());
            printf("\"port\":%d", atoi(header["port2"].c_str()));
        } else {
            printf("\"ip\":\"%s\",", header["ip1"].c_str());
            printf("\"port\":%d", atoi(header["port1"].c_str()));
        }
        printf("},");

        printf("\"id\":%d,", ns_msg_id(*ns_handle));
        printf("\"qr\":%d,", ns_msg_getflag(*ns_handle, ns_f_qr));
        printf("\"op\":%d,", ns_msg_getflag(*ns_handle, ns_f_opcode));
        printf("\"aa\":%d,", ns_msg_getflag(*ns_handle, ns_f_aa));
        printf("\"tc\":%d,", ns_msg_getflag(*ns_handle, ns_f_tc));
        printf("\"rd\":%d,", ns_msg_getflag(*ns_handle, ns_f_rd));
        printf("\"ra\":%d,", ns_msg_getflag(*ns_handle, ns_f_ra));
        printf("\"z\":%d,", ns_msg_getflag(*ns_handle, ns_f_z));
        printf("\"ad\":%d,", ns_msg_getflag(*ns_handle, ns_f_ad));
        printf("\"cd\":%d,", ns_msg_getflag(*ns_handle, ns_f_cd));
        printf("\"rc\":%d,", ns_msg_getflag(*ns_handle, ns_f_rcode));
        printf("\"query_count\":%d,", query_count);
        printf("\"answer_count\":%d,", answer_count);
        printf("\"authority_count\":%d,", authority_count);
        printf("\"additional_count\":%d,", additional_count);
        int i;

        printf("\"query\":[");
        i = 0;
        for (i = 0; i < query_count; i++) {
            if (rr_print(ns_handle, ns_s_qd, i, format) == 0)
                break;
            if (i + 1 < query_count)
                printf(",");
        }
        printf("],");

        printf("\"answer\":[");
        for (i = 0; i < answer_count; i++) {
            if (rr_print(ns_handle, ns_s_an, i, format) == 0)
                break;
            if (i + 1 < answer_count)
                printf(",");
        }
        printf("],");

        printf("\"authority\":[");
        for (i = 0; i < authority_count; i++) {
            if (rr_print(ns_handle, ns_s_ns, i, format) == 0)
                break;
            if (i + 1 < authority_count)
                printf(",");
        }
        printf("],");

        printf("\"additional\":[");
        for (i = 0; i < additional_count; i++) {
            if (rr_print(ns_handle, ns_s_ar, i, format) == 0)
                break;
            if (i + 1 < additional_count)
                printf(",");
            }
        printf("]}\n");
    } else {
        return 1;
    }

    return 0;
}

void parse_dns(unsigned char *payload, int length,
               std::map<std::string, std::string> &header)
{
    ns_msg ns_handle;
    memset(&ns_handle, 0, sizeof(ns_handle));

    ns_initparse(payload, length, &ns_handle);
    ns_print(&ns_handle, ns_format, header);

    return;
}

std::string read_line(int sock)
{
    std::string line;
    int len;

    for (;;) {
        char c;

        // read function is called too many
        len = read(sock, &c, 1);
        if (len == 0) {
            fprintf(stderr, "remote socket was closed");
            exit(0);
        } else if (len <= 0) {
            perror("couldn't read 1");
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
    int   result;
    std::string uxpath = (char*)"/tmp/sf-tap/udp/dns";

    while((result = getopt(argc, argv, "u:pjh"))!=-1){
        switch (result) {
        case 'p':
            ns_format = 1;
            break;
        case 'j':
            ns_format = 2;
            break;
        case 'u':
            uxpath = optarg;
            break;
        case 'h':
        default:
            std::cout << argv[0] << " -p -u unix_domain_path\n"
                      << "  -j: print data as JSON format (default)\n"
                      << "  -p: print data as packet format\n"
                      << "  -u: specify a path of unix domain socket"
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

    init_rr_type();

    for (;;) {
        std::map<std::string, std::string> header;

        std::string line = read_line(sock);
        parse_header(header, line);

        if (ns_format != 2)
            std::cout << line << std::endl;

        auto it = header.find("len");
        if (it == header.end()) {
            std::cerr << "no length field in header" << std::endl;
            exit(1);
        }

        unsigned char buf[65536];
        int readlen = atoi(header["len"].c_str());
        int len;
        if (readlen > 0) {
            len = read(sock, buf, readlen);
            if (len <= 0) {
                std::cerr << header["len"] << std::endl;
                perror("couldn't read 2");
                exit(1);
            }
            parse_dns(buf, len, header);
            fflush(stdout);
        }
    }
}
