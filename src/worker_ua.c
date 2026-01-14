#include "config.h"

#include <string.h>
#include "libckpool.h"
#include "ua_utils.h"
#include "worker_ua.h"
#include "uthash.h"
#include "utlist.h"

void recalc_worker_useragent(sdata_t *sdata, user_instance_t *user, worker_instance_t *worker)
{
	if (!user || !worker)
		return;

	/* Fast path using instance_count: keep persisted UA if no clients
	 * attached; set to "Other" if multiple clients attached; otherwise pick
	 * the single client's UA. This function is intended to be called with the
	 * instance_lock held. */
	if (worker->instance_count == 0) {
		/* preserve persisted UA */
		return;
	}

	if (worker->instance_count > 1) {
		/* Multiple active instances — set to generic token if it changed */
		if (!worker->useragent || strcmp(worker->useragent, "Other") != 0) {
			if (worker->useragent)
				free(worker->useragent);
			worker->useragent = strdup("Other");
			strncpy(worker->norm_useragent, "Other", sizeof(worker->norm_useragent) - 1);
			worker->norm_useragent[sizeof(worker->norm_useragent) - 1] = '\0';
		}
		return;
	}

	/* instance_count == 1: find the single client attached to this worker */
	stratum_instance_t *c;
	stratum_instance_t *client = NULL;

	DL_FOREACH2(user->clients, c, user_next) {
		if (c->worker_instance == worker) {
			client = c;
			break;
		}
	}

	if (client && client->useragent && strlen(client->useragent)) {
		if (worker->useragent)
			free(worker->useragent);
		worker->useragent = strdup(client->useragent);
		normalize_ua_buf(worker->useragent, worker->norm_useragent, sizeof(worker->norm_useragent));
	} else {
		/* Empty UA from client — record empty string */
		if (worker->useragent)
			free(worker->useragent);
		worker->useragent = ckzalloc(1);
		worker->norm_useragent[0] = '\0';
	}
}
