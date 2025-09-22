#pragma once
#define CH13613_USER_NOP 0x00           // No Operation (0000h)
#define CH13613_USER_SWRESET 0x01       // Software Reset (0100h)
#define CH13613_USER_RDDID 0x04         // Read Display ID (0400h~0402h
#define CH13613_USER_RDNUMED 0x05       // Read Number of Errors on DSI (0500h)
#define CH13613_USER_RDDPM 0x0A         // Read display power mode (0A00h
#define CH13613_USER_RDDMADCTR 0x0B     // Read Display MADCTR (0B00h)
#define CH13613_USER_RDDCOLMOD 0x0C     // Read Display Pixel Format (0C00h
#define CH13613_USER_RDDIM 0x0D         // Read Display Image Mode (0D00h)
#define CH13613_USER_RDDSM 0x0E         // Read Display Signal Mode (0E00h)
#define CH13613_USER_RDDSDR 0x0F        // Read Display Self-Diagnostic Result (0F00h)
#define CH13613_USER_SLPIN 0x10         // Sleep In (1000h)
#define CH13613_USER_SLPOUT 0x11        // Sleep Out (1100h)
#define CH13613_USER_PTLON 0x12         // Partial Display Mode On(1200h)
#define CH13613_USER_NORON 0x13         // Normal Display Mode on (1300h)
#define CH13613_USER_INVOFF 0x20        // Display Inversion Off (2000h)
#define CH13613_USER_INVON 0x21         // Display Inversion On (2100h)
#define CH13613_USER_ALLPOFF 0x22       // All Pixel Off (2200h)
#define CH13613_USER_ALLPON 0x23        // All Pixel On (2300h)
#define CH13613_USER_DISPOFF 0x28       // Display Off (2800h)
#define CH13613_USER_DISPON 0x29        // Display On (2900h)
#define CH13613_USER_CASET 0x2A         // Set Column Start Address(2A00h~2A03h)
#define CH13613_USER_RASET 0x2B         // Set Row Start Address(2B00h~2B03h)
#define CH13613_USER_RAMW 0x2C          // Write memory Start (2Ch
#define CH13613_USER_RAMR 0x2E          // Read memory (2Eh)
#define CH13613_USER_HPTLAR 0x30        // Horizontal Partial Area (3000h~3003h)
#define CH13613_USER_VPTLAR 0x31        // Vertical Partial Area (3100h~3101h)
#define CH13613_USER_TEOFF 0x34         // TE Signal OFF (3400h)
#define CH13613_USER_TEON 0x35          // TE Signal ON (3500h
#define CH13613_USER_SDC 0x36           // ScanDirectionControl(3600h)
#define CH13613_USER_IDMOFF 0x38        // Idle Mode Off (3800h)
#define CH13613_USER_IDMON 0x39         // Idle mode On (3900h
#define CH13613_USER_IPF 0x3A           // Interface Pixel Format(3A00h)
#define CH13613_USER_RAMWC 0x3C         // Write memory Continue (3Ch)
#define CH13613_USER_RAMRDC 0x3E        // Memory Continuous Read (3Eh)
#define CH13613_USER_STESL 0x44         // Set tearing effect scan line (4400h~4401h)
#define CH13613_USER_GSL 0x45           // Get Scan Line (4500h~4501h
#define CH13613_USER_RAMW_AOD 0x4c      // Write AOD RAM Start (4Ch)
#define CH13613_USER_DSTBON 0x4F        // Deep Standby Mode On (4F00h)
#define CH13613_USER_WDB 0x51           // Write Display Brightness (5100h)
#define CH13613_USER_RDB 0x52           // Read Display Brightness (5200h)
#define CH13613_USER_HBMSEL 0x53        // High Brightness Mode Selection (5300h)
#define CH13613_USER_RHBM 0x54          // Read High Brightness Mode (5400h)
#define CH13613_USER_NEM 0x55           // Emission Width for Normal Mode (5500h)
#define CH13613_USER_IEM 0x56           // Emission Width for Idle mode (5600h)
#define CH13613_USER_HEM 0x57           // Emission Width for HBM mode (5700h)
#define CH13613_USER_RAMWC_AOD 0x5C     // Write AOD RAM Continue (5Ch
#define CH13613_USER_BACTR 0x60         // Brightness Adjustment Control (6000h)
#define CH13613_USER_DECTR 0x62         // Dynamic ELVSS Function Control (6200h)
#define CH13613_USER_SRECTR 0x63        // Sunlight Readable Enhance Function Control (6300h)
#define CH13613_USER_GACMSET 0x64       // Gamma Set and Color ModeSet (6400h)
#define CH13613_USER_SPIRDC 0x65        // SPI read manufacture command control (6500h)
#define CH13613_USER_CICEN 0x67         // Circular Edge Optimization Algorithm Enable(6700h)
#define CH13613_USER_OLOAEN 0x68        // Oblique Line Optimization Algorithm Enable (6800h)
#define CH13613_USER_NOTCHEN 0x69       // Notch AlgorithmEnable (6900h)
#define CH13613_USER_INCIREN 0x6A       // Inside CircularAlgorithmEnable(6A00h
#define CH13613_USER_SWIREMANU 0x6C     // SWIRE manual control (6C00h)
#define CH13613_USER_SPIRDC_EN 0x6D     // SPI read manufacture command Enable (6D00h)
#define CH13613_USER_AODCLKS 0x82       // AOD clock setting (8200h)
#define CH13613_USER_AODTIME 0x83       // AOD Time setting (8300h~8302h)
#define CH13613_USER_AODSECCAL 0x84     // AOD second hand calibration method (8400h)
#define CH13613_USER_AODSYNCP 0x85      // AOD synchronization cycle setting (8500h~8504h
#define CH13613_USER_DCLKFSIZE 0x86     // Digital clock font size setting (8600h~8602h)
#define CH13613_USER_DHCOLORR 0x87      // Digital clock Hour color setting (8700h~87FFh)
#define CH13613_USER_DHCOLORG 0x88      // Digital clock Hour color setting (8800h~88FFh)
#define CH13613_USER_DHCOLORB 0x89      // Digital clock Hour color setting (8900h~89FFh)
#define CH13613_USER_DHCOLORA 0x8A      // Digital clock Hour color setting (8A00h~8AFFh)
#define CH13613_USER_DMCOLORR 0x8B      // Digital clock minute color setting (8B00h~8BFFh)
#define CH13613_USER_DMCOLORG 0x8C      // Digital clock minute color setting (8C00h~8CFFh)
#define CH13613_USER_DMCOLORB 0x8D      // Digital clock minute color setting (8D00h~8DFFh)
#define CH13613_USER_DMCOLORA 0x8E      // Digital clock minute color setting (8E00h~8EFFh)
#define CH13613_USER_DCCOLOR 0x8F       // Digital clock colon color setting (8F00h
#define CH13613_USER_DCLKHC 0x90        // Digital clock location setting (9000h-9002h)
#define CH13613_USER_DCLKOCS 0x91       // Digital clock offset coordinate setting (9100h-910Ch)
#define CH13613_USER_ACLKSET 0x92       // Analog clock setting (9200h)
#define CH13613_USER_ACLKRCC 0x93       // Analog clock rotate center coordinate setting (9300h~9303h)
#define CH13613_USER_ACLKCLS 0x94       // Analog clock Center location setting (9400h~9402h)
#define CH13613_USER_DEBURNSET 0x95     // AOD De-burn-in setting (9500h~9503h)
#define CH13613_USER_DEBURNDIR_LSB 0x96 // AOD De-burn-in direction of each step setting (9600h~960Fh)
#define CH13613_USER_DEBURNDIR_MSB 0x97 // AOD De-burn-in direction of each step setting (9700h~970Fh)
#define CH13613_USER_RDID1 0xDA         // Read ID1 Value (DA00h
#define CH13613_USER_RDID2 0xDB         // Read ID2 Value (DB00h)
#define CH13613_USER_RDID3 0xDC         // Read ID3 Value (DC00h)

