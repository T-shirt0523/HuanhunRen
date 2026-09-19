#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>
#include <cstdio>
#include "net.hpp"

// ============================================================
// 联机传输层实现：UDP 非阻塞收发 + 小端定长序列化
// ============================================================

static bool gWsaOn = false;

bool Net::Init() {
    if (gWsaOn) return true;
    WSADATA wd;
    if (WSAStartup(MAKEWORD(2, 2), &wd) != 0) return false;
    gWsaOn = true;
    return true;
}

void Net::Shutdown() {
    if (gWsaOn) { WSACleanup(); gWsaOn = false; }
}

unsigned short Net::UdpOpen(unsigned short port) {
    if (!Init()) return (unsigned short)-1;
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) return (unsigned short)-1;
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(s, (sockaddr*)&addr, sizeof(addr)) != 0) { closesocket(s); return (unsigned short)-1; }
    u_long nb = 1;
    ioctlsocket(s, FIONBIO, &nb);
    return (unsigned short)s;
}

void Net::UdpClose(unsigned short s) {
    if (s != (unsigned short)-1) closesocket((SOCKET)s);
}

unsigned Net::ResolveIp(const char* ip) {
    if (!Init()) return 0;
    unsigned a = inet_addr(ip);          // 已是数字点分格式
    if (a != INADDR_NONE) return a;
    hostent* h = gethostbyname(ip);      // 兼容主机名
    if (!h || !h->h_addr_list || !h->h_addr_list[0]) return 0;
    return *(unsigned*)h->h_addr_list[0];
}

char* Net::LocalIp() {
    static char buf[16] = "127.0.0.1";
    if (!Init()) return buf;
    // 经典做法：向公共地址发一个 UDP（不产生实际流量），读本地出口 IP
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s != INVALID_SOCKET) {
        sockaddr_in dst;
        std::memset(&dst, 0, sizeof(dst));
        dst.sin_family = AF_INET;
        dst.sin_addr.s_addr = inet_addr("8.8.8.8");
        dst.sin_port = htons(53);
        if (connect(s, (sockaddr*)&dst, sizeof(dst)) == 0) {
            sockaddr_in local;
            int len = sizeof(local);
            if (getsockname(s, (sockaddr*)&local, &len) == 0) {
                unsigned char* b = (unsigned char*)&local.sin_addr.s_addr;
                snprintf(buf, sizeof(buf), "%u.%u.%u.%u", b[0], b[1], b[2], b[3]);
            }
        }
        closesocket(s);
    }
    return buf;
}

void Net::W8(Buf& b, unsigned v) { if (b.n < (int)sizeof(b.d)) b.d[b.n++] = (unsigned char)(v & 0xFF); }
void Net::W16(Buf& b, unsigned v) { W8(b, v); W8(b, v >> 8); }
void Net::W32(Buf& b, unsigned v) { W16(b, v); W16(b, v >> 16); }
void Net::WFl(Buf& b, float f) { unsigned u; std::memcpy(&u, &f, 4); W32(b, u); }
void Net::WStr(Buf& b, const char* s, int maxLen) {
    int len = (int)std::strlen(s);
    if (len > maxLen - 1) len = maxLen - 1;
    if (len > 255) len = 255;
    W8(b, (unsigned)len);
    for (int i = 0; i < len; i++) W8(b, (unsigned char)s[i]);
}

Net::Rd Net::BeginRead(const unsigned char* d, int n) { return { d, d + n, n > 0 }; }

unsigned Net::R8(Rd& r) {
    if (!r.ok || r.p >= r.end) { r.ok = false; return 0; }
    return *r.p++;
}
unsigned Net::R16(Rd& r) { unsigned lo = R8(r); return lo | (R8(r) << 8); }
unsigned Net::R32(Rd& r) { unsigned lo = R16(r); return lo | (R16(r) << 16); }
float Net::RFl(Rd& r) { unsigned u = R32(r); float f; std::memcpy(&f, &u, 4); return f; }
void Net::RStr(Rd& r, char* out, int maxLen) {
    unsigned len = R8(r);
    if (!r.ok) { out[0] = 0; return; }
    if (len > (unsigned)maxLen - 1) len = (unsigned)maxLen - 1;
    for (unsigned i = 0; i < len; i++) out[i] = (char)R8(r);
    out[len] = 0;
    if (!r.ok) out[0] = 0;
}

void Net::UdpSend(unsigned short s, unsigned addrU, unsigned short portU, const Buf& b) {
    sockaddr_in to;
    std::memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = addrU;
    to.sin_port = htons(portU);
    sendto((SOCKET)s, (const char*)b.d, b.n, 0, (sockaddr*)&to, sizeof(to));
}

bool Net::UdpRecv(unsigned short s, unsigned& addrU, unsigned short& portU, Buf& b) {
    sockaddr_in from;
    int flen = sizeof(from);
    int got = recvfrom((SOCKET)s, (char*)b.d, (int)sizeof(b.d), 0, (sockaddr*)&from, &flen);
    if (got <= 0) return false;
    b.n = got;
    addrU = from.sin_addr.s_addr;
    portU = ntohs(from.sin_port);
    return true;
}
