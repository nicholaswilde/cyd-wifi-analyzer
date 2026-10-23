#include "oui.h"
#include <cstring>
#include <cctype>

// Compact static prefix table sorted in ascending order for O(log N) binary search.
// Stored in PROGMEM (Flash) to preserve SRAM.
static const OuiEntry OUI_TABLE[] PROGMEM = {
    {0x0000F0, "Samsung"}, // 00:00:F0
    {0x000142, "Cisco/Meraki"}, // 00:01:42
    {0x000143, "Cisco/Meraki"}, // 00:01:43
    {0x0001E6, "Aruba/HP"}, // 00:01:E6
    {0x0002B3, "Intel"}, // 00:02:B3
    {0x000347, "Intel"}, // 00:03:47
    {0x000393, "Apple"}, // 00:03:93
    {0x00040E, "AVM"}, // 00:04:0E
    {0x000423, "Intel"}, // 00:04:23
    {0x00045A, "Linksys"}, // 00:04:5A
    {0x000502, "Apple"}, // 00:05:02
    {0x00055D, "D-Link"}, // 00:05:5D
    {0x00059A, "Cisco/Meraki"}, // 00:05:9A
    {0x000625, "Linksys"}, // 00:06:25
    {0x000652, "Cisco/Meraki"}, // 00:06:52
    {0x0007AB, "Samsung"}, // 00:07:AB
    {0x0007E9, "Intel"}, // 00:07:E9
    {0x000802, "Aruba/HP"}, // 00:08:02
    {0x00095B, "Netgear"}, // 00:09:5B
    {0x000A27, "Apple"}, // 00:0A:27
    {0x000A95, "Apple"}, // 00:0A:95
    {0x000AEB, "TP-Link"}, // 00:0A:EB
    {0x000B86, "Aruba/HP"}, // 00:0B:86
    {0x000C41, "Linksys"}, // 00:0C:41
    {0x000C42, "MikroTik"}, // 00:0C:42
    {0x000C6E, "Asus"}, // 00:0C:6E
    {0x000C85, "Cisco/Meraki"}, // 00:0C:85
    {0x000D88, "D-Link"}, // 00:0D:88
    {0x000D93, "Apple"}, // 00:0D:93
    {0x000E0C, "Intel"}, // 00:0E:0C
    {0x000E35, "Intel"}, // 00:0E:35
    {0x000E38, "Linksys"}, // 00:0E:38
    {0x000E7F, "Aruba/HP"}, // 00:0E:7F
    {0x000F3D, "D-Link"}, // 00:0F:3D
    {0x000F66, "Linksys"}, // 00:0F:66
    {0x000FB5, "Netgear"}, // 00:0F:B5
    {0x001007, "Cisco/Meraki"}, // 00:10:07
    {0x0010FA, "Apple"}, // 00:10:FA
    {0x00110A, "Aruba/HP"}, // 00:11:0A
    {0x001132, "Synology"}, // 00:11:32
    {0x001150, "Belkin"}, // 00:11:50
    {0x001195, "D-Link"}, // 00:11:95
    {0x0011D8, "Asus"}, // 00:11:D8
    {0x001217, "Linksys"}, // 00:12:17
    {0x001247, "Samsung"}, // 00:12:47
    {0x001302, "Intel"}, // 00:13:02
    {0x001310, "Linksys"}, // 00:13:10
    {0x001346, "D-Link"}, // 00:13:46
    {0x0013E8, "Intel"}, // 00:13:E8
    {0x001451, "Apple"}, // 00:14:51
    {0x00146C, "Netgear"}, // 00:14:6C
    {0x001478, "TP-Link"}, // 00:14:78
    {0x0014BF, "Linksys"}, // 00:14:BF
    {0x001500, "Intel"}, // 00:15:00
    {0x00150C, "AVM"}, // 00:15:0C
    {0x00156D, "Ubiquiti"}, // 00:15:6D
    {0x0015E9, "D-Link"}, // 00:15:E9
    {0x0015F2, "Asus"}, // 00:15:F2
    {0x00166C, "Samsung"}, // 00:16:6C
    {0x001676, "Intel"}, // 00:16:76
    {0x0016B6, "Linksys"}, // 00:16:B6
    {0x0016CB, "Apple"}, // 00:16:CB
    {0x00173F, "Belkin"}, // 00:17:3F
    {0x00179A, "D-Link"}, // 00:17:9A
    {0x0017A4, "Aruba/HP"}, // 00:17:A4
    {0x0017F2, "Apple"}, // 00:17:F2
    {0x00180A, "Cisco/Meraki"}, // 00:18:0A
    {0x001839, "Linksys"}, // 00:18:39
    {0x00184D, "Netgear"}, // 00:18:4D
    {0x001871, "Aruba/HP"}, // 00:18:71
    {0x001882, "Huawei"}, // 00:18:82
    {0x0018BA, "Cisco/Meraki"}, // 00:18:BA
    {0x00195B, "D-Link"}, // 00:19:5B
    {0x0019E0, "TP-Link"}, // 00:19:E0
    {0x0019E3, "Apple"}, // 00:19:E3
    {0x001A11, "Google"}, // 00:1A:11
    {0x001A1E, "Aruba/HP"}, // 00:1A:1E
    {0x001A4F, "AVM"}, // 00:1A:4F
    {0x001A70, "Linksys"}, // 00:1A:70
    {0x001A92, "Asus"}, // 00:1A:92
    {0x001AA1, "Cisco/Meraki"}, // 00:1A:A1
    {0x001B11, "D-Link"}, // 00:1B:11
    {0x001B21, "Intel"}, // 00:1B:21
    {0x001B2F, "Netgear"}, // 00:1B:2F
    {0x001B63, "Apple"}, // 00:1B:63
    {0x001C0F, "Cisco/Meraki"}, // 00:1C:0F
    {0x001C10, "Linksys"}, // 00:1C:10
    {0x001C4A, "AVM"}, // 00:1C:4A
    {0x001CB3, "Apple"}, // 00:1C:B3
    {0x001CF0, "D-Link"}, // 00:1C:F0
    {0x001D0F, "TP-Link"}, // 00:1D:0F
    {0x001D25, "Samsung"}, // 00:1D:25
    {0x001D2D, "Ruckus"}, // 00:1D:2D
    {0x001D4F, "Apple"}, // 00:1D:4F
    {0x001D7E, "Linksys"}, // 00:1D:7E
    {0x001E10, "Huawei"}, // 00:1E:10
    {0x001E2A, "Netgear"}, // 00:1E:2A
    {0x001E52, "Apple"}, // 00:1E:52
    {0x001E58, "D-Link"}, // 00:1E:58
    {0x001E67, "Intel"}, // 00:1E:67
    {0x001E8C, "Asus"}, // 00:1E:8C
    {0x001EC2, "Apple"}, // 00:1E:C2
    {0x001F33, "Netgear"}, // 00:1F:33
    {0x001FFE, "Aruba/HP"}, // 00:1F:FE
    {0x002119, "Samsung"}, // 00:21:19
    {0x002127, "TP-Link"}, // 00:21:27
    {0x00215A, "Aruba/HP"}, // 00:21:5A
    {0x00216A, "Intel"}, // 00:21:6A
    {0x002191, "D-Link"}, // 00:21:91
    {0x0021E9, "Apple"}, // 00:21:E9
    {0x002215, "Asus"}, // 00:22:15
    {0x002241, "Apple"}, // 00:22:41
    {0x002275, "Belkin"}, // 00:22:75
    {0x0022B0, "D-Link"}, // 00:22:B0
    {0x002312, "Apple"}, // 00:23:12
    {0x002332, "Apple"}, // 00:23:32
    {0x002369, "Linksys"}, // 00:23:69
    {0x00236C, "Apple"}, // 00:23:6C
    {0x00237D, "Aruba/HP"}, // 00:23:7D
    {0x0023CD, "TP-Link"}, // 00:23:CD
    {0x0023D7, "Samsung"}, // 00:23:D7
    {0x002401, "D-Link"}, // 00:24:01
    {0x002436, "Apple"}, // 00:24:36
    {0x00246C, "Aruba/HP"}, // 00:24:6C
    {0x002484, "Ruckus"}, // 00:24:84
    {0x002497, "Cisco/Meraki"}, // 00:24:97
    {0x0024B2, "Netgear"}, // 00:24:B2
    {0x0024D7, "Intel"}, // 00:24:D7
    {0x0024FE, "AVM"}, // 00:24:FE
    {0x002500, "Apple"}, // 00:25:00
    {0x00254B, "Apple"}, // 00:25:4B
    {0x002561, "Aruba/HP"}, // 00:25:61
    {0x002586, "TP-Link"}, // 00:25:86
    {0x00259C, "Linksys"}, // 00:25:9C
    {0x00259E, "Huawei"}, // 00:25:9E
    {0x002608, "Apple"}, // 00:26:08
    {0x002618, "Asus"}, // 00:26:18
    {0x002637, "Samsung"}, // 00:26:37
    {0x00264A, "Apple"}, // 00:26:4A
    {0x00265A, "D-Link"}, // 00:26:5A
    {0x0026BB, "Apple"}, // 00:26:BB
    {0x0026F2, "Netgear"}, // 00:26:F2
    {0x00270D, "Cisco/Meraki"}, // 00:27:0D
    {0x002722, "Ubiquiti"}, // 00:27:22
    {0x004096, "Cisco/Meraki"}, // 00:40:96
    {0x009EC8, "Xiaomi"}, // 00:9E:C8
    {0x00E0FC, "Huawei"}, // 00:E0:FC
    {0x00FC8B, "Amazon"}, // 00:FC:8B
    {0x040CCE, "Apple"}, // 04:0C:CE
    {0x041552, "Apple"}, // 04:15:52
    {0x0418D6, "Ubiquiti"}, // 04:18:D6
    {0x042665, "Apple"}, // 04:26:65
    {0x045453, "Apple"}, // 04:54:53
    {0x049226, "Asus"}, // 04:92:26
    {0x08028E, "Netgear"}, // 08:02:8E
    {0x0808C2, "Samsung"}, // 08:08:C2
    {0x0819A6, "Huawei"}, // 08:19:A6
    {0x0836C9, "Netgear"}, // 08:36:C9
    {0x08606E, "Asus"}, // 08:60:6E
    {0x086698, "Apple"}, // 08:66:98
    {0x08863B, "Belkin"}, // 08:86:3B
    {0x0C3E9F, "Apple"}, // 0C:3E:9F
    {0x0C47C9, "Amazon"}, // 0C:47:C9
    {0x0C8DDB, "Cisco/Meraki"}, // 0C:8D:DB
    {0x100C6B, "Netgear"}, // 10:0C:6B
    {0x104780, "Huawei"}, // 10:47:80
    {0x107B44, "Asus"}, // 10:7B:44
    {0x1093E9, "Apple"}, // 10:93:E9
    {0x109ADD, "Apple"}, // 10:9A:DD
    {0x10BF48, "Asus"}, // 10:BF:48
    {0x10DA43, "Netgear"}, // 10:DA:43
    {0x14109F, "Apple"}, // 14:10:9F
    {0x14CC20, "TP-Link"}, // 14:CC:20
    {0x14D64D, "D-Link"}, // 14:D6:4D
    {0x14DDA9, "Asus"}, // 14:DD:A9
    {0x14F42A, "Samsung"}, // 14:F4:2A
    {0x185936, "Xiaomi"}, // 18:59:36
    {0x18742E, "Amazon"}, // 18:74:2E
    {0x18AF61, "Apple"}, // 18:AF:61
    {0x18E829, "Ubiquiti"}, // 18:E8:29
    {0x18FE34, "Espressif"}, // 18:FE:34
    {0x1C3BF3, "TP-Link"}, // 1C:3B:F3
    {0x1C56FE, "Google"}, // 1C:56:FE
    {0x1C7EE5, "D-Link"}, // 1C:7E:E5
    {0x1C872C, "Asus"}, // 1C:87:2C
    {0x200CC8, "Netgear"}, // 20:0C:C8
    {0x20DFB9, "Google"}, // 20:DF:B9
    {0x20E52A, "Netgear"}, // 20:E5:2A
    {0x20F3A3, "Huawei"}, // 20:F3:A3
    {0x240AC4, "Espressif"}, // 24:0A:C4
    {0x2462AB, "Espressif"}, // 24:62:AB
    {0x246F28, "Espressif"}, // 24:6F:28
    {0x24A43C, "Ubiquiti"}, // 24:A4:3C
    {0x24DCC3, "Espressif"}, // 24:DC:C3
    {0x24DEC6, "Aruba/HP"}, // 24:DE:C6
    {0x28107B, "D-Link"}, // 28:10:7B
    {0x286C07, "Xiaomi"}, // 28:6C:07
    {0x286ED4, "Huawei"}, // 28:6E:D4
    {0x288088, "Netgear"}, // 28:80:88
    {0x28940F, "Cisco/Meraki"}, // 28:94:0F
    {0x28CDC1, "Raspberry Pi"}, // 28:CD:C1
    {0x28CFDA, "Apple"}, // 28:CF:DA
    {0x2C4D54, "Asus"}, // 2C:4D:54
    {0x2C91AB, "AVM"}, // 2C:91:AB
    {0x2CB05D, "Netgear"}, // 2C:B0:5D
    {0x2CE6CC, "Ruckus"}, // 2C:E6:CC
    {0x2CF0EE, "Apple"}, // 2C:F0:EE
    {0x2CF432, "Espressif"}, // 2C:F4:32
    {0x2CFDA1, "Asus"}, // 2C:FD:A1
    {0x302303, "Belkin"}, // 30:23:03
    {0x30469A, "Netgear"}, // 30:46:9A
    {0x305A3A, "Asus"}, // 30:5A:3A
    {0x30AEA4, "Espressif"}, // 30:AE:A4
    {0x30DE4B, "TP-Link"}, // 30:DE:4B
    {0x3413E8, "Intel"}, // 34:13:E8
    {0x3423BA, "Samsung"}, // 34:23:BA
    {0x3431C4, "AVM"}, // 34:31:C4
    {0x3481C4, "AVM"}, // 34:81:C4
    {0x3810D5, "AVM"}, // 38:10:D5
    {0x38D547, "Asus"}, // 38:D5:47
    {0x38F73D, "Amazon"}, // 38:F7:3D
    {0x3C5A37, "Google"}, // 3C:5A:37
    {0x3C5CC4, "Amazon"}, // 3C:5C:C4
    {0x3C6105, "Espressif"}, // 3C:61:05
    {0x3C71BF, "Espressif"}, // 3C:71:BF
    {0x3CD92B, "Apple"}, // 3C:D9:2B
    {0x40167E, "Asus"}, // 40:16:7E
    {0x4022D8, "Espressif"}, // 40:22:D8
    {0x406C8F, "Apple"}, // 40:6C:8F
    {0x409BCD, "eero"}, // 40:9B:CD
    {0x40A6D9, "Apple"}, // 40:A6:D9
    {0x40B4CD, "Amazon"}, // 40:B4:CD
    {0x40E3D6, "Aruba/HP"}, // 40:E3:D6
    {0x44650D, "Amazon"}, // 44:65:0D
    {0x4494FC, "Netgear"}, // 44:94:FC
    {0x44D9E7, "Ubiquiti"}, // 44:D9:E7
    {0x483FDA, "Espressif"}, // 48:3F:DA
    {0x4846FB, "Huawei"}, // 48:46:FB
    {0x4851B7, "Intel"}, // 48:51:B7
    {0x485519, "Espressif"}, // 48:55:19
    {0x488F5A, "MikroTik"}, // 48:8F:5A
    {0x48D705, "Google"}, // 48:D7:05
    {0x500604, "Cisco/Meraki"}, // 50:06:04
    {0x50465D, "Asus"}, // 50:46:5D
    {0x508569, "Samsung"}, // 50:85:69
    {0x50C7BF, "TP-Link"}, // 50:C7:BF
    {0x50DCE7, "Amazon"}, // 50:DC:E7
    {0x50F5DA, "eero"}, // 50:F5:DA
    {0x5443B2, "Espressif"}, // 54:43:B2
    {0x545AA6, "Espressif"}, // 54:5A:A6
    {0x546009, "Google"}, // 54:60:09
    {0x589396, "Ruckus"}, // 58:93:96
    {0x5CCF7F, "Espressif"}, // 5C:CF:7F
    {0x600194, "Espressif"}, // 60:01:94
    {0x600308, "Apple"}, // 60:03:08
    {0x6045CB, "Asus"}, // 60:45:CB
    {0x60A44C, "TP-Link"}, // 60:A4:4C
    {0x647002, "TP-Link"}, // 64:70:02
    {0x64CC2E, "Xiaomi"}, // 64:CC:2E
    {0x64D154, "MikroTik"}, // 64:D1:54
    {0x6805CA, "Intel"}, // 68:05:CA
    {0x6837E9, "Amazon"}, // 68:37:E9
    {0x68545A, "Amazon"}, // 68:54:5A
    {0x685B35, "Apple"}, // 68:5B:35
    {0x68C63A, "Espressif"}, // 68:C6:3A
    {0x68D79A, "Ubiquiti"}, // 68:D7:9A
    {0x6C3B6B, "MikroTik"}, // 6C:3B:6B
    {0x6CF37F, "Aruba/HP"}, // 6C:F3:7F
    {0x70039F, "Espressif"}, // 70:03:9F
    {0x703EAC, "Google"}, // 70:3E:AC
    {0x704D7B, "Asus"}, // 70:4D:7B
    {0x704F57, "TP-Link"}, // 70:4F:57
    {0x706979, "Cisco/Meraki"}, // 70:69:79
    {0x707BE8, "Huawei"}, // 70:7B:E8
    {0x70886B, "eero"}, // 70:88:6B
    {0x70A741, "Ubiquiti"}, // 70:A7:41
    {0x70CD60, "Apple"}, // 70:CD:60
    {0x744D28, "MikroTik"}, // 74:4D:28
    {0x747548, "Amazon"}, // 74:75:48
    {0x7483C2, "Ubiquiti"}, // 74:83:C2
    {0x74911A, "Ruckus"}, // 74:91:1A
    {0x7811DC, "Xiaomi"}, // 78:11:DC
    {0x788A20, "Ubiquiti"}, // 78:8A:20
    {0x78E103, "Amazon"}, // 78:E1:03
    {0x7CDFA1, "Espressif"}, // 7C:DF:A1
    {0x7CFF4D, "AVM"}, // 7C:FF:4D
    {0x802AA8, "Ubiquiti"}, // 80:2A:A8
    {0x807D3A, "Espressif"}, // 80:7D:3A
    {0x8086F2, "Intel"}, // 80:86:F2
    {0x840D8E, "Espressif"}, // 84:0D:8E
    {0x8416F9, "TP-Link"}, // 84:16:F9
    {0x841B5E, "Netgear"}, // 84:1B:5E
    {0x842519, "Samsung"}, // 84:25:19
    {0x84D6D0, "Google"}, // 84:D6:D0
    {0x84F3EB, "Espressif"}, // 84:F3:EB
    {0x8843E1, "Cisco/Meraki"}, // 88:43:E1
    {0x88665A, "Apple"}, // 88:66:5A
    {0x888603, "Huawei"}, // 88:86:03
    {0x88C9D0, "eero"}, // 88:C9:D0
    {0x8C0C90, "Ruckus"}, // 8C:0C:90
    {0x8C705A, "Intel"}, // 8C:70:5A
    {0x90380C, "Espressif"}, // 90:38:0C
    {0x90F652, "TP-Link"}, // 90:F6:52
    {0x90FD61, "Apple"}, // 90:FD:61
    {0x9463D1, "Samsung"}, // 94:63:D1
    {0x94B40F, "Aruba/HP"}, // 94:B4:0F
    {0x94B555, "Espressif"}, // 94:B5:55
    {0x94B97E, "Espressif"}, // 94:B9:7E
    {0x94EBCD, "Google"}, // 94:EB:CD
    {0x98DAC4, "TP-Link"}, // 98:DA:C4
    {0x9C3DCF, "Netgear"}, // 9C:3D:CF
    {0x9CC7A6, "AVM"}, // 9C:C7:A6
    {0xA002DC, "Amazon"}, // A0:02:DC
    {0xA00460, "Netgear"}, // A0:04:60
    {0xA020A6, "Espressif"}, // A0:20:A6
    {0xA0F3C1, "TP-Link"}, // A0:F3:C1
    {0xA44CC8, "Intel"}, // A4:4C:C8
    {0xA47733, "Google"}, // A4:77:33
    {0xA47B9D, "Espressif"}, // A4:7B:9D
    {0xA4C138, "Espressif"}, // A4:C1:38
    {0xAC220B, "Asus"}, // AC:22:0B
    {0xAC3B77, "eero"}, // AC:3B:77
    {0xAC63BE, "Amazon"}, // AC:63:BE
    {0xAC67B2, "Espressif"}, // AC:67:B2
    {0xAC84C6, "TP-Link"}, // AC:84:C6
    {0xAC8BA9, "Ubiquiti"}, // AC:8B:A9
    {0xACA31E, "Aruba/HP"}, // AC:A3:1E
    {0xACBC32, "Apple"}, // AC:BC:32
    {0xACE215, "Huawei"}, // AC:E2:15
    {0xB04E26, "TP-Link"}, // B0:4E:26
    {0xB07FB9, "Netgear"}, // B0:7F:B9
    {0xB0C554, "D-Link"}, // B0:C5:54
    {0xB4E62D, "Espressif"}, // B4:E6:2D
    {0xB4FBE4, "Ubiquiti"}, // B4:FB:E4
    {0xB827EB, "Raspberry Pi"}, // B8:27:EB
    {0xB869F4, "MikroTik"}, // B8:69:F4
    {0xBC0543, "AVM"}, // BC:05:43
    {0xBC52B7, "Apple"}, // BC:52:B7
    {0xBC72B9, "Samsung"}, // BC:72:B9
    {0xBCDDC2, "Espressif"}, // BC:DD:C2
    {0xBCEE7B, "Asus"}, // BC:EE:7B
    {0xC006C3, "TP-Link"}, // C0:06:C3
    {0xC02506, "AVM"}, // C0:25:06
    {0xC025E9, "TP-Link"}, // C0:25:E9
    {0xC03F0E, "Netgear"}, // C0:3F:0E
    {0xC04A00, "TP-Link"}, // C0:4A:00
    {0xC0FFD4, "Netgear"}, // C0:FF:D4
    {0xC4108A, "Ruckus"}, // C4:10:8A
    {0xC44F33, "Espressif"}, // C4:4F:33
    {0xC47295, "Cisco/Meraki"}, // C4:72:95
    {0xC4AD34, "MikroTik"}, // C4:AD:34
    {0xC4DD57, "Espressif"}, // C4:DD:57
    {0xC8D3A3, "D-Link"}, // C8:D3:A3
    {0xCC07AB, "Samsung"}, // CC:07:AB
    {0xCC4463, "eero"}, // CC:44:63
    {0xCC50E3, "Espressif"}, // CC:50:E3
    {0xCCCE1E, "AVM"}, // CC:CE:1E
    {0xD4CA6D, "MikroTik"}, // D4:CA:6D
    {0xD80D17, "TP-Link"}, // D8:0D:17
    {0xD86C63, "Google"}, // D8:6C:63
    {0xD8A01D, "Espressif"}, // D8:A0:1D
    {0xD8C7C8, "Aruba/HP"}, // D8:C7:C8
    {0xDC396F, "AVM"}, // DC:39:6F
    {0xDC4F22, "Espressif"}, // DC:4F:22
    {0xDC5475, "Espressif"}, // DC:54:75
    {0xDC9FDB, "Ubiquiti"}, // DC:9F:DB
    {0xDCA632, "Raspberry Pi"}, // DC:A6:32
    {0xDCA904, "Apple"}, // DC:A9:04
    {0xE0286D, "AVM"}, // E0:28:6D
    {0xE0469A, "Netgear"}, // E0:46:9A
    {0xE0553D, "Cisco/Meraki"}, // E0:55:3D
    {0xE063DA, "Ubiquiti"}, // E0:63:DA
    {0xE0CBBC, "Cisco/Meraki"}, // E0:CB:BC
    {0xE0E2E6, "Espressif"}, // E0:E2:E6
    {0xE45F01, "Raspberry Pi"}, // E4:5F:01
    {0xE48D8C, "MikroTik"}, // E4:8D:8C
    {0xE4C32A, "Huawei"}, // E4:C3:2A
    {0xE4F042, "Google"}, // E4:F0:42
    {0xE848B8, "TP-Link"}, // E8:48:B8
    {0xE868E7, "Espressif"}, // E8:68:E7
    {0xE8802E, "Apple"}, // E8:80:2E
    {0xE89F6D, "Espressif"}, // E8:9F:6D
    {0xEC086B, "TP-Link"}, // EC:08:6B
    {0xEC1A59, "Belkin"}, // EC:1A:59
    {0xEC6260, "Espressif"}, // EC:62:60
    {0xEC94CB, "Espressif"}, // EC:94:CB
    {0xF01898, "Apple"}, // F0:18:98
    {0xF09FC2, "Ubiquiti"}, // F0:9F:C2
    {0xF40304, "Google"}, // F4:03:04
    {0xF4559C, "Huawei"}, // F4:55:9C
    {0xF4CFA2, "Espressif"}, // F4:CF:A2
    {0xF4F26D, "TP-Link"}, // F4:F2:6D
    {0xF4F5DB, "Google"}, // F4:F5:DB
    {0xF8FFC2, "Apple"}, // F8:FF:C2
    {0xFC65DE, "Amazon"}, // FC:65:DE
    {0xFCA667, "Amazon"}, // FC:A6:67
};