#define CH13613_M_MAUCCTR 0xf0    // Manufacture Command Set Control (F000h)
#define CH13613_M_SETPAINDEX 0xf1 // Set Parameter Index in MIPI Interface (F100h)

#define CH13613_MP0_IFCTR1 0xB0     // Interface Signals Control 1
#define CH13613_MP0_IFCTR2 0xB1     // Interface Signal Control 2
#define CH13613_MP0_IFCTR3 0xB2     // interface signal control 3
#define CH13613_MP0_IFCTR4 0xB3     // MIPIinterface signal control 4
#define CH13613_MP0_DOPCTR 0xB4     // Display Option Control
#define CH13613_MP0_DPRESCTR 0xB5   // Display source number control
#define CH13613_MP0_DPSLCTR 0xB6    // Display Scan Line Control
#define CH13613_MP0_GSEQCTR 0xB7    // Charge Control Function for GOA Signals
#define CH13613_MP0_SWEQCTR 0xB8    //  Switch signal EQ time control
#define CH13613_MP0_STDIM 0xB9      //  Set Display Interface mode
#define CH13613_MP0_VPGSTS 0xBA     //  Display Control in Vertical Porch Time
#define CH13613_MP0_GRAYOPT1 0xBB   // GRAY option1
#define CH13613_MP0_DISCLK 0xBE     // Display Clock Control
#define CH13613_MP0_BGCFG 0xBF      // Non-display Background configuration for Partial
#define CH13613_MP0_SETPAINDEX 0xF1 //  Set Parameter Index in MIPI Interface
#define CH13613_MP0_SDTCTR 0xC0     //  Source Data Output Timing Control
#define CH13613_MP0_OFSWOS 0xC1     //  Odd frame odd line and even line switch output select
#define CH13613_MP0_EFSWOS 0xC2     //  Even frame odd line and even line switch output select
#define CH13613_MP0_OFSDOS 0xC3     //  Odd frame odd line and even line source data output order select
#define CH13613_MP0_EFSDOS 0xC4     //  Even frame odd line and even line source data output order select
#define CH13613_MP0_IDLESDTCTR 0xC6 //  Source Data Output Timing Control for idle mode
#define CH13613_MP0_GAMDIT 0xD0     //  Gamma Dithering Control
#define CH13613_MP0_ERRFGCTR 0xD1   //  ERR_FG control
#define CH13613_MP0_MCKSM 0xD2      // The CRC of the last long packet data
#define CH13613_MP0_ETPSD 0xD4      //  Touch Panel Sync Definition
#define CH13613_MP0_THSYNC 0xD6     //  The period of internal Hsync
#define CH13613_MP0_BCEN 0xD7       //  Brightness Control Enable
#define CH13613_MP0_SOURCEOPT1 0xD8 // SOURCE option1
#define CH13613_MP0_RAMCKSUMEN 0xD9 //  RAM Checksum Control
#define CH13613_MP0_RAMCK 0xDD      //  RAM Checksum Value
#define CH13613_MP0_BISTEN 0xE0     //  BIST mode enable control
#define CH13613_MP0_BISTCON 0xE1    //  BIST Cycle and Pattern Control
#define CH13613_MP0_BISTPAT 0xE2    //  BIST pattern set
#define CH13613_MP0_COLMS1 0xE3     //  color mapping set1 for SPI111 pixel format
#define CH13613_MP0_COLMS2 0xE4     //  color mapping set2 for SPI111 pixel format
#define CH13613_MP0_COLMS3 0xE5     //  color mapping set3 for SPI111 pixel format
#define CH13613_MP0_COLMS4 0xE6     //  color mapping set4 for SPI111 pixel format

