#include "prodtest.h"
#include "cli.h"
#include "minio.h"
#include "uicr.h"

static int cli_hwid(int argc, const char **argv)
{
	char item_id[UICR_STRLEN_ITEM_ID];
	char serial[UICR_STRLEN_SERIAL];

    const target_t *target = target_get();
    target_hw_identifier_t hwid;
    uicr_get_hw_identifier(&hwid);

    if (strcmp(TARGET_PASCAL, target->name) == 0) {
        volatile const uicr_pascal_t *p_uicr = uicr_get_pascal();

        printf("HWID: A%02u%02u-%02u%02u.%c.M%cD%cH%c", p_uicr->id.pascal.pba[0], p_uicr->id.pascal.pba[1],
               p_uicr->id.pascal.pba[2], p_uicr->id.pascal.pba[3], 'A' + p_uicr->id.pascal.pba_rev_reduced - 1,
               uicr_pba_motor_rev_is_set(p_uicr) ? '0' + p_uicr->pba_motor_rev : 'x',
               uicr_pba_display_rev_is_set(p_uicr) ? '0' + p_uicr->pba_display_rev : 'x',
               uicr_pba_hr_rev_is_set(p_uicr) ? '0' + p_uicr->pba_hr_rev : 'x');
        uicr_get_item_id_str(item_id, UICR_STRLEN_ITEM_ID);
        printf("ITEM: %s, ", item_id);

        uicr_get_serial_str(serial, UICR_STRLEN_SERIAL);
        printf("SERIAL: %s;\r\n", serial);
    } else {
        // defaults to PLUTO
        printf("HWID: A%02d%02d-%02d%02d.%c, sub_revision: %02X, ", hwid.pronto.pba[0], hwid.pronto.pba[1],
                    hwid.pronto.pba[2], hwid.pronto.pba[3], hwid.pronto.rev + 'A' - 1, hwid.pronto.subrev);

        uicr_get_item_id_str(item_id, UICR_STRLEN_ITEM_ID);
        printf("ITEM: %s, ", item_id);

        uicr_get_serial_str(serial, UICR_STRLEN_SERIAL);
        printf("SERIAL: %s;\r\n", serial);
    }

	return ERROR_OK;
}
CLI_FUNCTION(cli_hwid, "HWID", "Read hardware info");

static void print_bitcoded_word(const char *prefix, uint32_t w, uint8_t bitsize) {
    int active_ix = uicr_get_active_entry_bit_ix(w, bitsize);
    int max_ix = (32 / (bitsize + 1)) * (bitsize + 1);
    printf("%s", prefix);
    for (int i = 31; i >= 0; i--) {
        printf("%d", (w>>i) & 1);
        if ((i % (bitsize+1)) == 0) printf(" ");
    }
    printf("\r\n");

    printf("%s", prefix);
    for (int i = 31; i >= 0; i--) {
        if (i >= max_ix) {
            printf(" "); // garbage bit, not used
        } else {
            bool data_ix = ((i-bitsize) % (bitsize+1)) != 0;
            char bit_char = ((w>>i) & 1) ? '1' : '0';
            if (i >= active_ix + bitsize + 1) {
                // free entry, bits to be used
                printf(data_ix ? "." : "f");
            } else if (i >= active_ix) {
                // active entry
                printf("%c", data_ix ? bit_char : 'A');
            } else {
                // deleted entry
                printf("x");
            }
        }
        if ((i % (bitsize+1)) == 0) printf(" ");
    }
    printf("\r\n");
}

static int cli_uicr(int argc, const char **argv)
{
    const target_t *t = target_get();
    if (strcmp(TARGET_PASCAL, t->name) == 0)
    {
        //volatile const uicr_pascal_t *uicr = uicr_get_pascal();
        printf("TODO\r\n"); // TODO PETER
    }
    else
    {
        // defaults to pluto
        volatile const uicr_pluto_t *uicr = uicr_get_pluto();
        printf("id.pba    0x%02x 0x%02x 0x%02x 0x%02x [hex]\r\n", uicr->id.pronto.pba[0], uicr->id.pronto.pba[1], uicr->id.pronto.pba[2], uicr->id.pronto.pba[3]);
        printf("          %4d %4d %4d %4d [dec]\r\n", uicr->id.pronto.pba[0], uicr->id.pronto.pba[1], uicr->id.pronto.pba[2], uicr->id.pronto.pba[3]);
        printf("id.rev    0x%02x [hex] %4d [dec]\r\n", uicr->id.pronto.rev, uicr->id.pronto.rev);
        printf("id.subrev 0x%02x [hex] %4d [dec]\r\n", uicr->id.pronto.subrev, uicr->id.pronto.subrev);
        printf("serial    0x%02x 0x%02x 0x%02x 0x%02x 0x%02x [hex]\r\n", uicr->serial[0], uicr->serial[1], uicr->serial[2], uicr->serial[3], uicr->serial[4]);
        printf("          %4d %4d %4d %4d %4d [dec]\r\n", uicr->serial[0], uicr->serial[1], uicr->serial[2], uicr->serial[3], uicr->serial[4]);
        printf("itemid    0x%02x 0x%02x 0x%02x 0x%02x [hex]\r\n", uicr->itemid[0], uicr->itemid[1], uicr->itemid[2], uicr->itemid[3]);
        printf("          %4d %4d %4d %4d [dec]\r\n", uicr->itemid[0], uicr->itemid[1], uicr->itemid[2], uicr->itemid[3]);
        printf("prodtest  0x%08x\r\n", uicr->prodtest);
        print_bitcoded_word("          ", uicr->prodtest, t->uicr_info.pronto.marker_prodtest_bitsize);
        printf("coretest  0x%08x\r\n", uicr->coretest);
        print_bitcoded_word("          ", uicr->coretest, t->uicr_info.pronto.marker_coretest_bitsize);
        printf("mvttest   0x%08x\r\n", uicr->movementtest);
        print_bitcoded_word("          ", uicr->movementtest, t->uicr_info.pronto.marker_movementtest_bitsize);
        printf("x_calib   0x%08x\r\n", uicr->x_axis_calibration);
        printf("y_calib   0x%08x\r\n", uicr->y_axis_calibration);
        printf("z_calib   0x%08x\r\n", uicr->z_axis_calibration);
    }
    return ERROR_OK;
}
CLI_FUNCTION(cli_uicr, "UICR", "Read UICR");

