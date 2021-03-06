#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif // HAVE_CONFIG_H

#include <err.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <nfc/nfc.h>
#include <nfc/nfc-types.h>

#include "utils/nfc-utils.h"

#define MAX_DEVICE_COUNT 16

static nfc_device *pnd = NULL;
static nfc_context *context;

static void stop_polling(int sig)
{
  (void) sig;
  if (pnd != NULL)
    nfc_abort_command(pnd);
  else {
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }
}

static void
print_usage(const char *progname)
{
  printf("usage: %s [-v]\n", progname);
  printf("  -v\t verbose display\n");
}

int
main(int argc, const char *argv[])
{
  bool verbose = false;

  signal(SIGINT, stop_polling);

  // Display libnfc version
  const char *acLibnfcVersion = nfc_version();

  printf("%s uses libnfc %s\n", argv[0], acLibnfcVersion);
  if (argc != 1) {
    if ((argc == 2) && (0 == strcmp("-v", argv[1]))) {
      verbose = true;
    } else {
      print_usage(argv[0]);
      exit(EXIT_FAILURE);
    }
  }

  const uint8_t uiPollNr = 20;
  const uint8_t uiPeriod = 2;
  const nfc_modulation nmModulations[5] = {
    { .nmt = NMT_ISO14443A, .nbr = NBR_106 },
    { .nmt = NMT_ISO14443B, .nbr = NBR_106 },
    { .nmt = NMT_FELICA, .nbr = NBR_212 },
    { .nmt = NMT_FELICA, .nbr = NBR_424 },
    { .nmt = NMT_JEWEL, .nbr = NBR_106 },
  };
  const size_t szModulations = 5;

  nfc_target nt;
  int res = 0;

  nfc_init(&context);
  if (context == NULL) {
    ERR("Unable to init libnfc (malloc)");
    exit(EXIT_FAILURE);
  }

  pnd = nfc_open(context, NULL);

  if (pnd == NULL) {
    ERR("%s", "Unable to open NFC device.");
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  if (nfc_initiator_init(pnd) < 0) {
    nfc_perror(pnd, "nfc_initiator_init");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  printf("NFC reader: %s opened\n", nfc_device_get_name(pnd));
  printf("NFC device will poll during %ld ms (%u pollings of %lu ms for %" PRIdPTR " modulations)\n", (unsigned long) uiPollNr * szModulations * uiPeriod * 150, uiPollNr, (unsigned long) uiPeriod * 150, szModulations);
  if ((res = nfc_initiator_poll_target(pnd, nmModulations, szModulations, uiPollNr, uiPeriod, &nt))  < 0) {
    nfc_perror(pnd, "nfc_initiator_poll_target");
    nfc_close(pnd);
    nfc_exit(context);
    exit(EXIT_FAILURE);
  }

  if (res > 0) {
    print_nfc_target(&nt, verbose);
  } else {
    printf("No target found.\n");
  }

  printf("Waiting for card removing...");
  while (0 == nfc_initiator_target_is_present(pnd, NULL)) {}
  nfc_perror(pnd, "nfc_initiator_target_is_present");
  printf("done.\n");

  nfc_close(pnd);
  nfc_exit(context);
  exit(EXIT_SUCCESS);
}