#define CH13613_MP1_SETVREFP 0xB2    //  Setting VREFP Voltage
#define CH13613_MP1_SETVREFN 0xB3    //  Setting VREFN Voltage
#define CH13613_MP1_SETVGHO 0xB4     //  Setting VGHO Voltage
#define CH13613_MP1_SETVGLO 0xB5     //  Setting VGLO Voltage
#define CH13613_MP1_BT1CTR 0xBA      //  BT1 Power Control for AVDD
#define CH13613_MP1_BT2CTR 0xBB      //  BT2 Power Control for AVEE
#define CH13613_MP1_BT3CTR 0xBC      //  BT3 Power Control for VGH
#define CH13613_MP1_BT4CTR 0xBD      //  BT4 Power Control for VGL
#define CH13613_MP1_GOUTEN 0xBE      //  Gamma Voltage output enable
#define CH13613_MP1_PVSW 0xC0        //  S-wire Pulse Number for Positive Voltage
#define CH13613_MP1_ELVSSCMD 0xC1    //  S-wire Pulse Number for ELVSS
#define CH13613_MP1_FDCMD 0xC2       // S-wire Pulse Number for Fast Discharge
#define CH13613_MP1_SWIRECTR 0xC3    //  S-wire Control
#define CH13613_MP1_SWIREFRESH 0xC4  //  SWIRE Refresh Function
#define CH13613_MP1_SWIRESEL 0xC5    //  SWIRE auto or manual control selection
#define CH13613_MP1_RDIDPRD 0xC6     //  Read ID for IC Production Code
#define CH13613_MP1_WRDID 0xC7       //  Write Display ID
#define CH13613_MP1_BSPE 0xC9        //  Border Swire Pulse for ELVSS/VEN
#define CH13613_MP1_PBMCTR 0xCB      //  PowerBuild Mode Control
#define CH13613_MP1_PSCTR 0xCC       //  Power Switch Control
#define CH13613_MP1_SFCFG 0xCD       //  Scan Frame Configuration for Idle Modeand Normal Mode
#define CH13613_MP1_GSSCFG 0xCE      //  GOA Scan Status Configuration Non-Display
#define CH13613_MP1_SETVEPN 0xCF     //  Set Internal Build VEP and VEN Voltage
#define CH13613_MP1_PWRONCTR 0xD0    //  Power On Output Time Control
#define CH13613_MP1_DISPONPCTR1 0xD1 //  Display On Process Control1
#define CH13613_MP1_DISPONPCTR2 0xD2 //  Display On Process Control2
#define CH13613_MP1_DISPOFFPCTR 0xD3 //  Display Off Process Control
#define CH13613_MP1_PWROFFPCTR 0xD4  //  Power Off Time Control
#define CH13613_MP1_FBFONGOA 0xD7    //  GOA Signals Output State Setting in Power On Front Blank Frame
#define CH13613_MP1_BONGOA 0xD8      //  GOA Signals Output State Setting in Power On Back Blank Frame
#define CH13613_MP1_MTPPWRCTR1 0xEB  //  MTP Power Control1
#define CH13613_MP1_MTPPWRCTR2 0xEC  //  MTP Power Control2
#define CH13613_MP1_MTPEN 0xED       //  MTP Enable Control
#define CH13613_MP1_MTPWR 0xEE       //  MTP Write