static const size_t OUI_TABLE_SIZE = sizeof(OUI_TABLE) / sizeof(OUI_TABLE[0]);

size_t getOuiTableSize() {
    return OUI_TABLE_SIZE;
}

const OuiEntry* getOuiTable() {
    return OUI_TABLE;
}

bool parseMacPrefix(const char* macStr, uint32_t* prefixOut) {
    if (!macStr || !prefixOut) return false;

    while (*macStr == ' ' || *macStr == '\t') {
        macStr++;
    }

    uint32_t result = 0;
    int octetCount = 0;
    int digitCount = 0;
    uint8_t currentOctet = 0;

    for (const char* p = macStr; *p != '\0' && octetCount < 3; ++p) {
        char c = *p;
        int val = -1;
        if (c >= '0' && c <= '9') val = c - '0';
        else if (c >= 'a' && c <= 'f') val = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') val = c - 'A' + 10;
        else if (c == ':' || c == '-') {
            if (digitCount > 0) {
                result = (result << 8) | currentOctet;
                octetCount++;
                digitCount = 0;
                currentOctet = 0;
            }
            continue;
        } else {
            break;
        }

        currentOctet = (currentOctet << 4) | (uint8_t)val;
        digitCount++;
        if (digitCount == 2) {
            result = (result << 8) | currentOctet;
            octetCount++;
            digitCount = 0;
            currentOctet = 0;
            if (*(p + 1) == ':' || *(p + 1) == '-') {
                p++;
            }
        }
    }

    if (digitCount > 0 && octetCount < 3) {
        result = (result << 8) | currentOctet;
        octetCount++;
    }

    if (octetCount == 3) {
        *prefixOut = result;
        return true;
    }
    return false;
}

const char* getOuiVendor(uint32_t prefix) {
    size_t low = 0;
    size_t high = OUI_TABLE_SIZE;
    while (low < high) {
        size_t mid = low + (high - low) / 2;
        if (OUI_TABLE[mid].prefix < prefix) {
            low = mid + 1;
        } else if (OUI_TABLE[mid].prefix > prefix) {
            high = mid;
        } else {
            return OUI_TABLE[mid].vendor;
        }
    }
    return "Unknown Vendor";
}

const char* getOuiVendor(const uint8_t* bssid) {
    if (!bssid) return "Unknown Vendor";
    uint32_t prefix = ((uint32_t)bssid[0] << 16) | ((uint32_t)bssid[1] << 8) | (uint32_t)bssid[2];
    return getOuiVendor(prefix);
}

const char* getOuiVendor(const char* bssidStr) {
    if (!bssidStr) return "Unknown Vendor";
    uint32_t prefix = 0;
    if (parseMacPrefix(bssidStr, &prefix)) {
        return getOuiVendor(prefix);
    }
    return "Unknown Vendor";
}

const char* getOuiVendor(const String& bssidStr) {
    return getOuiVendor(bssidStr.c_str());
}
