/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "cli.h"
#include "minio.h"
#include "board.h"
#include "cpu.h"

#define _str(x) __str(x)
#define __str(x) #x

static int cli_info_app(int argc, const char **argv) {
   printf("target:\t%s %s/%s/%s@%s\n",
       _str(BUILD_INFO_TARGET_NAME),
       _str(BUILD_INFO_TARGET_ARCH), _str(BUILD_INFO_TARGET_FAMILY), _str(BUILD_INFO_TARGET_PROC),
       _str(BUILD_INFO_TARGET_BOARD));
   printf("rev:\t%s %s on %s\n",
       _str(BUILD_INFO_GIT_COMMIT), _str(BUILD_INFO_GIT_TAG), _str(BUILD_INFO_GIT_BRANCH))
   printf("build:\t%s@%s %s-%s %s\n",
       _str(BUILD_INFO_HOST_WHO), _str(BUILD_INFO_HOST_NAME),
       _str(BUILD_INFO_HOST_WHEN_DATE), _str(BUILD_INFO_HOST_WHEN_TIME),
       _str(BUILD_INFO_HOST_WHEN_EPOCH));
   printf("cc:\t%s %s\n",
       _str(BUILD_INFO_CC_MACHINE),
       _str(BUILD_INFO_CC_VERSION));

   return 0;
}
CLI_FUNCTION(cli_info_app, "info_app", "");

#if 0 // BUILD_INFO_TARGET_FAMILY == nrf52
// TODO this should be halified someway
#include "nrf52.h"
static int cli_info_arch(int argc, const char **argv) {
   printf("part:\t%x\n", NRF_FICR->INFO.PART);
   printf("var:\t%c%c%c%c\n",
       (NRF_FICR->INFO.VARIANT >> 24) &0xff,
       (NRF_FICR->INFO.VARIANT >> 16) &0xff,
       (NRF_FICR->INFO.VARIANT >> 8) &0xff,
       (NRF_FICR->INFO.VARIANT >> 0) &0xff
   );
   printf("pack:\t%x\n", NRF_FICR->INFO.PACKAGE);
   printf("ram:\t%d kB\n", NRF_FICR->INFO.RAM);
   printf("flash:\t%d kB\n", NRF_FICR->INFO.FLASH);

   printf("dev-id:\t0x%08x%08x\n", NRF_FICR->DEVICEID[0], NRF_FICR->DEVICEID[1]);
   printf("pages:\t%d\n", NRF_FICR->CODESIZE);
   printf("pagesz:\t%d bytes\n", NRF_FICR->CODEPAGESIZE);
   return 0;
}
CLI_FUNCTION(cli_info_arch, "info_arch", "");
#endif