#define CH13613_MP2_SETVG 0xB0     //  Setting Gamma1 Voltage
#define CH13613_MP2_GMPEN 0xB1     //  Select Gamma Point Correction Mode
#define CH13613_MP2_GMA1RCTR1 0xB2 //  Gamma1 Correction for Red1
#define CH13613_MP2_GMA1RCTR2 0xB3 //  Gamma1 Correction for Red2
#define CH13613_MP2_GMA1RCTR3 0xB4 //  Gamma1 Correction for Red3
#define CH13613_MP2_GMA1GCTR1 0xB5 //  Gamma1 Correction for Green1
#define CH13613_MP2_GMA1GCTR2 0xB6 //  Gamma1 Correction for Green2
#define CH13613_MP2_GMA1GCTR3 0xB7 //  Gamma1 Correction for Green3
#define CH13613_MP2_GMA1BCTR1 0xB8 //  Gamma1 Correction for Blue1
#define CH13613_MP2_GMA1BCTR2 0xB9 //  Gamma1 Correction for Blue2
#define CH13613_MP2_GMA1BCTR3 0xBA //  Gamma1 Correction for Blue3
#define CH13613_MP2_SETVG2 0xC0    //  Setting Gamma2 Voltage
#define CH13613_MP2_GMA2BCTR1 0xC2 //  Gamma2 Correction for Red1
#define CH13613_MP2_GMA2BCTR2 0xC3 //  Gamma2 Correction for Red2
#define CH13613_MP2_GMA2BCTR3 0xC4 //  Gamma2 Correction for Red3
#define CH13613_MP2_GMA2BCTG1 0xC5 //  Gamma2 Correction for Green1
#define CH13613_MP2_RDMTP 0xEF     //  Read MTP Status
#define CH13613_MP2_GMA2BCTG2 0xC6 //  Gamma2 Correction for Green2
#define CH13613_MP2_GMA2BCTG3 0xC7 //  Gamma2 Correction for Green3
#define CH13613_MP2_GMA2BCTB1 0xC8 //  Gamma2 Correction for Blue1
#define CH13613_MP2_GMA2BCTB2 0xC9 //  Gamma2 Correction for Blue2
#define CH13613_MP2_GMA2BCTB3 0xCA //  Gamma2 Correction for Blue3
#define CH13613_MP2_SETVG3 0xD0    //  Setting Gamma3 Voltage
#define CH13613_MP2_GMA3BCTR1 0xD2 //  Gamma3 Correction for Red1
#define CH13613_MP2_GMA3BCTR2 0xD3 //  Gamma3 Correction for Red2
#define CH13613_MP2_GMA3BCTR3 0xD4 //  Gamma3 Correction for Red3
#define CH13613_MP2_GMA3BCTG1 0xD5 //  Gamma3 Correction for Green1
#define CH13613_MP2_GMA3BCTG2 0xD6 //  Gamma3 Correction for Green2
#define CH13613_MP2_GMA3BCTG3 0xD7 //  Gamma3 Correction for Green3
#define CH13613_MP2_GMA3BCTB1 0xD8 //  Gamma3 Correction for Blue1
#define CH13613_MP2_GMA3BCTB2 0xD9 //  Gamma3 Correction for Blue2
#define CH13613_MP2_GMA3BCTB3 0xDD //  Gamma3 Correction for Blue3

