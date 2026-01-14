#ifndef WORKER_UA_H
#define WORKER_UA_H

#include "stratifier_internal.h"

/* Recalculate worker useragent based on active instances. Thread-safety: caller
 * must hold the instance_lock when invoking this function. */
void recalc_worker_useragent(sdata_t *sdata, user_instance_t *user, worker_instance_t *worker);

#endif /* WORKER_UA_H */
