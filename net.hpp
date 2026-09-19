#pragma once
// ============================================================
// 联机传输层（局域网 UDP · Winsock）
// 职责仅限：UDP 收发 + 定长包读写。所有游戏语义在 main.cpp。
// 设计：房主权威模拟；客人本地预测移动，生物/掉落/投射物走快照插值。
// ============================================================

namespace Net {

constexpr unsigned short PORT = 47991;   // 固定端口：同局域网直连，无需服务器

bool Init();       // WSAStartup（进程级幂等）
void Shutdown();

// 打开非阻塞 UDP 套接字并绑定端口（port=0 由系统分配）；失败返回 (unsigned short)-1
unsigned short UdpOpen(unsigned short port);
void           UdpClose(unsigned short s);
unsigned       ResolveIp(const char* ip);   // "192.168.1.3" -> 网络字节序地址（失败 0）
char*          LocalIp();                   // 本机局域网 IP（"192.168.x.x"；失败返回 "127.0.0.1"）

// ---- 包缓冲与定长读写（小端，手写序列化，不跨平台传输结构体）----
struct Buf { unsigned char d[8192]; int n = 0; };
void W8(Buf& b, unsigned v);
void W16(Buf& b, unsigned v);
void W32(Buf& b, unsigned v);
void WFl(Buf& b, float f);
void WStr(Buf& b, const char* s, int maxLen);   // 1 字节长度前缀 + UTF-8

struct Rd { const unsigned char* p; const unsigned char* end; bool ok; };
Rd BeginRead(const unsigned char* d, int n);
unsigned R8(Rd& r);
unsigned R16(Rd& r);
unsigned R32(Rd& r);
float    RFl(Rd& r);
void     RStr(Rd& r, char* out, int maxLen);

void UdpSend(unsigned short s, unsigned addrU, unsigned short portU, const Buf& b);
bool UdpRecv(unsigned short s, unsigned& addrU, unsigned short& portU, Buf& b);   // false = 无包

} // namespace Net