#define CH13613_MP3_GRPS 0xB0         // GOA reference point shift
#define CH13613_MP3_GSTVCTR1 0xB1     // GOA STV1 control
#define CH13613_MP3_GSTVCTR2 0xB2     // GOA STV2 control
#define CH13613_MP3_GSTVCTR3 0xB3     // GOA STV3 control
#define CH13613_MP3_GSTVCTR4 0xB4     // GOA STV4 control
#define CH13613_MP3_GCKVCTR1 0xB5     // GOA CKV1 control
#define CH13613_MP3_GCKVCTR2 0xB6     // GOA CKV2 CONTROL
#define CH13613_MP3_GCKVCTR3 0xB7     // GOA CKV3 control
#define CH13613_MP3_GCKVCTR4 0xB8     // GOA CKV4 control
#define CH13613_MP3_GCKVCTR5 0xB9     // GOA CKV5 control
#define CH13613_MP3_GCKVCTR6 0xBA     // GOA CKV6 control
#define CH13613_MP3_GCKVCTR7 0xBB     // GOA CKV7 control
#define CH13613_MP3_GCKVCTR8 0xBC     // GOA CKV8 control
#define CH13613_MP3_GOASWAP 0xBE      //  Swap and Individually control
#define CH13613_MP3_IMGPS 0xBF        //  Idle mode STV and CKVand SW parameter set select
#define CH13613_MP3_GOATSEL 0xC1      // GOA signal type select
#define CH13613_MP3_MUXTSEL 0xC2      // MUX signal type select
#define CH13613_MP3_GSTV1ICTR1 0xC3   //  GOA STV1 Idle control
#define CH13613_MP3_GSTV2ICTR2 0xC4   //  GOA STV2 Idle control
#define CH13613_MP3_GSTV3ICTR3 0xC5   //  GOA STV3 Idle control
#define CH13613_MP3_GSTV4ICTR4 0xC6   //  GOA STV4 Idle control
#define CH13613_MP3_GCKV1ICTR1 0xC7   //  GOA CKV1 Idle control
#define CH13613_MP3_GCKV2ICTR2 0xC8   //  GOA CKV2 Idle control
#define CH13613_MP3_GCKV3ICTR3 0xC9   //  GOA CKV3 Idle control
#define CH13613_MP3_GCKV4ICTR4 0xCA   //  GOA CKV4 Idle control
#define CH13613_MP3_GCKV5ICTR5 0xCB   //  GOA CKV5 Idle control
#define CH13613_MP3_GCKV6ICTR6 0xCC   //  GOA CKV6 Idle control
#define CH13613_MP3_GCKV7ICTR7 0xCD   //  GOA CKV7 Idle control
#define CH13613_MP3_GCKV8ICTR8 0xCE   //  GOA CKV8 Idle control
#define CH13613_MP3_GOATSEL_IDLE 0xCF //  GOA signal type select
