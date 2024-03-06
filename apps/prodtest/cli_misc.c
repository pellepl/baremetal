/* Copyright (c) 2019 Peter Andersson (pelleplutt1976<at>gmail.com) */
/* MIT License (see ./LICENSE) */

#include "prodtest.h"
#include "cli.h"
#include "minio.h"
#include "board.h"
#include "cpu.h"

#if NO_EXTRA_CLI == 0

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
CLI_FUNCTION(cli_info_app, "app", "list application information");

static int cli_arch(int argc, const char **argv) {
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
CLI_FUNCTION(cli_arch, "arch", "list arch information");

static void toggle_dcdc_state(uint8_t dcdc_state)
{
    if (NRF_FICR->INFO.PART == 0x52840 || NRF_FICR->INFO.PART == 0x52833)
    {
        if (dcdc_state == 0)
        {
            NRF_POWER->DCDCEN0 = (NRF_POWER->DCDCEN0 == POWER_DCDCEN0_DCDCEN_Disabled) ? 1 : 0;
        }
        else if (dcdc_state == 1)
        {
            NRF_POWER->DCDCEN = (NRF_POWER->DCDCEN == POWER_DCDCEN_DCDCEN_Disabled) ? 1 : 0;
        }
    }
    else
    {
        if (dcdc_state <= 1)
        {
            NRF_POWER->DCDCEN = dcdc_state;
        }
    }
}

static int cli_toggle_dc(int argc, const char **argv)
{
    if (argc != 1)
    {
        return ERR_CLI_EINVAL;
    }
    toggle_dcdc_state(atoi(argv[0]) ? 1 : 0);

    uint32_t dcdcen = NRF_POWER->DCDCEN;
    if (NRF_FICR->INFO.PART == 0x52840 || NRF_FICR->INFO.PART == 0x52833)
    {
        printf("DCDC REG0:%d\n"
               "DCDC REG1:%d\n"
               "'0' to toggle REG0\n"
               "'1' to toggle REG1\n",
               NRF_POWER->DCDCEN0,
               dcdcen);
    }
    else
    {
        printf("DCDC:%d\n"
               "'1':enable, '0':disable\n",
               dcdcen);
    }
    return 0;
}
CLI_FUNCTION(cli_toggle_dc, "dcdc", "control dcdc: 0|1");

static int cli_help(int argc, const char **argv)
{
    print_commands(false, 20);
    printf("\r\n");
    print_commands(true, 6);
    return 0;
}
CLI_FUNCTION(cli_help, "help", "display help");

static int cli_reset(int argc, const char **argv)
{
    cpu_reset();
    return 0;
}
CLI_FUNCTION(cli_reset, "reset", "reset");

#endif