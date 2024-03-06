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
