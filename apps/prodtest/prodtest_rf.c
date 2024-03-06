#include "prodtest.h"
#include "cli.h"
#include "minio.h"
#include "dtm_uart.h"

static int cli_bttm(int argc, const char **argv) {
	if (argc != 1 || argv[0][0] != '1')
		return ERR_CLI_EINVAL;

	printf("BTTM START;\r\n");
	return dtm_start("BTTM=0;");
}
CLI_FUNCTION(cli_bttm, "BTTM", "Put the device in BT Test Mode");