static int cli_tatf(int argc, const char **argv)
{
    if (argc != 1) return ERR_CLI_EINVAL;
	int err = ERROR_OK;
	bool first = true;
    int val = strtol(argv[0], NULL, 0);
    const target_t *t = target_get();

	if (val > t->uicr_info.prodtest_flags || val < 0)
		return ERR_CLI_EINVAL;

	if (val == 0) {
		const uint32_t v = uicr_get_prodtest_marker();

		printf("TATF: [");

		for (uint8_t i = 0; i < t->uicr_info.prodtest_flags; i++) {
			if ((v & (1 << (i))) == 0) {
				if (!first) {
    				 printf(", ");
                }
				first = false;
				printf("%d", i + 1);
			}
		}

		printf("];\r\n");
	} else {
		if (val > 1 && (uicr_get_prodtest_marker() & (1 << (val - 2))) != 0) {
			printf("ERROR: TATF %d not set;\r\n", val - 1);
			return ERROR_OK;
		}

		err = uicr_set_prodtest_marker(val - 1);
	}

	return err;
}
CLI_FUNCTION(cli_tatf, "TATF", 
	  "Write and Read test OK flag for test 1 to 4. Example: TATF=1; Sets test 1 to ok, TATF=0; displays tests passed");

static int cli_tatc(int argc, const char **argv)
{
	int err = uicr_clear_prodtest_marker();
    if (err == -ENOMEM) {
       printf("ERROR: TATC FAILED - Reflashing required;\r\n"); 
    }
	
	return err;
}
CLI_FUNCTION(cli_tatc, "TATC", 
	  "Reset Test Ok flags. Note: This can only be done about half a dozen times!");


static int cli_ccbt(int argc, const char **argv)
{
    if (argc == 0) return ERR_CLI_EINVAL;
    if (argv[0][0] == '0') {
        printf("CCBT result : %s;\r\n", uicr_get_coretest_marker() == 0 ? "PASSED" : "FAILED");
    } else {
        int err = uicr_clear_coretest_marker();
        if (err == -ENOMEM) {
            printf("ERROR RESETTING CORETEST REGISTER, REFLASH NEEDED;\r\n"); 
        }
    }
	
	return ERROR_OK;
}
CLI_FUNCTION(cli_ccbt, "CCBT", "CCBT=0; Print value, CCBT=1; Clears CoreTest pass flag");

static int cli_scbt(int argc, const char **argv)
{
    return uicr_set_coretest_marker(0);
}
CLI_FUNCTION(cli_scbt, "SCBT", "Sets the CoreTest pass flag;");

static int cli_mvtc(int argc, const char **argv)
{
    int err = uicr_clear_mvttest_marker();
    if (err == -ENOMEM) {
        printf("ERROR RESETTING MOVEMENTTEST REGISTER, REFLASH NEEDED;\r\n"); 
    }
    return err;
}
CLI_FUNCTION(cli_mvtc, "MVTC", "MVTC=1; Clear the Movement test pass flag");

static int cli_mvts(int argc, const char **argv)
{
    return uicr_set_mvttest_marker(0);
}
CLI_FUNCTION(cli_mvts, "MVTS", "MVTS=1; Set the Movement test pass flag");

static int cli_mvtf(int argc, const char **argv) {
    printf("Movement result : %s;\r\n", uicr_get_mvttest_marker() == 0 ? "PASSED" : "FAILED");
    return ERROR_OK;
}
CLI_FUNCTION(cli_mvtf, "MVTF", "Read test OK flag for movement test. Example:. MVTF=1; displays if tests passed");


